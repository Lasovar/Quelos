project "Quelos-Editor"
  kind "ConsoleApp"
  language "C++"
  targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
  objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")
  files { "src/**.cpp", "src/**.h" }
  includedirs { 
    "src",
    "../Quelos/src/", 
    "%{IncludeDir.ImGui}",
    "%{IncludeDir.flecs}",
    "%{IncludeDir.glm}",
    "%{IncludeDir.spdlog}",
  }
  dependson { "Quelos" }
  links { "Quelos" }