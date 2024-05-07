#pragma once

namespace LuaFsm
{
    /**
     * \brief Base blueprint for objects with a name and a unique id
     */
    class DrawableObject
    {
    public:
        virtual ~DrawableObject() = default;
    
        //Name in this context is for visual editor purposes
        [[nodiscard]] virtual std::string GetName() const { return m_Name; }
        void virtual SetName(const std::string& name) { m_Name = name; }

        //Id is used for internal purposes
        [[nodiscard]] virtual std::string GetId() const { return m_Id; }
        void virtual SetId(const std::string& id) { m_Id = id; }
    
    protected:
        std::string m_Name;
        std::string m_Id;
    };
    
}
