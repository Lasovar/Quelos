include "Dependencies.lua"

workspace "Quelos"
  configurations { "Debug", "Release" }
  architecture "x64"
  startproject "Quelos-Editor"
  cppdialect "C++20"
  defines { "BX_PLATFORM_WINDOWS", "BX_CONFIG_DEBUG" }
  buildoptions { "/Zc:preprocessor", "/Zc:__cplusplus", "/utf-8" }


outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies/bgfx"
include "Quelos/vendor/bx"
include "Quelos/vendor/bimg"
include "Quelos/vendor/bgfx"

group "Dependencies"
include "Quelos/vendor/GLFW"
include "Quelos/vendor/imgui"
include "Quelos/vendor/flecs"

group ""
include "Quelos"
include "Quelos-Editor"
