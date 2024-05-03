#include "pch.h"
#include "NodeEditor.h"

namespace LuaFsm
{
    void NodeEditor::AddNode(const std::string& name)
    {
        if (m_NodeIds.contains(name))
            return;
        m_NodeIds[name] = static_cast<int>(m_NodeIds.size()) + 10;
    }
}
