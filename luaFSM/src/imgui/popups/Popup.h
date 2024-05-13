#pragma once

#include "imgui/ImFileDialog.h"

namespace LuaFsm
{
    
    class Popup
    {
    public:
        Popup(const Popup& other) = default;

        Popup(Popup&& other) noexcept
            : id(std::move(other.id)),
              isOpen(other.isOpen)
        {
        }

        Popup() = default;

        Popup& operator=(const Popup& other)
        {
            if (this == &other)
                return *this;
            id = other.id;
            isOpen = other.isOpen;
            return *this;
        }

        Popup& operator=(Popup&& other) noexcept
        {
            if (this == &other)
                return *this;
            id = std::move(other.id);
            isOpen = other.isOpen;
            return *this;
        }

        virtual ~Popup();
        std::string id;
        virtual void DrawFields(){}
        bool Draw();
        virtual void DrawButtons(){}
        virtual void Open();
        virtual void Close();
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

    class OptionsPopUp : public Popup
    {
    public:
        explicit OptionsPopUp();
        bool appendToFile = true;
        bool showPriority = false;
        bool functionEditorOnly = false;
        void DrawFields() override;
        void DrawButtons() override;
    };

    class PopupManager
    {
    public:
        template<typename T>
        void AddPopup(T id, const std::shared_ptr<Popup>& popup)
        {
            m_Popups[static_cast<int>(id)] = popup;
        }
        
        template<typename T>
        void RemovePopup(const T id)
        {
            if (m_Popups.contains(static_cast<int>(id)))
                m_Popups.erase(static_cast<int>(id));
        }
        
        template<typename ReturnType, typename T>
        std::shared_ptr<ReturnType> GetPopup(const T id)
        {
            if (m_Popups.contains(static_cast<int>(id)))
                return std::dynamic_pointer_cast<ReturnType>(m_Popups[static_cast<int>(id)]);
            return {};
        }

        template<typename T>
        void OpenPopup(const T id)
        {
            if (m_Popups.contains(static_cast<int>(id)))
                m_Popups[static_cast<int>(id)]->isOpen = true;
        }
        
        void ShowOpenPopups();
    private:
        std::unordered_map<int, std::shared_ptr<Popup>> m_Popups;
    };

}
