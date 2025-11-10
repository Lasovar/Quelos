#include <Quelos/Core/Application.h>

extern Quelos::Application* Quelos::CreateApplication(int argc, char** argv);

namespace Quelos {
	int Main(int argc, char** argv)
	{
		Application* app = CreateApplication(argc, argv);
		app->Run();
		delete app;
		return 0;
	}
}

int main(int argc, char** argv)
{
	return Quelos::Main(argc, argv);
}

