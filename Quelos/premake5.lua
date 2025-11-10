project "Quelos"
  kind "StaticLib"
  language "C++"
  cppdialect "C++20"
  pchheader "qspch.h"
  pchsource "src/qspch.cpp"

  defines { "GLM_ENABLE_EXPERIMENTAL"  }

  targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
  objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

  files { 
    "src/**.cpp",
    "src/**.h",
  }
  includedirs { 
      "src",
      "%{IncludeDir.ImGui}",
      "%{IncludeDir.glm}",
      "%{IncludeDir.GLFW}",
      "%{IncludeDir.bgfx}",
      "%{IncludeDir.bx}",
      "%{IncludeDir.bimg}",
      "%{IncludeDir.spdlog}",
      "%{IncludeDir.flecs}",
  }
  links { "bgfx", "bimg", "bx", "GLFW", "ImGUI", "flecs" }