#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
    typedef struct QS_EditorAPI QS_EditorAPI;

    typedef struct QS_Buffer {
        void* Data = nullptr;
        uint64_t Size = 0;
    } QS_Buffer;

    typedef struct QS_ShaderCompileDesc {
        const char* sourcePath;
        const char* entryPoint;
        const char* stage;
        const char* defines;
    } QS_ShaderCompileDesc;

    typedef enum QS_ShaderOutputType
    {
        QS_SHADER_OUTPUT_VERTEX,
        QS_SHADER_OUTPUT_FRAGMENT,
        //QS_SHADER_OUTPUT_COMPUTE
        QS_SHADER_OUTPUT_COUNT
    } QS_ShaderOutputType;

    typedef struct QS_ShaderOutputArray {
        QS_Buffer Outputs[QS_SHADER_OUTPUT_COUNT];
        uint32_t Count = 0;
    } QS_ShaderOutputArray;

    typedef struct QS_ShaderCompiler {
        QS_ShaderOutputArray (*Compile)(const QS_ShaderCompileDesc*);
        void (*FreeBuffer)(void*);
    } QS_ShaderCompiler;

    struct QS_EditorAPI {
        uint32_t Version;

        // Renderer
        void (*RegisterShaderCompiler)(const char* rendererName, QS_ShaderCompiler compiler);
    };
#ifdef __cplusplus
}
#endif