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

#define QS_PROFILE_SCOPED() ZoneScoped
#define QS_PROFILE_SCOPED_N(name) ZoneScopedN(name)
#define QS_PROFILE_SCOPED_C(color) ZoneScopedC(color)
#define QS_PROFILE_SCOPED_NC(name, color) ZoneScopedNC(name, color)
#define QS_PROFILE_FRAME() FrameMark
#define QS_PROFILE_FRAME_NAMED(name) FrameMarkNamed(name)
#define QS_PROFILE_FRAME_START(name) FrameMarkStart(name)
#define QS_PROFILE_FRAME_END(name) FrameMarkEnd(name)
#define QS_PROFILE_ALLOC(pointer, size) TracyAlloc(pointer, size)
#define QS_PROFILE_ALLOC_N(pointer, size, name) TracyAllocN(pointer, size, name)
#define QS_PROFILE_FREE(pointer) TracyFree(pointer)
#define QS_PROFILE_FREE_N(pointer, name) TracyFreeN(pointer, name)
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
