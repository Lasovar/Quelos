#pragma once

#if defined(_WIN32)
    #if defined(QS_LIB_SHARED)
        #if defined(QS_LIB_BUILD)
            #define QS_API __declspec(dllexport)
        #else
            #define QS_API __declspec(dllimport)
        #endif
    #else
        #define QS_API
    #endif
#else
    #if defined(QS_LIB_SHARED)
        #define QS_API __attribute__((visibility("default")))
    #else
        #define QS_API
    #endif
#endif
