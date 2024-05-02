workspace "luaFSM"
    architecture "x64"

    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["imgui"] = "3rd/imgui"

include "3rd/imgui_premake.lua"

project "luaFSM"
    location "luaFSM"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "pch.h"
    pchsource "src/tpch.cpp"

    files
    {
        "src/**.h",
        "src/**.cpp",
    }

    includedirs
    {
        "%{prj.name}/src",
        "%{IncludeDir.imgui}",
    }

    links
    {
        "imgui",
    }
    filter "configurations:Debug"
        defines "LUAFSM_DEBUG"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        defines "LUAFSM_RELEASE"
        runtime "Release"
        optimize "on"

    filter "configurations:Dist"
        defines "LUAFSM_DIST"
        runtime "Release"
        optimize "on"

    filter {"system:windows", "configurations:Debug"}
        defines "THRYLOS_DEBUG"
        runtime "Debug"
        symbols "On"

    filter {"system:windows", "configurations:Release"}
        defines "THRYLOS_RELEASE"
        runtime "Release"
        optimize "On"

    filter {"system:windows", "configurations:Dist"}
        defines "THRYLOS_DIST"
        runtime "Release"
        optimize "On"



