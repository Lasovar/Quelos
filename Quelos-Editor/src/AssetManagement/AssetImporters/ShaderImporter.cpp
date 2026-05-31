#include "ShaderImporter.h"

#include "EditorLayer.h"

#include "Quelos/Utility/FileSystem.h"
#include "Quelos/Utility/QuelosUtil.h"
#include "slang.h"
#include "slang-com-helper.h"
#include "slang-com-ptr.h"
#include "Quelos/Core/Profiling.h"

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
                    "ShaderImporter",
                    "Failed to serialize shader handle at {}",
                    path.generic_string()
                );

                return false;
            }

            std::string buffer;
            StringQuelWriter writer(buffer);
            writer.Write(SectionEvent{"Shader"});
            writer.CloseSection();
            writer.WriteField("assetId", UnquotedString{handle.ToFormattedString()});

            file.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            return true;
        }

        struct ShaderMetadata {
            AssetID AssetId;
            uint64_t MaterialSize = 0;
            Vec<MaterialPropertySpec> MaterialProperties;
        };

        static bool SerializeShaderMetadata(
            const OsPath& path,
            const ShaderMetadata& shaderMetadata
        ) {
            using namespace Serialization;

            std::ofstream file(path, std::ios::binary);
            if (!file) {
                QS_CORE_ERROR_TAG(
                    "ShaderImporter",
                    "Failed to serialize shader handle at {}",
                    path.generic_string()
                );

                return false;
            }

            std::string buffer;
            StringQuelWriter writer(buffer);
            writer.Write(SectionEvent{"Shader"});
            writer.CloseSection();
            writer.WriteField("assetId", UnquotedString{shaderMetadata.AssetId.ToFormattedString()});
            writer.WriteField("materialSize", shaderMetadata.MaterialSize);
            writer.Write(ComponentEvent { "MaterialProperties" });
            for (const auto& materialProperty : shaderMetadata.MaterialProperties) {
                writer.Write(FieldEvent { materialProperty.Name });
                writer.Write(TupleBeginEvent{});
                writer.Write(ValueEvent{ UnquotedString { magic_enum::enum_name(materialProperty.Type) } });
                writer.Write(ValueEvent { materialProperty.Size });
                writer.Write(ValueEvent { materialProperty.Offset });
                writer.Write(TupleEndEvent{});
            }

            file.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            return true;
        }

        static std::optional<ShaderMetadata> DeserializeShaderMetadata(const OsPath& path) {
            using namespace Serialization;

            if (!std::filesystem::exists(path)) {
                return {};
            }

            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file) {
                QS_CORE_ERROR_TAG(
                    "ShaderImporter",
                    "DeserializeShaderMetadata: Failed to open metadata file '{}'!",
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
            std::string_view currentComponent;
            bool inTuple = false;
            uint32_t tupleIndex = 0;
            MaterialPropertySpec materialProperty;

            ShaderMetadata metadata;

            for (auto&& parserEvent : reader.Parse()) {
                std::visit(
                    Overloaded {
                        [&](const SectionEvent& event) {
                            currentSection = event.Name;
                        },
                        [&](const ComponentEvent& event) {
                            currentComponent = event.Name;
                        },
                        [&](const FieldEvent& event) {
                            currentField = event.Path;

                            if (currentComponent == "MaterialProperties") {
                                materialProperty.Name = currentField;
                            }
                        },
                        [&](const TupleBeginEvent&) {
                            inTuple = true;
                            tupleIndex = 0;
                        }, [&](const TupleEndEvent&) {
                            inTuple = false;
                            metadata.MaterialProperties.push_back(materialProperty);
                            materialProperty = {};
                        },
                        [&](const ValueEvent& event) {
                            if (currentSection != "Shader") {
                                return;
                            }

                            if (currentComponent.empty()) {
                                if (currentField == "assetId") {
                                    metadata.AssetId = AssetID(std::get<std::string_view>(event.Value));
                                } else if (currentField == "materialSize") {
                                    const auto materialSizeSv = std::get<std::string_view>(event.Value);

                                    std::from_chars(
                                        materialSizeSv.data(),
                                        materialSizeSv.data() + materialSizeSv.size(),
                                        metadata.MaterialSize
                                    );
                                }
                            }
                            else if (currentComponent == "MaterialProperties" && inTuple) {
                                if (tupleIndex == 0) {
                                    materialProperty.Type = magic_enum::enum_cast<MaterialPropertyType>(
                                       std::get<std::string_view>(event.Value)
                                   ).value_or(MaterialPropertyType::Unknown);
                                } else if (tupleIndex == 2) {
                                    const auto sizeSv = std::get<std::string_view>(event.Value);

                                    std::from_chars(
                                        sizeSv.data(),
                                        sizeSv.data() + sizeSv.size(),
                                        materialProperty.Size
                                    );
                                } else if (tupleIndex == 1) {
                                    const auto offsetSv = std::get<std::string_view>(event.Value);

                                    std::from_chars(
                                        offsetSv.data(),
                                        offsetSv.data() + offsetSv.size(),
                                        materialProperty.Offset
                                    );
                                }

                                tupleIndex++;
                            }
                        },
                        []([[maybe_unused]] auto& e) {}
                    },
                    parserEvent);
            }

            return metadata;
        }

        static AssetID DeserializeHandle(const OsPath& path) {
            using namespace Serialization;

            if (!std::filesystem::exists(path)) {
                return {};
            }

            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file) {
                QS_CORE_ERROR_TAG(
                    "ShaderImporter",
                    "DeserializeHandle Failed to open metadata file '{}'",
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
            std::string_view currentComponent;

            AssetID assetId;
            for (auto&& parserEvent : reader.Parse()) {
                if (assetId) {
                    break;
                }

                std::visit(Overloaded{
                               [&](const SectionEvent& event) {
                                   currentSection = event.Name;
                               },
                               [&](const FieldEvent& event) {
                                   currentField = event.Path;
                               }, [&](const ComponentEvent& event) {
                                   currentComponent = event.Name;
                               },
                               [&](const ValueEvent& event) {
                                   if (currentSection != "Shader" || !currentComponent.empty()) {
                                       return;
                                   }

                                   if (currentField == "assetId") {
                                       assetId = AssetID(std::get<std::string_view>(event.Value));
                                   }
                               },
                               []([[maybe_unused]] auto& e) {}
                           },
                           parserEvent);
            }

            return assetId;
        }

        static MaterialPropertyType GetMaterialProperty(slang::VariableLayoutReflection* variableLayout) {
            slang::TypeReflection* fieldType = variableLayout->getType();
            switch (fieldType->getKind()) {
            case slang::TypeReflection::Kind::None:
            case slang::TypeReflection::Kind::ConstantBuffer:
            case slang::TypeReflection::Kind::Resource:
            case slang::TypeReflection::Kind::SamplerState:
            case slang::TypeReflection::Kind::TextureBuffer:
            case slang::TypeReflection::Kind::ShaderStorageBuffer:
            case slang::TypeReflection::Kind::ParameterBlock:
            case slang::TypeReflection::Kind::GenericTypeParameter:
            case slang::TypeReflection::Kind::Interface:
            case slang::TypeReflection::Kind::OutputStream:
            case slang::TypeReflection::Kind::Specialized:
            case slang::TypeReflection::Kind::Feedback:
            case slang::TypeReflection::Kind::Pointer:
            case slang::TypeReflection::Kind::DynamicResource:
            case slang::TypeReflection::Kind::MeshOutput:
            case slang::TypeReflection::Kind::Enum:
            case slang::TypeReflection::Kind::Struct:
            case slang::TypeReflection::Kind::Array:
            case slang::TypeReflection::Kind::Matrix:
                return MaterialPropertyType::Unknown;
            case slang::TypeReflection::Kind::Scalar: {
                slang::VariableReflection* variable = variableLayout->getVariable();
                for (uint32_t attribIndex = 0; attribIndex < variable->getUserAttributeCount(); attribIndex++) {
                    const char* attributeName = variable->getUserAttributeByIndex(attribIndex)->getName();
                    if (!strcmp(attributeName, "Texture2D")) {
                        return MaterialPropertyType::Texture2D;
                    }
                }

                return MaterialPropertyType::Float;
            }
            case slang::TypeReflection::Kind::Vector:
                switch (fieldType->getElementCount()) {
                case 1:
                    return MaterialPropertyType::Float;
                case 2:
                    return MaterialPropertyType::Float2;
                case 3:
                    return MaterialPropertyType::Float3;
                case 4: {
                    slang::VariableReflection* variable = variableLayout->getVariable();
                    for (uint32_t attribIndex = 0; attribIndex < variable->getUserAttributeCount(); attribIndex++) {
                        const char* attributeName = variable->getUserAttributeByIndex(attribIndex)->getName();
                        if (!strcmp(attributeName, "Color")) {
                            return MaterialPropertyType::Color;
                        }
                    }

                    return MaterialPropertyType::Float4;
                }
                default:
                    return MaterialPropertyType::Unknown;
                }
            default:
                return MaterialPropertyType::Unknown;
            }
        }

        bool CompileShader(
            const std::string& shaderPath,
            const AssetID handle,
            Buffer& vertexBuffer,
            Buffer& fragmentBuffer,
            Vec<MaterialPropertySpec>& materialProperties,
            uint64_t& materialSize
        ) {
            //QS_PROFILE_SCOPED_N("Shader compilation");
            auto t0 = std::chrono::high_resolution_clock::now();

            slang::SessionDesc sessionDesc;

            slang::TargetDesc targetDesc;
            switch (Renderer::GetRendererAPI()) {
            case RendererAPI::Direct3D11:
            case RendererAPI::Direct3D12: {
                targetDesc.format = SLANG_DXBC;
                targetDesc.profile = s_GlobalSession->findProfile("sm_5_0");
                break;
            }
            case RendererAPI::Vulkan: {
                targetDesc.format = SLANG_SPIRV;
                targetDesc.profile = s_GlobalSession->findProfile("spirv_1_5");

                slang::CompilerOptionEntry opts[] = {
                    {slang::CompilerOptionName::VulkanUseEntryPointName, {slang::CompilerOptionValueKind::Int, 1}},
                    {slang::CompilerOptionName::EmitSpirvDirectly, {slang::CompilerOptionValueKind::Int, 1}},
                };

                targetDesc.compilerOptionEntries = opts;
                targetDesc.compilerOptionEntryCount = 2;
                break;
            }
            case RendererAPI::OpenGL: {
                targetDesc.format = SLANG_HLSL;
                targetDesc.profile = s_GlobalSession->findProfile("sm_5_0");
                break;
            }
            default:
                QS_CORE_ASSERT(false, "Unsupported renderer API");
                break;
            }

            sessionDesc.targets = &targetDesc;
            sessionDesc.targetCount = 1;

            std::string parentPath = (Project::GetProjectPath() / shaderPath).parent_path().generic_string();
            const char* searchPaths[] = {parentPath.c_str()};
            sessionDesc.searchPaths = searchPaths;
            sessionDesc.searchPathCount = 1;

            Slang::ComPtr<slang::ISession> session;
            s_GlobalSession->createSession(sessionDesc, session.writeRef());

            const std::string moduleName(FS::Stem(shaderPath));
            Slang::ComPtr<slang::IBlob> diagnostics;
            slang::IModule* module = session->loadModule(moduleName.c_str(), diagnostics.writeRef());
            if (diagnostics) {
                QS_CORE_TRACE_TAG("Slang", "{}", static_cast<const char*>(diagnostics->getBufferPointer()));
                diagnostics = nullptr;
            }

            slang::ProgramLayout* layout = module->getLayout(0, diagnostics.writeRef());
            if (diagnostics) {
                QS_CORE_ERROR_TAG("Slang", "{}", static_cast<const char*>(diagnostics->getBufferPointer()));
                diagnostics = nullptr;
            }

            for (uint32_t parameterIndex = 0; parameterIndex < layout->getParameterCount(); parameterIndex++) {
                slang::VariableLayoutReflection* parameter = layout->getParameterByIndex(parameterIndex);
                if (strcmp(parameter->getName(), "Materials") != 0) {
                    continue;
                }

                slang::TypeLayoutReflection* type = parameter->getTypeLayout()->getElementTypeLayout();
                uint64_t offset = 0;

                materialSize = type->getSize();
                for (uint32_t typeFieldIndex = 0; typeFieldIndex < type->getFieldCount(); typeFieldIndex++) {
                    slang::VariableLayoutReflection* field = type->getFieldByIndex(typeFieldIndex);
                    MaterialPropertyType propertyType = GetMaterialProperty(field);
                    const uint64_t fieldSize = field->getTypeLayout()->getSize();

                    if (propertyType == MaterialPropertyType::Unknown) {
                        offset += fieldSize;
                        continue;
                    }

                    materialProperties.emplace_back(field->getName(), propertyType, offset, fieldSize);
                    offset += fieldSize;
                }
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

            QS_CORE_TRACE_TAG("Slang", "Module found: {}", module->getName());
            for (uint32_t i = 0; i < moduleLayout->getParameterCount(); i++) {
                slang::VariableLayoutReflection* parameter = moduleLayout->getParameterByIndex(i);
                QS_CORE_TRACE_TAG(
                    "Slang",
                    "{}",
                    parameter->getName() ? parameter->getName() : ""
                );
            }

            Slang::ComPtr<slang::IComponentType> linkedProgram;
            program->link(linkedProgram.writeRef(), diagnostics.writeRef());

            if (diagnostics) {
                QS_CORE_ERROR_TAG(
                    "Slang",
                    "{}",
                    static_cast<const char*>(diagnostics->getBufferPointer())
                );

                diagnostics = nullptr;
            }

            auto t1 = std::chrono::high_resolution_clock::now();

            QS_CORE_TRACE_TAG(
                "Slang",
                "Module compilation: {}",
                std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0)
            );

            const int targetIndex = 0;

            Slang::ComPtr<slang::IBlob> vsBlob;
            linkedProgram->getEntryPointCode(0, targetIndex, vsBlob.writeRef(), diagnostics.writeRef());
            if (diagnostics) {
                QS_CORE_ERROR_TAG(
                    "Slang",
                    "Failed to get vertex code: {}", static_cast<const char*>(diagnostics->getBufferPointer())
                );

                diagnostics = nullptr;
            }

            auto t2 = std::chrono::high_resolution_clock::now();
            QS_CORE_TRACE_TAG(
                "Slang",
                "Vertex code compilation: {}",
                std::chrono::duration_cast<std::chrono::milliseconds>(t2-t1)
            );

            Slang::ComPtr<slang::IBlob> fsBlob;
            linkedProgram->getEntryPointCode(1, targetIndex, fsBlob.writeRef(), diagnostics.writeRef());
            if (diagnostics) {
                QS_CORE_ERROR_TAG(
                    "Slang",
                    "Failed to get fragment code: {}", static_cast<const char*>(diagnostics->getBufferPointer()));
            }

            if (!vsBlob || !fsBlob) {
                return false;
            }

            auto t3 = std::chrono::high_resolution_clock::now();

            QS_CORE_TRACE_TAG(
                "Slang",
                "Fragment code compilation: {}",
                std::chrono::duration_cast<std::chrono::milliseconds>(t3-t2)
            );

            vertexBuffer = Buffer::Copy(vsBlob->getBufferPointer(), vsBlob->getBufferSize());
            fragmentBuffer = Buffer::Copy(fsBlob->getBufferPointer(), fsBlob->getBufferSize());

            return true;
        }

        bool Import(void* dataSlot, const AssetMetadata& metadata) {
            std::optional<ShaderMetadata> shaderMetadata = DeserializeShaderMetadata(GetMetadataPath(metadata.FilePath));
            if (!shaderMetadata) {
                return false;
            }

            const OsPath cookedPath = Project::GetCookedAssetsPath() / "Shaders";
            std::string handleStr = metadata.Handle.ToString();
            const OsPath cookedVertexPath = cookedPath / fmt::format("{}_vs", handleStr);
            const OsPath cookedFragmentPath = cookedPath / fmt::format("{}_fs", handleStr);

            Buffer vertexBuffer = Utility::ReadFile(cookedVertexPath, false);
            Buffer fragmentBuffer = Utility::ReadFile(cookedFragmentPath, false);

            auto* shader = new(dataSlot) GraphicsShader(
                std::move(vertexBuffer),
                std::move(fragmentBuffer),
                std::string(FS::Filename(metadata.FilePath)),
                shaderMetadata->MaterialProperties,
                shaderMetadata->MaterialSize
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
                    "ShaderImporter",
                    "Failed to recompile shader: invalid shader asset!"
                );

                return false;
            }

            const Ref<EditorAssetManager> assetManager = RefAs<EditorAssetManager>(Project::GetAssetManager());
            if (!assetManager) {
                QS_ERROR_TAG(
                    "ShaderImporter",
                    "Failed to recompile shader '{}': couldn't retrieve EditorAssetManager",
                    shader->GetName()
                );

                return false;
            }

            const AssetMetadata* metadata = assetManager->GetAssetMetadata(shader->GetAssetID());
            if (!metadata) {
                QS_ERROR_TAG(
                    "ShaderImporter",
                    "Failed to recompile shader '{}': couldn't retrieve asset metadata for shader!",
                    shader->GetName()
                );

                return false;
            }

            Buffer vertexBuffer, fragmentBuffer;
            Vec<MaterialPropertySpec> materialProperties;
            uint64_t materialSize = 0;
            if (!CompileShader(
                metadata->FilePath,
                metadata->Handle,
                vertexBuffer,
                fragmentBuffer,
                materialProperties,
                materialSize)
            ) {
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

            namespace fs = std::filesystem;

            // Check if cache is valid (exists and newer than source)
            const auto srcTime = fs::last_write_time(Project::GetProjectPath() / metadata.FilePath);
            const bool cacheValid = fs::exists(cookedVertexPath) && fs::exists(cookedFragmentPath)
                           && fs::last_write_time(cookedVertexPath) >= srcTime
                           && fs::last_write_time(cookedFragmentPath) >= srcTime;

            if (cacheValid) {
                return true;
            }

            ShaderMetadata shaderMetadata;
            shaderMetadata.AssetId = metadata.Handle;

            if (!CompileShader(
                    metadata.FilePath,
                    metadata.Handle,
                    vertexBuffer,
                    fragmentBuffer,
                    shaderMetadata.MaterialProperties,
                    shaderMetadata.MaterialSize
                )
            ) {
                return false;
            }

            if (!std::filesystem::exists(cookedVertexPath.parent_path())) {
                std::filesystem::create_directories(cookedVertexPath.parent_path());
            }

            Utility::WriteFile(cookedVertexPath, vertexBuffer);
            Utility::WriteFile(cookedFragmentPath, fragmentBuffer);

            SerializeShaderMetadata(GetMetadataPath(metadata.FilePath), shaderMetadata);

            return true;
        }

        AssetID ReadAssetHandle(const std::string_view assetPath) {
            return DeserializeHandle(GetMetadataPath(assetPath));
        }

        bool WriteAssetHandle(const std::string_view assetPath, const AssetID& handle) {
            const OsPath path = GetMetadataPath(assetPath);
            std::optional<ShaderMetadata> metadata = DeserializeShaderMetadata(path);
            if (!metadata) {
                return false;
            }

            metadata.value().AssetId = handle;
            return SerializeShaderMetadata(path, metadata.value());
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
