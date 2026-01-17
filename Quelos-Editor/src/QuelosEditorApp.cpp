#include <iostream>

#include "EntryPoint.h"
#include "Quelos/Core/Window.h"
#include <Quelos/Core/Application.h>
#include <Quelos/Core/Ref.h>
#include <Quelos/Core/Layer.h>

#include <EditorLayer.h>

class QuelosEditorApp : public Quelos::Application {
public:
	QuelosEditorApp(Quelos::ApplicationSpecification appSpecs)
		: Application(appSpecs) {

	}
};

Quelos::Application* Quelos::CreateApplication(int argc, char** argv) {
	Quelos::ApplicationSpecification specs;
	specs.Name = "Quelos-Editor";
	specs.Executable = argv[0];
	specs.WindowSpec.Width = 1920;
	specs.WindowSpec.Height = 1080;
	specs.WindowSpec.Title = "Quelos Editor";

	auto app = new QuelosEditorApp(specs);
	app->PushLayer<EditorLayer>();

	return app;
}


