#include "qspch.h"
#include "Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#define QS_HAS_CONSOLE 1

namespace Quelos {
	Ref<spdlog::logger> Log::s_CoreLogger;
	Ref<spdlog::logger> Log::s_ClientLogger;
	HashMap<std::string, Log::TagDetails> Log::s_EnabledTags;
	Ref<spdlog::logger> Log::s_EditorConsoleLogger;

	HashMap<std::string, Log::TagDetails> Log::s_DefaultTagDetails = {
		{ "Core",              TagDetails{  true, Level::Trace } },
		{ "GLFW",              TagDetails{  true, Level::Error } },
		{ "Memory",            TagDetails{  true, Level::Error } },
		{ "Mesh",              TagDetails{  true, Level::Warn  } },
		{ "Physics",           TagDetails{  true, Level::Warn  } },
		{ "Project",           TagDetails{  true, Level::Warn  } },
		{ "AssetManager",      TagDetails{  true, Level::Warn  } },
		{ "Renderer",          TagDetails{  true, Level::Info  } },
		{ "Scene",             TagDetails{  true, Level::Info  } },
		{ "Scripting",         TagDetails{  true, Level::Warn  } },
		{ "Timer",             TagDetails{ false, Level::Trace } },
	};

	void Log::Init(const std::string& appName) {
		std::string logsDirectory = "logs";
		if (!std::filesystem::exists(logsDirectory))
			std::filesystem::create_directories(logsDirectory);

		SmallVec<spdlog::sink_ptr, 2> quelosSinks = {
			std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/Quelos.log", true),
		};

#if QS_HAS_CONSOLE
		const auto quelosColorSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		quelosColorSink->set_color_mode(spdlog::color_mode::automatic);
		quelosColorSink->set_pattern("%^[%T] %n: %v%$");

		quelosSinks.push_back(quelosColorSink);
#endif

		SmallVec<spdlog::sink_ptr, 2> appSinks = {
			std::make_shared<spdlog::sinks::basic_file_sink_mt>(std::format("logs/{}.log", appName), true),
		};

#if QS_HAS_CONSOLE
		const auto appColorSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		appColorSink->set_color_mode(spdlog::color_mode::automatic);
		appColorSink->set_pattern("%^[%T] %n: %v%$");

		appSinks.push_back(appColorSink);
#endif

		
		SmallVec<spdlog::sink_ptr, 2> editorConsoleSinks = {
			std::make_shared<spdlog::sinks::basic_file_sink_mt>(std::format("logs/{}.log", appName), true),
		};

#if QS_HAS_CONSOLE
		const auto editorColorSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		editorColorSink->set_color_mode(spdlog::color_mode::automatic);

		editorConsoleSinks.push_back(editorColorSink);
#endif

		quelosSinks[0]->set_pattern("[%T] [%l] %n: %v");
		appSinks[0]->set_pattern("[%T] [%l] %n: %v");

#if QS_HAS_CONSOLE
		for (const auto& sink : editorConsoleSinks) {
			sink->set_pattern("%^%v%$");
		}
#endif

		s_CoreLogger = std::make_shared<spdlog::logger>("Quelos", quelosSinks.begin(), quelosSinks.end());
		s_CoreLogger->set_level(spdlog::level::trace);

		s_ClientLogger = std::make_shared<spdlog::logger>(appName, appSinks.begin(), appSinks.end());
		s_ClientLogger->set_level(spdlog::level::trace);

		s_EditorConsoleLogger = std::make_shared<spdlog::logger>("Console", editorConsoleSinks.begin(), editorConsoleSinks.end());
		s_EditorConsoleLogger->set_level(spdlog::level::trace);

		SetDefaultTagSettings();
	}

	void Log::Shutdown() {
		//s_EditorConsoleLogger.reset();
		s_ClientLogger.reset();
		s_CoreLogger.reset();
		spdlog::drop_all();
	}

	void Log::SetDefaultTagSettings() {
		s_EnabledTags = s_DefaultTagDetails;
	}
}

