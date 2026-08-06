//
// Created by lasovar on 7/26/26.
//

#pragma once

namespace Quelos {
    template <typename SizeType>
        requires (std::is_integral_v<SizeType> && std::is_unsigned_v<SizeType>)
    struct IndexOutOfRangeT {
        SizeType Index;
        SizeType Size;
    };

    using IndexOutOfRange32 = IndexOutOfRangeT<uint32_t>;
    using IndexOutOfRange64 = IndexOutOfRangeT<uint64_t>;
    using IndexOutOfRange = IndexOutOfRange32;
}
