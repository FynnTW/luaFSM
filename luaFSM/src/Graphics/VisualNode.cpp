#include "pch.h"
#include "VisualNode.h"

namespace LuaFsm
{
    
    void VisualNode::HandleSelection(NodeEditor* editor)
    {
        if (ImGui::IsWindowHovered())
        {
            HighLight();
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                editor->SetSelectedNode(this);
        }
        else if (!IsSelected())
            UnHighLight();
        else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            editor->DeselectAllNodes();
    }

    ImVec2 VisualNode::GetTextPos(const char* text)
    {
        const ImVec2 textSize = ImGui::CalcTextSize(text);
        return {GetDrawPos().x - textSize.x / 2, GetDrawPos().y - textSize.y / 2};
    }
}
