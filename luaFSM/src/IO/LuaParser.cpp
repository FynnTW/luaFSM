#include "pch.h"
#include "LuaParser.h"
#include <sstream>
#include <regex>

namespace LuaFsm
{
    struct LuaLine
    {
        std::string text;
        bool isComment = false;
        bool isAnnotation = false;
        bool isMultiLineComment = false;
        bool isFunctionBody = false;
        bool isFunctionDeclaration = false;
        bool isFunctionArgs = false;
        bool isTableBody = false;
        bool isVariableValue = false;
        bool isStringLiteral = false;
        int openingStatementCount = 0;
    };

    std::shared_ptr<LuaFile> LuaParser::Parse(const std::string& content)
    {
        auto luaFile = std::make_shared<LuaFile>();
        std::istringstream stringStream(content);
        std::string commentCache;
        LuaLine line{};
        std::shared_ptr<LuaFunction> function;
        std::vector<std::shared_ptr<LuaTable>> tables;
        std::string functionName;
        std::string variableName;
        std::string fieldName;
        std::string stringLiteral;
        std::string functionBody;
        std::vector<LuaAnnotation> annotations;
        std::string word;
        while (std::getline(stringStream, line.text))
        {
            if (line.text.empty())
            {
                annotations.clear();
                commentCache.clear();
                continue;
            }
            std::istringstream lineStream(line.text);
            while (lineStream >> word)
            {
                std::smatch matches{};
                if (line.isComment)
                {
                    if (line.isMultiLineComment && std::regex_search(word, matches, m_MultiLineCommentEnd))
                    {
                        line.isMultiLineComment = false;
                        line.isComment = false;
                        word = matches.suffix().str();
                        matches = {};
                    }
                }
                else if (std::regex_search(word, matches, m_CommentStart))
                {
                    line.isComment = true;
                    matches = {};
                    if (std::regex_search(word, matches, m_MultiLineCommentStart))
                    {
                        line.isComment = true;
                        line.isMultiLineComment = true;
                        matches = {};
                    }
                    else if (std::regex_search(word, matches, m_AnnotationStart))
                    {
                        line.isAnnotation = true;
                        matches = {};
                    }
                    else if (line.text.find('@') != std::string::npos)
                    {
                        lineStream >> word;
                        if (word.find_first_of('@') == 0)
                            line.isAnnotation = true;
                    }
                    if (line.isAnnotation)
                    {
                        auto annotationType = word.substr(word.find_first_of('@') + 1);
                        std::string annotationValue;
                        while (lineStream >> word)
                        {
                            annotationValue += " " + word;
                        }
                        annotations.emplace_back(annotationType, annotationValue);
                    }
                }
                if (!line.isComment)
                {
                    if (word.find_first_of('"') != std::string::npos)
                    {
                        stringLiteral += word.substr(word.find_first_of('"') + 1, word.find_last_of('"') - 1);
                        while (lineStream >> word)
                        {
                            stringLiteral += " " + word.substr(0, word.find_last_of('"'));
                            if (word.find_first_of('"') != std::string::npos)
                            {
                                break;
                            }
                        }
                        line.isStringLiteral = false;
                    }
                    if (std::regex_search(word, matches, m_FunctionStart))
                    {
                        function = std::make_shared<LuaFunction>();
                        line.isFunctionDeclaration = true;
                        lineStream >> word;
                        matches = {};
                    }
                    if (line.isFunctionDeclaration && std::strcmp(word.c_str(), "function") != 0)
                    {
                        if (word.find_first_of('(') == std::string::npos)
                            functionName += word;
                        if (word.find_first_of('(') != std::string::npos)
                        {
                            functionName += word.substr(0, word.find_first_of('('));
                            line.isFunctionDeclaration = false;
                            line.isFunctionArgs = true;
                            if (word.length() > word.find_first_of('(') + 1)
                                word = word.substr(word.find_first_of('(') + 1);
                            if (functionName.find_first_of(".:") != std::string::npos)
                            {
                                auto tableName = functionName.substr(0, functionName.find_first_of(".:"));
                                auto funcName = functionName.substr(functionName.find_first_of(".:") + 1);
                                if(!luaFile->tables.contains(tableName))
                                {
                                    luaFile->tables[tableName] = std::make_shared<LuaTable>();
                                }
                                luaFile->tables[tableName]->fields[funcName] = function;
                                function->table = tableName;
                                functionName.clear();
                                function->name = functionName;
                                function->varType = "function";
                            }
                            else
                            {
                                function->name = "global." + functionName;
                                function->table = "global";
                                functionName.clear();
                                function->varType = "function";
                            }
                        }
                    }
                    if (line.isFunctionArgs && std::strcmp(word.c_str(), "(") != 0)
                    {
                        if (word.find_first_of(')') != std::string::npos)
                        {
                            line.isFunctionArgs = false;
                            line.isFunctionBody = true;
                            for (const auto& [varType, value] : annotations)
                            {
                                if (varType == "return")
                                {
                                    std::istringstream returnStream(value);
                                    std::string type, name;
                                    std::string returns;
                                    returnStream >> returns;
                                    if (!returns.empty())
                                        type = returns;
                                    returnStream >> returns;
                                    if (!returns.empty())
                                        name = returns;
                                    auto variable = std::make_shared<LuaVariable>();
                                    variable->varType = type;
                                    variable->name = name;
                                    function->returns.push_back(variable);
                                }
                            }
                            luaFile->functions[function->name] = function;
                            annotations.clear();
                            if (line.text.find_first_of(')') + 1 < line.text.length())
                                line.text = line.text.substr(line.text.find_first_of(')') + 1) + "\n";
                            else
                                line.text = "";
                        }
                        if (word.find_first_of(')') != 0)
                        {
                            word = word.substr(0, word.find_first_of(')'));
                            std::ranges::replace(word, ',', ' ');
                            std::istringstream argStream(word);
                            std::string arg;
                            while (argStream >> arg)
                            {
                                function->arguments[arg] = std::make_shared<LuaVariable>();
                                function->arguments[arg]->name = arg;
                                function->arguments[arg]->varType = "any";
                                for (const auto& [varType, value] : annotations)
                                {
                                    if (varType == "param")
                                    {
                                        std::istringstream paramStream(value);
                                        std::string param;
                                        paramStream >> param;
                                        if (!param.empty() && std::strcmp(param.c_str(), arg.c_str()) == 0)
                                        {
                                            paramStream >> param;
                                            function->arguments[arg]->varType = param;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (line.isFunctionBody)
                    {
                        for (const auto& openingKeyword : luaOpeningKeywords)
                        {
                            if (std::regex_search(word, matches, openingKeyword))
                            {
                                line.openingStatementCount++;
                                matches = {};
                                break;
                            }
                        }
                        if (std::regex_search(word, matches, m_FunctionEnd))
                        {
                            if (line.openingStatementCount == 0)
                            {
                                functionBody += matches.prefix().str();
                                line.isFunctionBody = false;
                                function->body = functionBody;
                                luaFile->functions[function->name] = function;
                                function.reset();
                                functionBody.clear();
                                matches = {};
                            }
                            else
                            {
                                line.openingStatementCount--;
                            }
                        }
                    }
                    if (!line.isFunctionBody
                        && !line.isFunctionArgs
                        && !line.isFunctionDeclaration
                        )
                    {
                        bool isKeyword = false;
                        for (const auto& keyword : luaKeywords)
                        {
                            if (std::strcmp(word.c_str(), keyword.c_str()) == 0)
                            {
                                isKeyword = true;
                            }
                        }
                        if (!isKeyword)
                        {
                            if (fieldName.empty() && word.find_first_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_") != std::string::npos)
                                fieldName = word.substr(0, word.find_first_of('='));
                            if (word.find_first_of('{') != std::string::npos)
                            {
                                line.isTableBody = true;
                                tables.emplace_back(std::make_shared<LuaTable>());
                                tables.back()->name = fieldName;
                                fieldName = "";
                                tables.back()->varType = "table";
                                word = word.substr(word.find_first_of('{') + 1);
                                tables.back()->comments = commentCache;
                                line.isVariableValue = false;
                            }
                            if (line.isTableBody)
                            {
                                if (!line.isVariableValue)
                                {
                                    if (word.find_first_of('=') != std::string::npos)
                                    {
                                        if (fieldName.empty() && word.find_first_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_") != std::string::npos)
                                            fieldName = word.substr(0, word.find_first_of('='));
                                        word = word.substr(word.find_first_of('=') + 1);
                                        line.isVariableValue = true;
                                    }
                                    else
                                    {
                                        fieldName = word;
                                    }
                                }
                                if (word.find_first_of(',') != std::string::npos || word.find_first_of('}') != std::string::npos
                                    && (!fieldName.empty() && !stringLiteral.empty()))
                                {
                                    auto variable = std::make_shared<LuaVariable>();
                                    if (line.isVariableValue)
                                        variable->name = fieldName;
                                    else
                                        variable->name = std::to_string(tables.back()->fields.size());
                                    line.isVariableValue = false;
                                    if (stringLiteral.empty())
                                        variable->value =  word.substr(0, word.find_first_of(",}"));
                                    else
                                        variable->value = stringLiteral;
                                    stringLiteral = "";
                                    variable->varType = "any";
                                    tables.back()->fields[variable->name] = variable;
                                    fieldName.clear();
                                }
                                if (word.find_first_of('}') != std::string::npos)
                                {
                                    if (tables.size() == 1)
                                        luaFile->tables[tables.back()->name] = tables.back();
                                    else
                                        tables[tables.size() - 2]->tables[tables.back()->name] = tables.back();
                                    tables.pop_back();
                                    fieldName = "";
                                    if (tables.empty())
                                        line.isTableBody = false;
                                }
                            }
                        }
                    }
                }
            }
            if (!line.isMultiLineComment)
            {
                if (line.isComment && !line.isFunctionBody && !line.isAnnotation)
                    commentCache += line.text + "\n";
                line.isComment = false;
            }
            if (line.isFunctionBody)
                functionBody += line.text + "\n";
            line.isAnnotation = false;
        }
        return luaFile;
    }
}
