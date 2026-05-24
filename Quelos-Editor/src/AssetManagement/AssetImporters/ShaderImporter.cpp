#include "ShaderImporter.h"

#include "EditorLayer.h"

#include "Quelos/Utility/FileSystem.h"
#include "Quelos/Utility/QuelosUtil.h"
#include "slang.h"
#include "slang-com-helper.h"
#include "slang-com-ptr.h"

namespace QuelosEditor {
    namespace ShaderImporter {
        static Slang::ComPtr<slang::IGlobalSession> s_GlobalSession;

        static std::string GetMetadataPath(std::string_view assetPath) {
            return fmt::format(
                "{}/{}.qmeta",
                Project::GetProjectPath().generic_string(),
                assetPath
            );
        }

        static bool SerializeHandle(const OsPath& path, AssetID handle) {
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

        static AssetID DeserializeHandle(const OsPath& path) {
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

            AssetID handle;
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
                                       handle = AssetID(std::get<std::string_view>(event.Value));
                                   }
                               },
                               []([[maybe_unused]] auto& e) {}
                           },
                           parserEvent);
            }

            return handle;
        }

        bool CompileShader(
            const std::string& shaderPath, const AssetID handle, Buffer& vertexBuffer, Buffer& fragmentBuffer
        ) {

            slang::SessionDesc sessionDesc;

            slang::TargetDesc targetDesc;
            targetDesc.format = SLANG_SPIRV;
            targetDesc.profile = s_GlobalSession->findProfile("spirv_1_5");
            slang::CompilerOptionEntry opts[] = {
                {slang::CompilerOptionName::VulkanUseEntryPointName, {slang::CompilerOptionValueKind::Int, 1}},
                {slang::CompilerOptionName::EmitSpirvDirectly, {slang::CompilerOptionValueKind::Int, 1}},
            };
            targetDesc.compilerOptionEntries = opts;
            targetDesc.compilerOptionEntryCount = 2;

            sessionDesc.targets = &targetDesc;
            sessionDesc.targetCount = 1;

            std::string parentPath = (Project::GetProjectPath() / shaderPath).parent_path().generic_string();
            const char* searchPaths[] = { parentPath.c_str() };
            sessionDesc.searchPaths = searchPaths;
            sessionDesc.searchPathCount = 1;

            Slang::ComPtr<slang::ISession> session;
            s_GlobalSession->createSession(sessionDesc, session.writeRef());

            std::string moduleName(FS::Stem(shaderPath));
            Slang::ComPtr<slang::IBlob> diagnostics;
            slang::IModule* module = session->loadModule(moduleName.c_str(), diagnostics.writeRef());
            if (diagnostics) {
                QS_CORE_TRACE_TAG("SlangDiagnostic", "{}", static_cast<const char*>(diagnostics->getBufferPointer()));
                diagnostics = nullptr;
            }

            Slang::ComPtr<slang::IEntryPoint> vertexEntryPoint;
            Slang::ComPtr<slang::IEntryPoint> fragmentEntryPoint;
            module->findEntryPointByName("vertexMain", vertexEntryPoint.writeRef());
            module->findEntryPointByName("fragmentMain", fragmentEntryPoint.writeRef());

            if (!vertexEntryPoint) {
                QS_CORE_ERROR_TAG("Slang", "vertexMain not found");
                return false;
            }

            if (!fragmentEntryPoint) {
                QS_CORE_ERROR_TAG("Slang", "fragmentMain not found");
                return false;
            }

            slang::IComponentType* components[] = {module, vertexEntryPoint, fragmentEntryPoint};
            Slang::ComPtr<slang::IComponentType> program;
            session->createCompositeComponentType(components, 3, program.writeRef());

            slang::ProgramLayout* moduleLayout = module->getLayout();
            slang::ProgramLayout* entryLayout = vertexEntryPoint->getLayout();

            QS_CORE_INFO_TAG("SlangModule", "Module name: {}", module->getName());
            for (uint32_t i = 0; i < moduleLayout->getParameterCount(); i++) {
                slang::VariableLayoutReflection* parameter = moduleLayout->getParameterByIndex(i);
                QS_CORE_TRACE_TAG(
                    "SlangModuleParameter",
                    "{}-{}",
                    parameter->getName() ? parameter->getName() : "",
                    parameter->getSemanticName() ? parameter->getSemanticName() : ""
                );
            }

            for (uint32_t i = 0; i < entryLayout->getParameterCount(); i++) {
                slang::VariableLayoutReflection* parameter = entryLayout->getParameterByIndex(i);
                QS_CORE_TRACE_TAG(
                    "SlangEntryParameter",
                    "{}-{}",
                    parameter->getName() ? parameter->getName() : "",
                    parameter->getSemanticName() ? parameter->getSemanticName() : ""
                );
            }

            Slang::ComPtr<slang::IComponentType> linkedProgram;
            program->link(linkedProgram.writeRef(), diagnostics.writeRef());

            if (diagnostics) {
                QS_CORE_ERROR_TAG(
                    "SlangDiagnostic",
                    "{}",
                    static_cast<const char*>(diagnostics->getBufferPointer())
                );

                diagnostics = nullptr;
            }

            const int targetIndex = 0;

            Slang::ComPtr<slang::IBlob> vsBlob;
            linkedProgram->getEntryPointCode(0, targetIndex, vsBlob.writeRef(), diagnostics.writeRef());
            if (diagnostics) {
                QS_CORE_ERROR_TAG("SlangDiagnostic", "{}", static_cast<const char*>(diagnostics->getBufferPointer()));
                diagnostics = nullptr;
            }

            Slang::ComPtr<slang::IBlob> fsBlob;
            linkedProgram->getEntryPointCode(1, targetIndex, fsBlob.writeRef(), diagnostics.writeRef());
            if (diagnostics) {
                QS_CORE_ERROR_TAG("SlangDiagnostic", "{}", static_cast<const char*>(diagnostics->getBufferPointer()));
            }

            if (!vsBlob || !fsBlob) {
                return false;
            }

            vertexBuffer = Buffer::Copy(vsBlob->getBufferPointer(), vsBlob->getBufferSize());
            fragmentBuffer = Buffer::Copy(fsBlob->getBufferPointer(), fsBlob->getBufferSize());

            return true;
        }

        bool Import(void* dataSlot, const AssetMetadata& metadata) {
            const OsPath cookedPath = Project::GetCookedAssetsPath() / "Shaders";
            std::string handleStr = metadata.Handle.ToString();
            const OsPath cookedVertexPath = cookedPath / fmt::format("{}_vs", handleStr);
            const OsPath cookedFragmentPath = cookedPath / fmt::format("{}_fs", handleStr);

            Buffer vertexBuffer = Utility::ReadFile(cookedVertexPath, false);
            Buffer fragmentBuffer = Utility::ReadFile(cookedFragmentPath, false);

            auto* shader = new(dataSlot) GraphicsShader(
                std::move(vertexBuffer),
                std::move(fragmentBuffer),
                std::string(FS::Filename(metadata.FilePath))
            );

            shader->SetAssetID(metadata.Handle);

            return shader;
        }

        bool Reimport(void* shader, [[maybe_unused]] const AssetMetadata& metadata) {
            return RecompileShader(static_cast<GraphicsShader*>(shader));
        }

        bool IsAssetSupported(const std::string_view assetPath) {
            std::string_view extension = FS::Extension(assetPath);
            return extension == ".slang";
        }

        bool RecompileShader(GraphicsShader* shader) {
            if (!shader || !shader->GetAssetID()) {
                QS_ERROR_TAG(
                    "ShaderImporter::RecompileShader",
                    "Failed to recompile shader: invalid shader asset!"
                );

                return false;
            }

            const Ref<EditorAssetManager> assetManager = RefAs<EditorAssetManager>(Project::GetAssetManager());
            if (!assetManager) {
                QS_ERROR_TAG(
                    "ShaderImporter::RecompileShader",
                    "Failed to recompile shader '{}': couldn't retrieve EditorAssetManager",
                    shader->GetName()
                );

                return false;
            }

            const AssetMetadata* metadata = assetManager->GetAssetMetadata(shader->GetAssetID());
            if (!metadata) {
                QS_ERROR_TAG(
                    "ShaderImporter::RecompileShader",
                    "Failed to recompile shader '{}': couldn't retrieve asset metadata for shader!",
                    shader->GetName()
                );

                return false;
            }

            Buffer vertexBuffer, fragmentBuffer;
            if (!CompileShader(metadata->FilePath, metadata->Handle, vertexBuffer, fragmentBuffer)) {
                return false;
            }

            shader->Recreate(std::move(vertexBuffer), std::move(fragmentBuffer));
            return true;
        }

        bool Cook(const AssetMetadata& metadata) {
            const OsPath cookedPath = Project::GetCookedAssetsPath() / "Shaders";
            std::string handleStr = metadata.Handle.ToString();
            const OsPath cookedVertexPath = cookedPath / fmt::format("{}_vs", handleStr);
            const OsPath cookedFragmentPath = cookedPath / fmt::format("{}_fs", handleStr);

            Buffer vertexBuffer, fragmentBuffer;

            if (!CompileShader(metadata.FilePath, metadata.Handle, vertexBuffer, fragmentBuffer)) {
                return false;
            }

            if (!std::filesystem::exists(cookedVertexPath.parent_path())) {
                std::filesystem::create_directories(cookedVertexPath.parent_path());
            }

            Utility::WriteFile(cookedVertexPath, vertexBuffer);
            Utility::WriteFile(cookedFragmentPath, fragmentBuffer);
            return true;
        }

        AssetID ReadAssetHandle(const std::string_view assetPath) {
            return DeserializeHandle(GetMetadataPath(assetPath));
        }

        bool WriteAssetHandle(const std::string_view assetPath, const AssetID& handle) {
            return SerializeHandle(GetMetadataPath(assetPath), handle);
        }

        bool HandlesAssetType(const AssetType& type, void* userData) {
            return type == GraphicsShader::GetStaticType();
        }

        EditorAssetImporterConfig GetImporterConfig() {
            return {
                GraphicsShader::GetStaticType(),
                Import,
                IsAssetSupported,
                Reimport,
                ReadAssetHandle,
                WriteAssetHandle,
                Cook
            };
        }

        void Initialize() {
            EditorAssetImporter::RegisterAssetImporter(GetImporterConfig());

            constexpr SlangGlobalSessionDesc desc;
            slang::createGlobalSession(&desc, s_GlobalSession.writeRef());
        }
    }
}
