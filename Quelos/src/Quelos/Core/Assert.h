#pragma once

#include "Log.h"

// Credits Hazel

#ifdef QS_PLATFORM_WINDOWS
	#define QS_DEBUG_BREAK __debugbreak()
#elif defined(QS_COMPILER_CLANG)
	#define QS_DEBUG_BREAK __builtin_debugtrap()
#else
	#define QS_DEBUG_BREAK
#endif

#ifdef QS_DEBUG
	#define QS_ENABLE_ASSERTS
#endif

#define QS_ENABLE_VERIFY
#define QS_ENABLE_ENSURE

#ifdef QS_ENABLE_ASSERTS
	#ifdef QS_COMPILER_CLANG
		#define QS_CORE_ASSERT_MESSAGE_INTERNAL(...)  ::Quelos::Log::PrintAssertMessage(::Quelos::Log::Type::Core, "Assertion Failed (" __FILE__ ":" QS_STRINGIFY(__LINE__) ") ", ##__VA_ARGS__)
		#define QS_ASSERT_MESSAGE_INTERNAL(...)  ::Quelos::Log::PrintAssertMessage(::Quelos::Log::Type::Client, "Assertion Failed (" __FILE__ ":" QS_STRINGIFY(__LINE__) ") ", ##__VA_ARGS__)
	#else
		#define QS_CORE_ASSERT_MESSAGE_INTERNAL(...)  ::Quelos::Log::PrintAssertMessage(::Quelos::Log::Type::Core, "Assertion Failed (" __FILE__ ":" QS_STRINGIFY(__LINE__) ") " __VA_OPT__(,) __VA_ARGS__)
		#define QS_ASSERT_MESSAGE_INTERNAL(...)  ::Quelos::Log::PrintAssertMessage(::Quelos::Log::Type::Client, "Assertion Failed (" __FILE__ ":" QS_STRINGIFY(__LINE__) ") " __VA_OPT__(,) __VA_ARGS__)
	#endif

	#define QS_CORE_ASSERT(condition, ...) do { if(!(condition)) { QS_CORE_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); QS_DEBUG_BREAK; } } while(0)
	#define QS_ASSERT(condition, ...) do { if(!(condition)) { QS_ASSERT_MESSAGE_INTERNAL(__VA_ARGS__); QS_DEBUG_BREAK; } } while(0)
#else
	#define QS_CORE_ASSERT(condition, ...) ((void) (condition))
	#define QS_ASSERT(condition, ...) ((void) (condition))
#endif

#ifdef QS_ENABLE_VERIFY
	#ifdef QS_COMPILER_CLANG
		#define QS_CORE_VERIFY_MESSAGE_INTERNAL(...)  ::Quelos::Log::PrintAssertMessage(::Quelos::Log::Type::Core, "Verify Failed (" __FILE__ ":" QS_STRINGIFY(__LINE__) ") ", ##__VA_ARGS__)
		#define QS_VERIFY_MESSAGE_INTERNAL(...)  ::Quelos::Log::PrintAssertMessage(::Quelos::Log::Type::Client, "Verify Failed (" __FILE__ ":" QS_STRINGIFY(__LINE__) ") ", ##__VA_ARGS__)
	#else
		#define QS_CORE_VERIFY_MESSAGE_INTERNAL(...)  ::Quelos::Log::PrintAssertMessage(::Quelos::Log::Type::Core, "Verify Failed (" __FILE__ ":" QS_STRINGIFY(__LINE__) ") " __VA_OPT__(,) __VA_ARGS__)
		#define QS_VERIFY_MESSAGE_INTERNAL(...)  ::Quelos::Log::PrintAssertMessage(::Quelos::Log::Type::Client, "Verify Failed (" __FILE__ ":" QS_STRINGIFY(__LINE__) ") " __VA_OPT__(,) __VA_ARGS__)
	#endif

	#define QS_CORE_VERIFY(condition, ...) do { if(!(condition)) { QS_CORE_VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); QS_DEBUG_BREAK; } } while(0)
	#define QS_VERIFY(condition, ...) do { if(!(condition)) { QS_VERIFY_MESSAGE_INTERNAL(__VA_ARGS__); QS_DEBUG_BREAK; } } while(0)
#else
	#define QS_CORE_VERIFY(condition, ...) ((void) (condition))
	#define QS_VERIFY(condition, ...) ((void) (condition))
#endif

#ifdef QS_ENABLE_ENSURE
	#ifdef QS_COMPILER_CLANG
		#define QS_CORE_ENSURE_MESSAGE_INTERNAL(...)  ::Quelos::Log::PrintAssertMessage(::Quelos::Log::Type::Core, "Ensure Failed", ##__VA_ARGS__)
		#define QS_ENSURE_MESSAGE_INTERNAL(...)  ::Quelos::Log::PrintAssertMessage(::Quelos::Log::Type::Client, "Ensure Failed", ##__VA_ARGS__)
	#else
		#define QS_CORE_ENSURE_MESSAGE_INTERNAL(...)  ::Quelos::Log::PrintAssertMessage(::Quelos::Log::Type::Core, "Ensure Failed" __VA_OPT__(,) __VA_ARGS__)
		#define QS_ENSURE_MESSAGE_INTERNAL(...)  ::Quelos::Log::PrintAssertMessage(::Quelos::Log::Type::Client, "Ensure Failed" __VA_OPT__(,) __VA_ARGS__)
	#endif

	#define QS_CORE_ENSURE(condition, ...) [&]{ if(!(condition)) { QS_CORE_ENSURE_MESSAGE_INTERNAL(__VA_ARGS__); QS_DEBUG_BREAK; } return (condition); }()
	#define QS_ENSURE(condition, ...) [&]{ if(!(condition)) { QS_ENSURE_MESSAGE_INTERNAL(__VA_ARGS__); QS_DEBUG_BREAK; } return (condition) }()

#else
	#define QS_CORE_ENSURE(condition, ...) (condition)
	#define QS_ENSURE(condition, ...) (condition)
#endif
