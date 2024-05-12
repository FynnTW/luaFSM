#pragma once

namespace LuaFsm
{
    /**
     * \brief Base blueprint for objects with a name and a unique id
     */
    class DrawableObject
    {
    public:
        virtual ~DrawableObject();
        [[nodiscard]] std::string MakeIdString(const std::string& name) const;

        DrawableObject(const DrawableObject& other) = default;

        DrawableObject(DrawableObject&& other) noexcept
            : m_Name(std::move(other.m_Name)),
              m_Id(std::move(other.m_Id))
        {
        }

        explicit DrawableObject(std::string id) : m_Id(std::move(id)) {}

        DrawableObject& operator=(const DrawableObject& other)
        {
            if (this == &other)
                return *this;
            m_Name = other.m_Name;
            m_Id = other.m_Id;
            return *this;
        }

        DrawableObject& operator=(DrawableObject&& other) noexcept
        {
            if (this == &other)
                return *this;
            m_Name = std::move(other.m_Name);
            m_Id = std::move(other.m_Id);
            return *this;
        }

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
