#pragma once

class DrawableObject
{
public:
    [[nodiscard]] virtual std::string GetName() const { return m_Name; }
    [[nodiscard]] virtual std::string GetId() const { return m_Id; }
    void virtual SetName(const std::string& name) { m_Name = name; }
    void virtual SetId(const std::string& id) { m_Id = id; }
protected:
    std::string m_Name;
    std::string m_Id;
    
};
