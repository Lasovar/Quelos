//
// Created by lasovar on 10/11/26.
//

#include "Hash.h"

#include <cstdio>

#include "xxhash.h"
#include "Quelos/Core/Buffer.h"
#include "Quelos/Core/Log.h"
#include "Quelos/Core/SmartPointers.h"

namespace Quelos::Hash {
    Optional<Hash128> File(std::string_view path) {
        const UniquePtr<FILE, decltype(&std::fclose)> file(
            std::fopen(FormatTemp("{}", path), "rb"),
            &std::fclose
        );

        if (!file) {
            return None;
        }

        const UniquePtr<XXH3_state_t, decltype(&XXH3_freeState)> state(XXH3_createState(), &XXH3_freeState);
        if (!state || XXH3_128bits_reset(state.get()) == XXH_ERROR) {
            return None;
        }

        static constexpr size_t ChunkSize = 1 << 20; // 1 MiB
        thread_local Buffer buffer = Buffer::Allocate(ChunkSize);
        size_t read;
        while ((read = std::fread(buffer.data(), 1, ChunkSize, file.get())) > 0) {
            XXH3_128bits_update(state.get(), buffer.data(), read);
        }

        if (std::ferror(file.get())) {
            return None;
        }

        // Canonical form: fixed big-endian byte order
        XXH128_canonical_t canonical;
        XXH128_canonicalFromHash(&canonical, XXH3_128bits_digest(state.get()));

        Hash128 result;
        static_assert(sizeof(result) == sizeof(canonical));
        std::memcpy(&result, &canonical, sizeof(result));

        return { result };
    }
}
