#pragma once
#include <iostream>
#include <fstream>

namespace LuaFsm
{
    class FileReader
    {
    public:
        std::string ReadLine();
        void Open(const std::string& path);
        void Close();
        bool IsOpen() const;
        int GetLineNumber() const;
        void SetLineNumber(int lineNumber);
        int GetCurrentPosition();
        [[nodiscard]] int GetStreamSize() const { return m_StreamSize; }
        void SetCurrentPosition(const int position);
        void SetStreamSize(const int size) { m_StreamSize = size; }
        void ReadBytes(char* buffer, int size);
        std::string ReadWord();
        std::string GetVarName(const std::string& identifier);
        std::string GetVarName(std::string& line, const std::string& identifier);
        bool LineContains(const std::string& identifier);
        static bool LineContains(std::string& line, const std::string& identifier);
        static bool CommentContains(std::string& line, const std::string& identifier);
        std::string GetTableField(const std::string& identifier);
        std::string GetTableField(std::string& line, const std::string& identifier);
        bool IsEndOfFile() const;
        static void RemoveLuaComments(std::string& line);
        static void RemoveTabs(std::string& line);
        static std::string ReadAllText(const std::string& path);
        static std::string RemoveStartingTab(const std::string& input);

    private:
        std::ifstream m_File;
        int m_LineNumber = 0;
        int m_StreamSize = 0;
        int m_CurrentPosition = 0;
        int m_LastPosition = 0;
    };
    
}
