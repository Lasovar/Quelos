#include "ShaderImporter.h"

#include "EditorLayer.h"
#include "Quelos/Utility/FileSystem.h"

namespace QuelosEditor {
    namespace ShaderImporter {
        static std::string GetMetadataPath(std::string_view assetPath) {
            return fmt::format(
                "{}/{}/{}.qmeta",
                Project::GetProjectPath().generic_string(),
                assetPath,
                FS::Stem(assetPath)
            );
        }

        static bool SerializeHandle(const OsPath& path, AssetHandle handle) {
            using namespace Serialization;

            std::ofstream file(path, std::ios::binary);
            if (!file) {
                QS_CORE_ERROR_TAG(
                    "ShaderImporter::SerializeHandle",
                    "Failed to serialize shader handle at {}",
                    path.generic_string()
                );

                return false;
            }

            std::string buffer;
            StringQuelWriter writer(buffer);
            writer.Write(SectionEvent{"Shader"});
            writer.CloseSection();
            writer.WriteField("handle", UnquotedString{handle.ToFormattedString()});

            file.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            return true;
        }

        static AssetHandle DeserializeHandle(const OsPath& path) {
            using namespace Serialization;

            if (!std::filesystem::exists(path)) {
                return {};
            }

            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file) {
                QS_CORE_ERROR_TAG(
                    "ShaderImporter::DeserializeHandle",
                    "Failed to open metadata file '{}'",
                    path.generic_string()
                );

                return {};
            }

            const std::streamsize fileSize = file.tellg();
            file.seekg(0);

            std::string assetRegistryBuffer;
            assetRegistryBuffer.resize(fileSize);
            file.read(assetRegistryBuffer.data(), fileSize);

            QuelReader reader(assetRegistryBuffer);
            std::string_view currentSection;
            std::string_view currentField;

            AssetHandle handle;
            for (auto&& parserEvent : reader.Parse()) {
                std::visit(Overloaded{
                               [&](const SectionEvent& event) {
                                   currentSection = event.Name;
                               },
                               [&](const FieldEvent& event) {
                                   currentField = event.Path;
                               },
                               [&](const ValueEvent& event) {
                                   if (currentSection == "Shader" && currentField == "handle") {
                                       handle = AssetHandle(std::get<std::string_view>(event.Value));
                                   }
                               },
                               [](auto e) {
                               }
                           }, parserEvent);
            }

            return handle;
        }

        bool CompileShader(const std::string& path, Buffer& vertexBuffer, Buffer& fragmentBuffer) {
            const QS_ShaderCompiler compiler = EditorLayer::GetShaderCompiler();
            QS_ShaderCompileDesc compileDesc;
            compileDesc.sourcePath = path.c_str();
            const QS_ShaderOutputArray shaderArray = compiler.Compile(&compileDesc);

            const QS_Buffer vertexOutput = shaderArray.Outputs[QS_SHADER_OUTPUT_VERTEX];
            const QS_Buffer fragmentOutput = shaderArray.Outputs[QS_SHADER_OUTPUT_FRAGMENT];

            if (shaderArray.Count < 2 || !vertexOutput.Data || !fragmentOutput.Data) {
                return false;
            }

            vertexBuffer = Buffer::Adopt(vertexOutput.Data, vertexOutput.Size, compiler.FreeBuffer);
            fragmentBuffer = Buffer::Adopt(fragmentOutput.Data, fragmentOutput.Size, compiler.FreeBuffer);

            return true;
        }

        Ref<Shader> Import(const AssetHandle handle, const AssetMetadata& metadata) {
            Buffer vertexBuffer, fragmentBuffer;
            CompileShader(metadata.FilePath, vertexBuffer, fragmentBuffer);
            Ref<Shader> shader = CreateRef<Shader>(
                std::move(vertexBuffer),
                std::move(fragmentBuffer),
                std::string(FS::Filename(metadata.FilePath))
            );

            shader->SetAssetHandle(handle);

            return shader;
        }

        bool IsAssetSupported(const std::string_view assetPath) {
            std::string_view extension = FS::Extension(assetPath);
            return extension == ".qshader";
        }

        void RecompileShader(const Ref<Shader>& shader) {
            if (!shader || !shader->GetAssetHandle()) {
                QS_ERROR_TAG(
                    "ShaderImporter::RecompileShader",
                    "Failed to recompile shader: invalid shader asset!"
                );

                return;
            }

            const Ref<EditorAssetManager> assetManager = RefAs<EditorAssetManager>(Project::GetAssetManager());
            if (!assetManager) {
                QS_ERROR_TAG(
                    "ShaderImporter::RecompileShader",
                    "Failed to recompile shader '{}': couldn't retrieve EditorAssetManager",
                    shader->GetName()
                );

                return;
            }

            const AssetMetadata* metadata = assetManager->GetAssetMetadata(shader->GetAssetHandle());
            if (!metadata) {
                QS_ERROR_TAG(
                    "ShaderImporter::RecompileShader",
                    "Failed to recompile shader '{}': couldn't retrieve asset metadata for shader!",
                    shader->GetName()
                );

                return;
            }

            Buffer vertexBuffer, fragmentBuffer;
            CompileShader(metadata->FilePath, vertexBuffer, fragmentBuffer);
            shader->Recreate(std::move(vertexBuffer), std::move(fragmentBuffer));
        }


        AssetHandle ReadAssetHandle(const std::string_view assetPath) {
            return DeserializeHandle(GetMetadataPath(assetPath));
        }

        bool WriteAssetHandle(const std::string_view assetPath, const AssetHandle& handle) {
            return SerializeHandle(GetMetadataPath(assetPath), handle);
        }

        bool HandlesAssetType(const AssetType& type, void* userData) {
            return type == Shader::GetStaticType();
        }

        EditorAssetImporterConfig GetImporterConfig() {
            return {
                Shader::GetStaticType(),
                Import,
                IsAssetSupported,
                ReadAssetHandle,
                WriteAssetHandle
            };
        }

        void Initialize() {
            EditorAssetImporter::RegisterAssetImporter(GetImporterConfig());
        }
    }
}
