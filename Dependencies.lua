VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}

IncludeDir["GLFW"] = "%{wks.location}/Quelos/vendor/glfw/include"
IncludeDir["glm"] = "%{wks.location}/Quelos/vendor/glm"
IncludeDir["ImGui"] = "%{wks.location}/Quelos/vendor/imgui"
IncludeDir["bgfx"] = "%{wks.location}/Quelos/vendor/bgfx/include"
IncludeDir["bx"] = "%{wks.location}/Quelos/vendor/bx/include"
IncludeDir["bimg"] = "%{wks.location}/Quelos/vendor/bimg/include"
IncludeDir["spdlog"] = "%{wks.location}/Quelos/vendor/spdlog"
IncludeDir["flecs"] = "%{wks.location}/Quelos/vendor/flecs/distr"
