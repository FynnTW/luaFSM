#pragma once
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

namespace LuaFsm
{
    class LuaFile;
    class LuaParser
    {
    public:
        static std::shared_ptr<LuaFile> Parse(const std::string& content);
        std::vector<std::string> luaTypes = {
            "nil",
            "boolean",
            "number",
            "string",
            "function",
            "userdata",
            "thread",
            "table"
        };
        inline static std::vector<std::string> luaKeywords = {
            "and",
            "break",
            "do",
            "else",
            "elseif",
            "end",
            "false",
            "for",
            "function",
            "if",
            "in",
            "local",
            "nil",
            "not",
            "or",
            "repeat",
            "return",
            "then",
            "true",
            "until",
            "while"
        };

        inline static std::vector<std::regex> luaOpeningKeywords = {
            std::regex("\\bif\\b"),
            std::regex("\\bfor\\b"),
            std::regex("\\bwhile\\b"),
        };

    private:
        inline static std::regex m_AnnotationStart = std::regex("@"); // Annotation start
        inline static std::regex m_CommentStart = std::regex("--"); // Comment start
        inline static std::regex m_StringBoundary = std::regex("\""); // String
        inline static std::regex m_TableStart = std::regex("\\{"); // Table start
        inline static std::regex m_TableEnd = std::regex("\\}"); // Table end
        inline static std::regex m_FunctionStart = std::regex("\\bfunction\\b"); // Function start
        inline static std::regex m_FunctionEnd = std::regex("\\bend"); // Function end
        inline static std::regex m_VariableAssignment = std::regex("="); // Variable assignment
        inline static std::regex m_MultiLineCommentStart = std::regex("\\[\\[");
        inline static std::regex m_MultiLineCommentEnd = std::regex("\\]\\]");
        inline static std::regex m_TableSeparator = std::regex("\\,");
        inline static std::regex m_FieldAccessor = std::regex("\\.");
        inline static std::regex m_InstanceCall = std::regex("\\:");

    };

    class LuaAnnotation
    {
    public:
        std::string varType;
        std::string value;
    };

    class LuaVariable
    {
    public:
        std::string name;
        std::string value;
        std::string varType;
        bool isLocal = false;
    };

    class LuaTable : public LuaVariable
    {
    public:
        std::string comments;
        std::unordered_map<std::string, std::shared_ptr<LuaVariable>> fields{};
        std::unordered_map<std::string, std::shared_ptr<LuaTable>> tables{};
        std::pair<std::string, std::string> tableType{};
    };

    class LuaFunction : public LuaVariable
    {
    public:
        std::string body;
        std::string comments;
        std::unordered_map<std::string, std::shared_ptr<LuaVariable>> arguments {};
        std::vector<std::shared_ptr<LuaVariable>> returns {};
        std::string table;
    };

    class LuaFile
    {
    public:
        std::unordered_map<std::string, std::shared_ptr<LuaTable>> tables{};
        std::unordered_map<std::string, std::shared_ptr<LuaFunction>> functions{};
        std::unordered_map<std::string, std::shared_ptr<LuaVariable>> variables{};
    };
    
}
