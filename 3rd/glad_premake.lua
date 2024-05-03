project "glad"
	kind "StaticLib"
	language "C"
    staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"glad/include/glad/glad.h",
		"glad/include/KHR/khrplatform.h",
		"glad/src/glad.c",
	}

	includedirs
	{
		"glad/include"
	}
	
	filter "system:windows"
		systemversion "latest"
		staticruntime "On"
