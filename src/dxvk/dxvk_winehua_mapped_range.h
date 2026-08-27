#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>

namespace dxvk {

  inline uint64_t winehuaMappedRangeEnd(uint64_t offset, uint64_t size) {
    if (size == std::numeric_limits<uint64_t>::max()
     || size > std::numeric_limits<uint64_t>::max() - offset)
      return std::numeric_limits<uint64_t>::max();
    return offset + size;
  }


  inline bool winehuaMergeMappedRange(
          uint64_t& baseOffset,
          uint64_t& baseSize,
          uint64_t  nextOffset,
          uint64_t  nextSize) {
    if (!baseSize || !nextSize)
      return false;

    const uint64_t baseEnd = winehuaMappedRangeEnd(baseOffset, baseSize);
    const uint64_t nextEnd = winehuaMappedRangeEnd(nextOffset, nextSize);
    if (nextOffset > baseEnd || baseOffset > nextEnd)
      return false;

    const uint64_t mergedBegin = std::min(baseOffset, nextOffset);
    const uint64_t mergedEnd = std::max(baseEnd, nextEnd);
    baseOffset = mergedBegin;
    baseSize = mergedEnd == std::numeric_limits<uint64_t>::max()
      ? std::numeric_limits<uint64_t>::max()
      : mergedEnd - mergedBegin;
    return true;
  }

}
