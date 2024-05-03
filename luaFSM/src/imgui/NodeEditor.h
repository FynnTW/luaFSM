#pragma once

namespace LuaFsm
{
    class NodeEditor
    {
    public:
        void AddNode(const std::string& name);
        [[nodiscard]] std::unordered_map<std::string, int> GetNodeIds() const { return m_NodeIds; }
        int GetNodeId(const std::string& name) { return m_NodeIds[name]; }
        int GetNodeInputId(const std::string& name) { return m_NodeIds[name] + 1; }
        int GetNodeOutputId(const std::string& name) { return m_NodeIds[name] + 2; }

    private:
        std::unordered_map<std::string, int> m_NodeIds{};
    };
    
}
