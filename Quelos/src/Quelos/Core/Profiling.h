#pragma once

#ifdef QS_ENABLE_PROFILING
#include "tracy/Tracy.hpp"

inline void* tracy_malloc_impl(size_t size) {
    void* p = malloc(size);
    TracyAlloc(p, size);
    return p;
}

inline void* tracy_calloc_impl(size_t n, size_t s) {
    void* p = calloc(n, s);
    TracyAlloc(p, n * s);
    return p;
}

inline void* tracy_realloc_impl(void* p, size_t s) {
    TracyFree(p);
    void* r = realloc(p, s);
    TracyAlloc(r, s);
    return r;
}

inline void tracy_free_impl(void* p) {
    TracyFree(p);
    free(p);
}

#define malloc(s)    tracy_malloc_impl(s)
#define calloc(n,s)  tracy_calloc_impl(n, s)
#define realloc(p,s) tracy_realloc_impl(p, s)
#define free(p)      tracy_free_impl(p)

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
