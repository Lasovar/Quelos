//
// Created by luais on 28/05/2026.
//

#include "Profiling.h"

#ifdef TRACY_ENABLE
void* operator new(size_t s) {
    void* p = malloc(s); TracyAlloc(p, s); return p;
}
void* operator new[](size_t s) {
    void* p = malloc(s); TracyAlloc(p, s); return p;
}
void operator delete(void* p) noexcept { TracyFree(p); free(p); }
void operator delete[](void* p) noexcept { TracyFree(p); free(p); }
#endif
namespace Quelos {

}
