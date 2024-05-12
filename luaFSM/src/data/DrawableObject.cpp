#include "pch.h"
#include "DrawableObject.h"

namespace LuaFsm
{
    DrawableObject::~DrawableObject()
    = default;

    std::string DrawableObject::MakeIdString(const std::string& name) const
    {
        return name + "##" + m_Id;
    }
}
