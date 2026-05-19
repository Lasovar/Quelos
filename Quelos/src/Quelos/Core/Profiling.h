#pragma once

#ifdef QS_ENABLE_PROFILING
#ifndef TRACY_ENABLE
#define TRACY_ENABLE
#endif
#include "tracy/Tracy.hpp"
#define QS_PROFILE_SCOPED() ZoneScoped
#define QS_PROFILE_SCOPED_N(name) ZoneScopedN(name)
#define QS_PROFILE_SCOPED_C(color) ZoneScopedC(color)
#define QS_PROFILE_SCOPED_NC(name, color) ZoneScopedNC(name, color)
#define QS_PROFILE_FRAME() FrameMark
#define QS_PROFILE_FRAME_NAMED(name) FrameMarkNamed(name)
#define QS_PROFILE_FRAME_START(name) FrameMarkStart(name)
#define QS_PROFILE_FRAME_END(name) FrameMarkEnd(name)
#else
#define QS_PROFILE_SCOPED()
#define QS_PROFILE_SCOPED_N(name)
#define QS_PROFILE_SCOPED_C(color)
#define QS_PROFILE_SCOPED_NC(name, color)
#define QS_PROFILE_FRAME()
#define QS_PROFILE_FRAME_NAMED(name)
#define QS_PROFILE_FRAME_START(name)
#define QS_PROFILE_FRAME_END(name)
#endif
