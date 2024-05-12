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
IncludeDir["glfw"] = "3rd/glfw/include"
IncludeDir["glad"] = "3rd/glad/include"
IncludeDir["imguifiledialog"] = "3rd/ImGuiFileDialog"

include "3rd/imgui_premake.lua"
include "3rd/glfw_premake.lua"
include "3rd/glad_premake.lua"

project "luaFSM"
    location "luaFSM"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "on"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "pch.h"
    pchsource "%{prj.name}/src/pch.cpp"

    files
    {
        "%{prj.name}/src/**.h",
        "%{prj.name}/src/**.cpp",
        "src/**.h",
        "src/**.cpp",
    }

    includedirs
    {
        "%{prj.name}/src",
        "%{prj.name}/src",
        "3rd/spdlog/include",
        "%{IncludeDir.imgui}",
        "%{IncludeDir.glad}",
        "%{IncludeDir.imguifiledialog}",
        "%{IncludeDir.glfw}",
    }

    links
    {
        "imgui",
        "GLFW",
        "glad",
    }

    postbuildcommands
    {
        "{COPY} %{prj.location}assets/ %{cfg.buildtarget.directory}/assets/",
    }

    filter "configurations:Debug"
        defines "LUAFSM_DEBUG"
        runtime "Debug"
        symbols "on"
        optimize "off"

    filter "configurations:Release"
        defines "LUAFSM_RELEASE"
        runtime "Release"
        optimize "on"

    filter "configurations:Dist"
        defines "LUAFSM_DIST"
        runtime "Release"
        optimize "on"

    filter {"system:windows", "configurations:Debug"}
        defines "LUAFSM_DEBUG"
        runtime "Debug"
        symbols "On"
        optimize "off"

    filter {"system:windows", "configurations:Release"}
        defines "LUAFSM_RELEASE"
        runtime "Release"
        optimize "On"

    filter {"system:windows", "configurations:Dist"}
        defines "LUAFSM_DIST"
        runtime "Release"
        optimize "On"



