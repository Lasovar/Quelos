//
// Created by lasovar on 10/23/26.
//

#include "SidecarHandler.hpp"

#include "Quelos/Serialization/Serializer.h"
#include "Quelos/Utility/QuelosUtil.h"

namespace QuelosEditor::SidecarHandler {
    const char* GetSidecarPath(std::string_view assetPath) {
        return FormatTemp("{}.qmeta", assetPath);
    }

    OsPath GetSidecarPath(const OsPath& assetPath) {
        OsPath sidecarPath = assetPath;
        sidecarPath.concat(".qmeta");
        return sidecarPath;
    }

    Optional<AssetID> ReadAssetID(const std::string_view assetTypeName, const std::string_view assetRelativePath) {
        using namespace Serialization;
        String sidecarPath(FormatTemp("{}.qmeta", assetRelativePath), Allocator::Frame);

        if (!std::filesystem::exists(sidecarPath.view())) {
            return None;
        }

        Optional<Buffer> dataOpt = Utility::ReadFile(sidecarPath.view());
        if (!dataOpt) {
            return None;
        }

        auto& data = dataOpt.value();
        const std::string_view content(data.as<const char>(), data.size());
        QuelReader reader(content);
        std::string_view currentField;
        std::string_view currentSection;

        Optional<AssetID> assetId = None;
        for (const auto& event : reader.Parse()) {
            const bool assetIdFound = std::visit(
                Overloaded {
                    [&](const SectionEvent& sectionEvent) {
                        currentSection = sectionEvent.Name;
                        return false;
                    },
                [&](const FieldEvent& fieldEvent) {
                        currentField = fieldEvent.Path;
                        return false;
                    },
                    [&](const ValueEvent& valueEvent) {
                        if (currentField == "assetId" && currentSection == assetTypeName) {
                            assetId = AssetID(std::get<std::string_view>(valueEvent.Value));
                            return true;
                        }

                        return false;
                    },
                    [](const auto&) {
                        return false;
                    }
                }, event);

            if (assetIdFound) {
                break;
            }
        }

        return assetId;
    }

    static Expected<void, String> EnsureSidecarExists(const std::string_view assetTypeName, const OsPath& absolutePath) {
        static constexpr std::string_view sidecarTemplate = R"(
[{}]
assetId = null
importerVersion = 0
)";

        if (std::filesystem::exists(absolutePath)) {
            return {};
        }

        String toWrite(FormatTemp(sidecarTemplate, assetTypeName), Allocator::Frame);

        if (!Utility::WriteFile(absolutePath, BufferView(reinterpret_cast<const byte*>(toWrite.data()), toWrite.size()))) {
            return Unexpected(String("Failed to write default sidecar for file '{}'!", Allocator::Persistent));
        }

        return {};
    }

    Expected<void, String> WriteAssetID(
        const std::string_view assetTypeName, std::string_view assetRelativePath, const AssetID& assetId
    ) {
        using namespace Serialization;

        String sidecarPath(FormatTemp("{}.qmeta", assetRelativePath), Allocator::Frame);
        OsPath sidecarAbsolutePath = Project::GetProjectPath() / sidecarPath.view();

        if (auto r = EnsureSidecarExists(assetTypeName, sidecarAbsolutePath); !r) {
            return r;
        }

        Optional<Buffer> dataOpt = Utility::ReadFile(sidecarPath.view());
        if (!dataOpt) {
            return Unexpected(String(FormatTemp("Failed to read file '{}'!", sidecarPath), Allocator::Persistent));
        }

        auto& data = dataOpt.value();
        const std::string_view content(data.as<const char>(), data.size());
        QuelReader reader(content);
        std::string_view currentSection;
        std::string_view currentField;

        std::string outBuffer;
        StringQuelWriter writer(outBuffer);

        for (const auto& event : reader.Parse()) {
            auto error = std::visit(
                Overloaded {
                    [&](const SectionEvent& sectionEvent) -> Optional<ParseError> {
                        currentSection = sectionEvent.Name;
                        writer.Write(sectionEvent);
                        writer.CloseSection();

                        return None;
                    },
                    [&](const FieldEvent& fieldEvent) -> Optional<ParseError> {
                        currentField = fieldEvent.Path;
                        writer.Write(fieldEvent);

                        return None;
                    },
                    [&](const ValueEvent& valueEvent) -> Optional<ParseError> {
                        if (currentSection == assetTypeName && currentField == "assetId") {
                            writer.Write(ValueEvent{ UnquotedString { assetId.ToFormattedString() } });
                        } else {
                            writer.Write(ValueEvent { UnquotedString { std::get<std::string_view>(valueEvent.Value) }});
                        }

                        return None;
                    },
                    [&](const ParseError& parseError) {
                        return Optional<ParseError>(parseError);
                    },
                    [&](const auto& other) -> Optional<ParseError> {
                        writer.Write(other);
                        return None;
                    }
                }, event);

            if (error) {
                return Unexpected(
                    String(
                        FormatTemp(
                            "Failed to write asset handle! Parse error line ({}): {}",
                            error->Line, error->Message
                        ),
                        Allocator::Persistent
                    )
                );
            }
        }

        if (!Utility::WriteFile(
            sidecarAbsolutePath,
            BufferView(reinterpret_cast<const byte*>(outBuffer.data()), outBuffer.size())
        )) {
            return Unexpected(String("Failed to write to file!", Allocator::Persistent));
        }

        return {};
    }

    Expected<void, String> WriteSidecar(std::string_view assetRelativePath, const SidecarWriter& sidecarWriter) {
        OsPath sidecarAbsolutePath = Project::GetProjectPath() / FormatTemp("{}.qmeta", assetRelativePath);

        const std::string_view data = sidecarWriter.GetData();
        if (!Utility::WriteFile(
            sidecarAbsolutePath,
            BufferView(reinterpret_cast<const byte*>(data.data()), data.size())
        )) {
            return Unexpected(String("Failed to write to file!", Allocator::Persistent));
        }

        return {};
    }

    Expected<SidecarReader, String> ReadSidecar(std::string_view assetTypeName, std::string_view assetRelativePath) {
        using namespace Serialization;
        String sidecarPath(FormatTemp("{}.qmeta", assetRelativePath), Allocator::Frame);

        Optional<Buffer> dataOpt = Utility::ReadFile(sidecarPath.view());
        if (!dataOpt) {
            return Unexpected(
                String(
                    FormatTemp("Failed to read sidecar's file at '{}'", sidecarPath),
                    Allocator::Persistent
                )
            );
        }

        auto& data = dataOpt.value();
        const std::string_view content(data.as<const char>(), data.size());
        return Expected<SidecarReader, String>(std::in_place, assetTypeName, content);
    }
}
