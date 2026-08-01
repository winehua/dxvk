#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "util_env.h"
#include "log/log.h"
#include "util_string.h"

namespace dxvk {

  enum class WineHuaDualSrcMode {
    TwoPass,
    SecondaryReplace,
    PrimaryReplace,
  };


  inline WineHuaDualSrcMode winehuaDualSrcMode() {
    static const WineHuaDualSrcMode mode = [] {
      const std::string value = env::getEnvVar("DXVK_WINEHUA_DUAL_SRC_MODE");

      if (value == "secondary-replace")
        return WineHuaDualSrcMode::SecondaryReplace;
      if (value == "primary-replace")
        return WineHuaDualSrcMode::PrimaryReplace;
      return WineHuaDualSrcMode::TwoPass;
    }();
    return mode;
  }


  inline const char* winehuaDualSrcModeName(WineHuaDualSrcMode mode) {
    switch (mode) {
      case WineHuaDualSrcMode::SecondaryReplace: return "secondary-replace";
      case WineHuaDualSrcMode::PrimaryReplace:   return "primary-replace";
      default:                                   return "two-pass";
    }
  }

  inline bool winehuaApiTraceEnabled() {
    static const bool enabled = env::getEnvVar("DXVK_WINEHUA_TRACE_API") == "1";
    return enabled;
  }


  inline bool winehuaMappedTraceEnabled() {
    static const bool enabled = env::getEnvVar("DXVK_WINEHUA_TRACE_MAPPED") == "1";
    return enabled;
  }


  inline bool winehuaGeometryTraceEnabled() {
    static const bool enabled = env::getEnvVar("DXVK_WINEHUA_TRACE_GEOMETRY") == "1";
    return enabled;
  }


  inline bool winehuaViewportTraceEnabled() {
    static const bool enabled = env::getEnvVar("DXVK_WINEHUA_TRACE_VIEWPORT") == "1";
    return enabled;
  }


  inline bool winehuaBatchMappedFlushEnabled() {
    static const bool enabled =
      env::getEnvVar("DXVK_WINEHUA_BATCH_MAPPED_FLUSH") == "1";
    return enabled;
  }


  inline bool winehuaBatchMappedFlushStatsEnabled() {
    static const bool enabled =
      env::getEnvVar("DXVK_WINEHUA_BATCH_MAPPED_FLUSH_STATS") == "1";
    return enabled;
  }


  inline bool winehuaMappedTraceSample(
          std::atomic<uint64_t>& counter,
          uint64_t&              sequence) {
    if (!winehuaMappedTraceEnabled())
      return false;

    sequence = counter.fetch_add(1, std::memory_order_relaxed) + 1;
    return sequence <= 256 || !(sequence & (sequence - 1));
  }


  inline bool winehuaGeometryTraceSample(
          std::atomic<uint64_t>& counter,
          uint64_t&              sequence,
          uint64_t               byteSize = 0) {
    if (!winehuaGeometryTraceEnabled())
      return false;

    sequence = counter.fetch_add(1, std::memory_order_relaxed) + 1;
    return sequence <= 512
        || !(sequence & (sequence - 1))
        || (byteSize >= 64u * 1024u && sequence <= 4096);
  }


  inline uint64_t winehuaGeometryTraceHash(
    const void* data,
          size_t size) {
    constexpr uint64_t OffsetBasis = 14695981039346656037ull;
    constexpr uint64_t Prime = 1099511628211ull;
    constexpr size_t EdgeBytes = 4096;

    const auto* bytes = reinterpret_cast<const uint8_t*>(data);
    uint64_t hash = OffsetBasis;

    auto hashRange = [&] (size_t begin, size_t end) {
      for (size_t i = begin; i < end; i++) {
        hash ^= bytes[i];
        hash *= Prime;
      }
    };

    if (size <= 2 * EdgeBytes) {
      hashRange(0, size);
    } else {
      hashRange(0, EdgeBytes);
      hashRange(size - EdgeBytes, size);
    }

    hash ^= size;
    hash *= Prime;
    return hash;
  }


  class WineHuaApiTraceScope {
  public:

    WineHuaApiTraceScope(const char* name, std::atomic<uint64_t>& counter)
    : m_name(name) {
      if (!winehuaApiTraceEnabled())
        return;

      m_sequence = counter.fetch_add(1, std::memory_order_relaxed) + 1;
      m_sample = m_sequence <= 64 || !(m_sequence & (m_sequence - 1));

      if (m_sample)
        Logger::info(str::format("WineHua api-trace: ", m_name, "#", m_sequence, " enter"));
    }

    ~WineHuaApiTraceScope() {
      if (m_sample)
        Logger::info(str::format("WineHua api-trace: ", m_name, "#", m_sequence, " exit"));
    }

  private:

    const char* m_name = nullptr;
    uint64_t    m_sequence = 0;
    bool        m_sample = false;
  };

}

#define WINEHUA_API_TRACE() \
  static std::atomic<uint64_t> winehuaApiTraceCounter { 0 }; \
  dxvk::WineHuaApiTraceScope winehuaApiTraceScope(__func__, winehuaApiTraceCounter)
