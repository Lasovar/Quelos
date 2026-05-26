#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#include "Ref.h"
#include "Quelos/Math/Math.h"

namespace Quelos {
	class QS_API Log {
	public:
		enum class Type : uint8_t {
			Core = 0, Client = 1
		};

		enum class Level : uint8_t {
			Trace = 0, Info, Warn, Error, Critical
		};

		struct TagDetails {
			bool Enabled = true;
			Level LevelFilter = Level::Trace;
		};

	public:
		static void Init(const std::string& appName);

		static void Shutdown();

		static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

		static bool HasTag(const std::string& tag) { return s_EnabledTags.find(tag) != s_EnabledTags.end(); }
		static HashMap<std::string, TagDetails>& EnabledTags() { return s_EnabledTags; }
		static void SetDefaultTagSettings();

		template<typename... Args>
		static void PrintMessage(Log::Type type, Log::Level level, fmt::format_string<Args...> format, Args&&... args);

		template<typename... Args>
		static void PrintMessageTag(Log::Type type, Log::Level level, std::string_view tag, fmt::format_string<Args...> format, Args&&... args);

		static void PrintMessageTag(Log::Type type, Log::Level level, std::string_view tag, std::string_view message);

		template<typename... Args>
		static void PrintAssertMessage(Log::Type type, std::string_view prefix, fmt::format_string<Args...> message, Args&&... args);

		static void PrintAssertMessage(Log::Type type, std::string_view prefix);
		
	public:
		
		static const char* LevelToString(Level level) {
			switch (level) {
				case Level::Trace: return "Trace";
				case Level::Info:  return "Info";
				case Level::Warn:  return "Warn";
				case Level::Error: return "Error";
				case Level::Critical: return "Critical";
			}

			return "";
		}

		static Level LevelFromString(std::string_view string) {
			if (string == "Trace") return Level::Trace;
			if (string == "Info")  return Level::Info;
			if (string == "Warn")  return Level::Warn;
			if (string == "Error") return Level::Error;
			if (string == "Critical") return Level::Critical;

			return Level::Trace;
		}
		
	private:
		static Ref<spdlog::logger> s_CoreLogger;
		static Ref<spdlog::logger> s_ClientLogger;
		static Ref<spdlog::logger> s_EditorConsoleLogger;

		static HashMap<std::string, TagDetails> s_EnabledTags;
		static HashMap<std::string, TagDetails> s_DefaultTagDetails;
	};
}

// CREDITS YOINKED FROM HAZEL!

// Core tag logging
#define QS_CORE_TRACE_TAG(tag, ...) ::Quelos::Log::PrintMessageTag(::Quelos::Log::Type::Core, ::Quelos::Log::Level::Trace, tag, __VA_ARGS__)
#define QS_CORE_INFO_TAG(tag, ...)  ::Quelos::Log::PrintMessageTag(::Quelos::Log::Type::Core, ::Quelos::Log::Level::Info, tag, __VA_ARGS__)
#define QS_CORE_WARN_TAG(tag, ...)  ::Quelos::Log::PrintMessageTag(::Quelos::Log::Type::Core, ::Quelos::Log::Level::Warn, tag, __VA_ARGS__)
#define QS_CORE_ERROR_TAG(tag, ...) ::Quelos::Log::PrintMessageTag(::Quelos::Log::Type::Core, ::Quelos::Log::Level::Error, tag, __VA_ARGS__)
#define QS_CORE_CRITICAL_TAG(tag, ...) ::Quelos::Log::PrintMessageTag(::Quelos::Log::Type::Core, ::Quelos::Log::Level::Critical, tag, __VA_ARGS__)

