#include <iostream>

#include "EntryPoint.h"
#include "Quelos/Core/Window.h"
#include <Quelos/Core/Application.h>
#include <Quelos/Core/Ref.h>
#include <Quelos/Core/Layer.h>
#include <print>

#include <EditorLayer.h>

#include "Quelos/Plugin/PluginAPI.h"
#include "QuelosEditor/EditorAPI.h"

class QuelosEditorApp : public Quelos::Application {
public:
	QuelosEditorApp(Quelos::ApplicationSpecification appSpecs)
		: Application(appSpecs) {

	}
};

QS_EditorAPI g_EditorAPI;

/*
extern "C" void RegisterBgfxRendererPlugin(Quelos::PluginContext& pluginContext);
extern "C" void RegisterBgfxEditorPlugin(QS_EditorAPI* editorApi);
*/

extern "C" void RegisterDiligentEnginePlugin(Quelos::PluginContext& pluginContext);

Quelos::Application* Quelos::CreateApplication(int argc, char** argv) {
	ApplicationSpecification specs;
	specs.Name = "Quelos-Editor";
	specs.ApplicationPath = std::filesystem::absolute(argv[0]).parent_path();
	specs.WindowSpec.Width = 1600;
	specs.WindowSpec.Height = 900;
	specs.WindowSpec.Title = "Quelos Editor";

#if QUELOS_USE_SDL
	specs.WindowSpec.Backed = Quelos::WindowingBackend::SDL;
#elif QUELOS_USE_GLFW
	specs.WindowSpec.Backed = Quelos::WindowingBackend::GLFW;
#endif

#if QS_PLATFORM_WINDOWS
	specs.RendererAPI = RendererAPI::Vulkan;
#elif QS_PLATFORM_LINUX
	specs.RendererAPI = RendererAPI::Vulkan;
#elif QS_PLATFORM_MACOS
	specs.RendererAPI = RendererAPI::Metal;
#endif

	PluginContext context;
	RegisterDiligentEnginePlugin(context);
	//RegisterBgfxRendererPlugin(context);
	g_EditorAPI.RegisterShaderCompiler = QuelosEditor::EditorLayer::RegisterShaderCompiler;
	//RegisterBgfxEditorPlugin(&g_EditorAPI);

	const auto app = new QuelosEditorApp(specs);
	const Ref<QuelosEditor::EditorLayer> editorLayer = app->PushLayer<QuelosEditor::EditorLayer>();

	return app;
}
