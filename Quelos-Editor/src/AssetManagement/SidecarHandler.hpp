//
// Created by lasovar on 10/23/26.
//

#pragma once

#include "Quelos/AssetManager/Asset.h"
#include "Quelos/Utility/Optional.hpp"
#include "Quelos/Serialization/QuelArchive.h"

namespace QuelosEditor {
    using namespace Quelos;

    class SidecarWriter {
    public:
        explicit SidecarWriter(const std::string_view assetTypeName, const AssetID& assetId, const uint32_t importerVersion)
            : m_Writer(m_Data)
        {
            m_Writer.Write(Serialization::SectionEvent{ assetTypeName });
            m_Writer.CloseSection();
            m_Writer.WriteField("assetId", assetId.ToFormattedString());
            m_Writer.WriteField("importerVersion", importerVersion);
        }

        template <SerializableWith<Serialization::QuelWriteArchive> T>
        void StoreSettings(const T& t) {
            m_Writer.Write(Serialization::SectionEvent { "Settings" });
            m_Writer.CloseSection();

            Serialization::QuelWriteArchive archive(m_Writer);
            T::Serialize(archive, t);
        }

        [[nodiscard]] std::string_view GetData() const { return m_Data; }

    private:
        std::string m_Data;
        Serialization::StringQuelWriter m_Writer;
    };

    class SidecarReader {
    public:
        explicit SidecarReader(const std::string_view assetTypeName, const std::string_view sidecarContent)
            : m_Reader(sidecarContent), m_Archive(m_Reader)
        {
            m_Archive.Section(assetTypeName);

            m_Archive.Field("assetId", m_AssetId);
            m_Archive.Field("importerVersion", m_ImporterVersion);
        }

        [[nodiscard]] const AssetID& GetAssetID() const { return m_AssetId; }
        [[nodiscard]] uint32_t GetImporterVersion() const { return m_ImporterVersion; }

        template <typename T>
        void ReadSettings(T& t) {
            m_Archive.Section("Settings");
            T::Serialize(m_Archive, t);
        }

    private:
        uint32_t m_ImporterVersion = 0;
        AssetID m_AssetId;
        Serialization::QuelReader m_Reader;
        Serialization::QuelAutoReadArchive m_Archive;
    };

    namespace SidecarHandler {
        /// Just appends '.qmeta` to the end and returns a new temporary string
        const char* GetSidecarPath(std::string_view assetPath);
        /// Just appends '.qmeta` to the end and returns a OsPath
        OsPath GetSidecarPath(const OsPath& assetPath);

        Optional<AssetID> ReadAssetID(std::string_view assetTypeName, std::string_view assetRelativePath);
        Expected<void, String> WriteAssetID(std::string_view assetTypeName, std::string_view assetRelativePath, const AssetID& assetId);

        Expected<void, String> WriteSidecar(std::string_view assetRelativePath, const SidecarWriter& sidecarWriter);
        Expected<SidecarReader, String> ReadSidecar(std::string_view assetTypeName, std::string_view assetRelativePath);
    }
}