// Client tag logging
#define QS_TRACE_TAG(tag, ...) ::Quelos::Log::PrintMessageTag(::Quelos::Log::Type::Client, ::Quelos::Log::Level::Trace, tag, __VA_ARGS__)
#define QS_INFO_TAG(tag, ...)  ::Quelos::Log::PrintMessageTag(::Quelos::Log::Type::Client, ::Quelos::Log::Level::Info, tag, __VA_ARGS__)
#define QS_WARN_TAG(tag, ...)  ::Quelos::Log::PrintMessageTag(::Quelos::Log::Type::Client, ::Quelos::Log::Level::Warn, tag, __VA_ARGS__)
#define QS_ERROR_TAG(tag, ...) ::Quelos::Log::PrintMessageTag(::Quelos::Log::Type::Client, ::Quelos::Log::Level::Error, tag, __VA_ARGS__)
#define QS_CRITICAL_TAG(tag, ...) ::Quelos::Log::PrintMessageTag(::Quelos::Log::Type::Client, ::Quelos::Log::Level::Critical, tag, __VA_ARGS__)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Core Logging
#define QS_CORE_TRACE(...)  ::Quelos::Log::PrintMessage(::Quelos::Log::Type::Core, ::Quelos::Log::Level::Trace, __VA_ARGS__)
#define QS_CORE_INFO(...)   ::Quelos::Log::PrintMessage(::Quelos::Log::Type::Core, ::Quelos::Log::Level::Info, __VA_ARGS__)
#define QS_CORE_WARN(...)   ::Quelos::Log::PrintMessage(::Quelos::Log::Type::Core, ::Quelos::Log::Level::Warn, __VA_ARGS__)
#define QS_CORE_ERROR(...)  ::Quelos::Log::PrintMessage(::Quelos::Log::Type::Core, ::Quelos::Log::Level::Error, __VA_ARGS__)
#define QS_CORE_CRITICAL(...)  ::Quelos::Log::PrintMessage(::Quelos::Log::Type::Core, ::Quelos::Log::Level::Critical, __VA_ARGS__)

// Client Logging
#define QS_TRACE(...)   ::Quelos::Log::PrintMessage(::Quelos::Log::Type::Client, ::Quelos::Log::Level::Trace, __VA_ARGS__)
#define QS_INFO(...)    ::Quelos::Log::PrintMessage(::Quelos::Log::Type::Client, ::Quelos::Log::Level::Info, __VA_ARGS__)
#define QS_WARN(...)    ::Quelos::Log::PrintMessage(::Quelos::Log::Type::Client, ::Quelos::Log::Level::Warn, __VA_ARGS__)
#define QS_ERROR(...)   ::Quelos::Log::PrintMessage(::Quelos::Log::Type::Client, ::Quelos::Log::Level::Error, __VA_ARGS__)
#define QS_CRITICAL(...)   ::Quelos::Log::PrintMessage(::Quelos::Log::Type::Client, ::Quelos::Log::Level::Critical, __VA_ARGS__)

namespace Quelos {
	template<typename... Args>
	void Log::PrintMessage(Log::Type type, Log::Level level, fmt::format_string<Args...> format, Args&&... args) {
		auto detail = s_EnabledTags[""];
		if (detail.Enabled && detail.LevelFilter <= level) {
			auto logger = (type == Type::Core) ? GetCoreLogger() : GetClientLogger();

			// Pre-format the message with the provided format and arguments before calling the logger.
			// This allows the code to compile on a wider range of compilers (notably, Clang with libstdc++ on Linux)
			std::string formatted = fmt::format(format, std::forward<Args>(args)...);
			switch (level)
			{
			case Level::Trace:
				logger->trace(formatted);
				break;
			case Level::Info:
				logger->info(formatted);
				break;
			case Level::Warn:
				logger->warn(formatted);
				break;
			case Level::Error:
				logger->error(formatted);
				break;
			case Level::Critical:
				logger->critical(formatted);
				break;
			}
		}
	}

	template<typename... Args>
	void Log::PrintMessageTag(Log::Type type, Log::Level level, std::string_view tag, const fmt::format_string<Args...> format, Args&&... args) {
		auto detail = s_EnabledTags[std::string(tag)];
		if (detail.Enabled && detail.LevelFilter <= level) {
			auto logger = (type == Type::Core) ? GetCoreLogger() : GetClientLogger();
			std::string formatted = fmt::format(format, std::forward<Args>(args)...);
			switch (level)
			{
				case Level::Trace:
					logger->trace("[{0}] {1}", tag, formatted);
					break;
				case Level::Info:
					logger->info("[{0}] {1}", tag, formatted);
					break;
				case Level::Warn:
					logger->warn("[{0}] {1}", tag, formatted);
					break;
				case Level::Error:
					logger->error("[{0}] {1}", tag, formatted);
					break;
				case Level::Critical:
					logger->critical("[{0}] {1}", tag, formatted);
					break;
			}
		}
	}

