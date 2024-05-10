#include "pch.h"
#include "FileReader.h"

#include "Log.h"
#include "imgui/ImGuiNotify.hpp"

namespace LuaFsm
{
    std::string FileReader::ReadLine()
    {
        std::string line;
        if (std::getline(m_File, line))
        {
            ++m_LineNumber;
        }
        return line;
    }

    void FileReader::Open(const std::string& path)
    {
        m_File.open(path);
        if (!m_File.is_open())
        {
            LOG_ERROR("Failed to open file: {0}", path);
        }

        // Save the current position
        const std::streampos currentPos = m_File.tellg();

        // Seek to the end of the file
        m_File.seekg(0, std::ios::end);

        // Get the position (this is the file size)
        m_StreamSize = m_File.tellg();

        // Seek back to the original position
        m_File.seekg(currentPos);
    }

    void FileReader::Close()
    {
        m_File.close();
    }

    bool FileReader::IsOpen() const
    {
        return m_File.is_open();
    }

    int FileReader::GetLineNumber() const
    {
        return m_LineNumber;
    }

    void FileReader::SetLineNumber(const int lineNumber)
    {
        m_LineNumber = lineNumber;
    }

    int FileReader::GetCurrentPosition()
    {
        if (!m_File.is_open())
        {
            LOG_ERROR("File is not open");
            return -1;
        }
        return m_File.tellg();
    }

    void FileReader::SetCurrentPosition(const int position)
    {
        if (!m_File.is_open())
        {
            LOG_ERROR("File is not open");
            return;
        }
        m_File.seekg(position);
    }

    void FileReader::ReadBytes(char* buffer, const int size)
    {
        if (!m_File.is_open())
        {
            LOG_ERROR("File is not open");
            return;
        }
        m_File.read(buffer, size);
    }

    std::string FileReader::ReadWord()
    {
        if (!m_File.is_open())
        {
            LOG_ERROR("File is not open");
            return "";
        }
        std::string buffer;
        m_File >> buffer;
        return buffer;
    }

    std::string FileReader::GetVarName(const std::string& identifier)
    {
        auto line = ReadLine();
        RemoveLuaComments(line);
        if (line.empty())
            return "";
        const auto pos = line.find(identifier);
        if (pos == std::string::npos)
        {
            return "";
        }
        if (const size_t equalPos = line.find_last_of('=', pos); equalPos != std::string::npos)
        {
            std::string varName = line.substr(0, equalPos);
            // Remove trailing whitespaces
            varName.erase(std::find_if(varName.rbegin(), varName.rend(), [](const unsigned char ch) {
                return !std::isspace(ch);
            }).base(), varName.end());
            return varName;
        }

        return "";
    }

    std::string FileReader::GetVarName(std::string& line, const std::string& identifier)
    {
        RemoveLuaComments(line);
        if (line.empty())
            return "";
        const auto pos = line.find(identifier);
        if (pos == std::string::npos)
        {
            return "";
        }
        if (const size_t equalPos = line.find_last_of('=', pos); equalPos != std::string::npos)
        {
            std::string varName = line.substr(0, equalPos);
            // Remove trailing whitespaces
            varName.erase(std::find_if(varName.rbegin(), varName.rend(), [](const unsigned char ch) {
                return !std::isspace(ch);
            }).base(), varName.end());
            return varName;
        }

        return "";
    }

    bool FileReader::LineContains(const std::string& identifier)
    {
        const auto currentPos = m_File.tellg();
        auto line = ReadLine();
        RemoveLuaComments(line);
        if (line.empty())
            return false;
        if (line.find(identifier) == std::string::npos)
            return false;
        m_LineNumber--;
        m_File.seekg(currentPos);
        return true;
    }

    void RemoveStartingWhiteSpace(std::string& line)
    {
        line.erase(0, line.find_first_not_of(' '));
        line.erase(0, line.find_first_not_of('\t'));
    }
    
    bool FileReader::LineContains(std::string& line, const std::string& identifier)
    {
        RemoveLuaComments(line);
        RemoveTabs(line);
        if (line.empty())
            return false;
        if (line.find(identifier) == std::string::npos)
            return false;
        return true;
    }
    
    bool FileReader::CommentContains(std::string& line, const std::string& identifier)
    {
        RemoveTabs(line);
        if (line.empty())
            return false;
        if (line.find(identifier) == std::string::npos)
            return false;
        return true;
    }

    std::string FileReader::GetTableField(const std::string& identifier)
    {
        auto line = ReadLine();
        RemoveLuaComments(line);
        RemoveTabs(line);
        if (line.empty())
            return "";
        const auto pos = line.find(identifier);
        if (pos == std::string::npos)
        {
            return "";
        }
        if (const size_t equalPos = line.find_last_of('=', pos); equalPos != std::string::npos)
        {
            std::string varName = line.substr(equalPos, line.size() - 1);
            // Remove trailing whitespaces
            varName.erase(std::find_if(varName.rbegin(), varName.rend(), [](const unsigned char ch) {
                return !std::isspace(ch);
            }).base(), varName.end());
            return varName;
        }

        return "";
    }

    std::string FileReader::GetTableField(std::string& line, const std::string& identifier)
    {
        RemoveLuaComments(line);
        RemoveTabs(line);
        if (line.empty())
            return "";
        const auto pos = line.find(identifier);
        if (pos == std::string::npos)
        {
            return "";
        }
        if (const size_t equalPos = line.find_last_of('='); equalPos != std::string::npos)
        {
            std::string varName = line.substr(equalPos + 1, line.size() - 1);
            const auto stringNameStart = varName.find_first_of('"');
            if (const auto stringNameEnd = varName.find_last_of('"');
                stringNameStart != std::string::npos && stringNameEnd != std::string::npos)
            {
                varName = varName.substr(stringNameStart + 1, stringNameEnd - 2);
            }
            return varName;
        }

        return "";
    }

    bool FileReader::IsEndOfFile() const
    {
        return m_File.eof();
    }

    void FileReader::RemoveLuaComments(std::string& line)
    {
        const auto pos = line.find("--");
        if (pos != std::string::npos)
        {
            line.erase(pos, line.size());
        }
    }

    void FileReader::RemoveTabs(std::string& line)
    {
        std::erase(line, '\t');
    }

    std::string FileReader::ReadAllText(const std::string& path)
    {
        std::string filePath = path;
        std::ranges::replace(filePath, '\\', '/');
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            ImGui::InsertNotification({ImGuiToastType::Error, 3000, "Can not open file: %s", filePath.c_str()});
            LOG_ERROR("Failed to open file: {0}", filePath);
            return "";
        }
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return content;
    }

    std::string FileReader::RemoveStartingTab(const std::string& input)
    {
        std::istringstream iss(input);
        std::ostringstream oss;
        std::string line;

        while (std::getline(iss, line)) {
            // Check if the line starts with a tab and remove it
            if (!line.empty() && line[0] == '\t') {
                line.erase(0, 1);  // Erase the first character if it's a tab
            }
            oss << line;
            if (!iss.eof()) {
                oss << '\n';  // Add newline back except for the last line
            }
        }

        return oss.str();
    }
}
