#pragma once

#include "imgui/ImFileDialog.h"

namespace LuaFsm
{
    class Popup
    {
    public:
        //static IGFD::FileDialogConfig config;
        virtual ~Popup() = default;
        std::string id;
        virtual void DrawFields(){}
        void Draw();
        virtual void DrawButtons(){}
        virtual void Open();
        bool isOpen = false;
        static bool OpenFileDialog(
            const std::string& key,
            const std::string& title,
            const std::string& ext,
            std::string* folder,
            std::string* filePath,
            const IGFD::FileDialogConfig& config
            );

        [[nodiscard]] std::string MakeIdString(const std::string& name) const
        {
            return name + "##" + id;
        }
    };

    class PopupManager
    {
    public:
        void AddPopup(int id, std::shared_ptr<Popup> popup);
        void RemovePopup(int id);
        std::shared_ptr<Popup> GetPopup(int id);
        void OpenPopup(int id);
        void ShowOpenPopups();
    private:
        std::unordered_map<int, std::shared_ptr<Popup>> m_Popups;
    };
    
    class AddFsmPopup : public Popup
    {
    public:
        AddFsmPopup();
        void DrawFields() override;
        void DrawButtons() override;
        std::string fsmName;
        std::string fsmId;
    };
    
    class ThemeEditor : public Popup
    {
    public:
        ThemeEditor();
        void DrawFields() override;
        void DrawButtons() override;
        void Open() override;
        std::string name;
        std::string lastActiveTheme;
        ImVec4 standard;
        ImVec4 hovered;
        ImVec4 active;
        ImVec4 text;
        ImVec4 background;
        ImVec4 windowBg;
    };
    
    class LoadFile : public Popup
    {
    public:
        LoadFile();
        void DrawFields() override;
        void Open() override;
        std::string filePath;
        std::string folder;
        IGFD::FileDialogConfig config;
    };
    
    class CreateFilePopup : public Popup
    {
    public:
        CreateFilePopup();
        void DrawFields() override;
        std::string filePath;
        std::string folder;
        IGFD::FileDialogConfig config;
    };

    class AddStatePopup : public Popup
    {
    public:
        explicit AddStatePopup(const std::string& instanceId);
        void DrawFields() override;
        void DrawButtons() override;
        std::string stateId;
        std::string name;
        std::string parent;
        bool isCopy = false;
        bool isDrawn = false;
        ImVec2 position{-1,-1};
    };

    class AddTriggerPopup : public Popup
    {
    public:
        explicit AddTriggerPopup(const std::string& instanceId);
        void DrawFields() override;
        void DrawButtons() override;
        std::string triggerId;
        std::string name;
        std::string parent;
        bool isCopy = false;
        bool isDrawn = false;
        ImVec2 position{-1,-1};
    };

    class UnlinkTriggerPopup : public Popup
    {
    public:
        explicit UnlinkTriggerPopup(const std::string& instanceId);
        std::string parent;
        void DrawButtons() override;
        std::string triggerId;
    };

    class UnlinkStatePopup : public Popup
    {
    public:
        explicit UnlinkStatePopup(const std::string& instanceId);
        std::string parent;
        void DrawButtons() override;
        std::string stateId;
    };

    class DeleteTriggerPopup : public Popup
    {
    public:
        explicit DeleteTriggerPopup(const std::string& instanceId);
        std::string parent;
        void DrawButtons() override;
    };

    class DeleteStatePopup : public Popup
    {
    public:
        explicit DeleteStatePopup(const std::string& instanceId);
        std::string parent;
        void DrawButtons() override;
    };

    class RefactorIdPopup : public Popup
    {
    public:
        explicit RefactorIdPopup(const std::string& instanceId, const std::string& objType);
        std::string parent;
        std::string objType;
        std::string newId;
        void DrawFields() override;
        void DrawButtons() override;
    };


    

    
}