	inline void Log::PrintMessageTag(Log::Type type, Log::Level level, std::string_view tag, std::string_view message) {
		auto detail = s_EnabledTags[std::string(tag)];
		if (detail.Enabled && detail.LevelFilter <= level) {
			auto logger = (type == Type::Core) ? GetCoreLogger() : GetClientLogger();
			switch (level)
			{
				case Level::Trace:
					logger->trace("[{0}] {1}", tag, message);
					break;
				case Level::Info:
					logger->info("[{0}] {1}", tag, message);
					break;
				case Level::Warn:
					logger->warn("[{0}] {1}", tag, message);
					break;
				case Level::Error:
					logger->error("[{0}] {1}", tag, message);
					break;
				case Level::Critical:
					logger->critical("[{0}] {1}", tag, message);
					break;
			}
		}
	}

	template<typename... Args>
	void Log::PrintAssertMessage(Log::Type type, std::string_view prefix, fmt::format_string<Args...> message, Args&&... args) {
		auto logger = (type == Type::Core) ? GetCoreLogger() : GetClientLogger();
		auto formatted = fmt::format(message, std::forward<Args>(args)...);
		logger->error("{0}: {1}", prefix, formatted);

#if QS_ASSERT_MESSAGE_BOX
		MessageBoxA(nullptr, formatted.data(), "Quelos Assert", MB_OK | MB_ICONERROR);
#endif
	}

	inline void Log::PrintAssertMessage(Log::Type type, std::string_view prefix) {
		auto logger = (type == Type::Core) ? GetCoreLogger() : GetClientLogger();
		logger->error("{0}", prefix);

#if QS_ASSERT_MESSAGE_BOX
		MessageBoxA(nullptr, "No message :(", "Quelos Assert", MB_OK | MB_ICONERROR);
#endif
	}
}

template<>
struct fmt::formatter<hlslpp::float2> {
	constexpr auto parse(fmt::format_parse_context& ctx) {
		return ctx.begin();
	}

	template<typename FormatContext>
	auto format(const hlslpp::float2& v, FormatContext& ctx) const {
		return fmt::format_to(ctx.out(), "({}, {})", v[0], v[1]);
	}
};

template <>
struct fmt::formatter<Quelos::float3> {
	constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const Quelos::float3& v, FormatContext& ctx) {
		return fmt::format_to(ctx.out(), "({},{},{})", v[0], v[1], v[2]);
	}
};

template <>
struct fmt::formatter<Quelos::float4> {
	constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const Quelos::float4& v, FormatContext& ctx) {
		return fmt::format_to(ctx.out(), "({},{},{},{})", v[0], v[1], v[2], v[3]);
	}
};

template <>
struct fmt::formatter<Quelos::quaternion> {
	constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const Quelos::quaternion& v, FormatContext& ctx) {
		return fmt::format_to(ctx.out(), "({},{},{},{})", v.f32[0], v.f32[1], v.f32[2], v.f32[3]);
	}
};


template <>
struct fmt::formatter<Quelos::float4x4> {
	constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }

	template <typename FormatContext>
	auto format(const Quelos::float4x4& m, FormatContext& ctx) {
		return fmt::format_to(ctx.out(), "{}",
			"\n\t[[{0.x}, {0.y}, {0.z}, {0.w}],\n"
			"\t [{1.x}, {1.y}, {1.z}, {1.w}],\n"
			"\t [{2.x}, {2.y}, {2.z}, {2.w}],\n"
			"\t [{3.x}, {3.y}, {3.z}, {3.w}]]",
			m[0], m[1], m[2], m[3]);
	}
};


