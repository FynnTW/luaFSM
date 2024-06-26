﻿#pragma once
#include <memory>
#include <string>

#include "DrawableObject.h"
#include "FsmState.h"
#include "FsmTrigger.h"
#include "json.hpp"
#include "imgui/popups/Popup.h"

namespace LuaFsm
{
    
    class FsmState;
    class FsmTrigger;
    typedef std::shared_ptr<FsmState> FsmStatePtr;
    typedef std::shared_ptr<FsmTrigger> FsmTriggerPtr;
    
    enum class FsmPopups : int
    {
        SetNewId
    };
    
    /**
     * \brief Represents a Finite State Machine
     */
    class Fsm final : public DrawableObject
    {
    public:
        explicit Fsm(const std::string& id);
        
        [[nodiscard]] std::unordered_map<std::string, FsmStatePtr> GetStates() const { return m_States; }
        void ClearStates() { m_States.clear(); }
        
        FsmStatePtr GetState(const std::string& key);
        FsmStatePtr AddState(const std::string& key);
        void AddState(const FsmStatePtr& value);
        void RemoveState(const std::string& state);
        
        [[nodiscard]] std::string GetInitialStateId() const { return m_InitialStateId; }
        void SetInitialState(const std::string& initialState) { m_InitialStateId = initialState; }
        FsmState* GetInitialState();
        
        std::unordered_map<std::string, FsmTriggerPtr> GetTriggers();
        void ClearTriggers() { m_Triggers.clear(); }
        
        void AddTrigger(const FsmTriggerPtr& value);
        FsmTriggerPtr GetTrigger(const std::string& key);
        void RemoveTrigger(const std::string& trigger);
        void ChangeTriggerId(const std::string& oldId, const std::string& newId);

        std::string GetLinkedFile() const { return m_LinkedFile; }
        void SetLinkedFile(const std::string& linkedFile) { m_LinkedFile = linkedFile; }

        std::string GetLinkedFileCode() const;
        void SaveLinkedFile(const std::string& code);
        
        std::string GetLuaCode();

        static std::shared_ptr<Fsm> CreateFromFile(const std::string& filePath);
        void UpdateFromFile(const std::string& filePath);
        void UpdateToFile(const std::string& oldId);
        void UpdateFileContents(std::string& code, const std::string& oldId);

        void DrawProperties();
        void LastState();
        void CheckForChanges();
        void RefactorId(const std::string& newId);

        bool IsUnSaved() const { return m_UnSaved; }
        void SetUnSaved(const bool unSaved) { m_UnSaved = unSaved; }

        bool IsUnSavedGlobal() const { return m_UnSavedGlobal; }
        void SetUnSavedGlobal(const bool unSavedGlobal) { m_UnSavedGlobal = unSavedGlobal; }
        
        void UpdateEditors();
        std::string GetActivateFunctionCode();
        
        PopupManager* GetPopupManager() {return &m_PopupManager;}
        void InitPopups();

    private:
        std::unordered_map<std::string, FsmStatePtr> m_States{};
        std::unordered_map<std::string, FsmTriggerPtr> m_Triggers{};
        std::string m_InitialStateId;
        std::string m_LastInitialStateId;
        std::string m_LastName;
        std::string m_LinkedFile = "";
        bool m_UnSaved = false;
        bool m_UnSavedGlobal = false;
        TextEditor m_LuaCodeEditor;
        PopupManager m_PopupManager;
    };
}
