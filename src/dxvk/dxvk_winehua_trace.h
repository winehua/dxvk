#pragma once

#include <atomic>
#include <cstring>
#include <cstdlib>
#include <cstdint>

#include "../util/log/log.h"

namespace dxvk {

  enum class WineHuaDualSrcMode {
    TwoPass,
    SecondaryReplace,
    SecondaryMultiply,
    PrimaryReplace,
    PrimaryAdd,
  };

  inline WineHuaDualSrcMode winehuaDualSrcMode() {
    const char* value = std::getenv("DXVK_WINEHUA_DUAL_SRC_MODE");

    if (value && !std::strcmp(value, "secondary-replace"))
      return WineHuaDualSrcMode::SecondaryReplace;
    if (value && !std::strcmp(value, "secondary-multiply"))
      return WineHuaDualSrcMode::SecondaryMultiply;
    if (value && !std::strcmp(value, "primary-replace"))
      return WineHuaDualSrcMode::PrimaryReplace;
    if (value && !std::strcmp(value, "primary-add"))
      return WineHuaDualSrcMode::PrimaryAdd;
    return WineHuaDualSrcMode::TwoPass;
  }

  inline const char* winehuaDualSrcModeName(WineHuaDualSrcMode mode) {
    switch (mode) {
      case WineHuaDualSrcMode::SecondaryReplace:  return "secondary-replace";
      case WineHuaDualSrcMode::SecondaryMultiply: return "secondary-multiply";
      case WineHuaDualSrcMode::PrimaryReplace:    return "primary-replace";
      case WineHuaDualSrcMode::PrimaryAdd:        return "primary-add";
      default:                                    return "two-pass";
    }
  }

  /* Narrow, opt-in diagnostics for the WineHua sampled-image investigation.
   * The normal DXVK runtime never emits these records. */
  inline bool winehuaSampleTraceEnabled() {
    static int enabled = -1;
    if (enabled < 0) {
      const char* value = std::getenv("DXVK_WINEHUA_TRACE_SAMPLED");
      enabled = value && value[0] == '1' ? 1 : 0;
    }
    return enabled != 0;
  }

  inline void winehuaSampleTrace(const std::string& message) {
    if (winehuaSampleTraceEnabled())
      Logger::info("WineHuaSampled: " + message);
  }

  inline void winehuaRenderPassTrace(const std::string& message) {
    if (!winehuaSampleTraceEnabled())
      return;

    static std::atomic<uint32_t> emitted { 0 };
    const uint32_t index = emitted.fetch_add(1, std::memory_order_relaxed);
    if (index < 1024)
      Logger::info("WineHuaRenderPass: " + message);
    else if (index == 1024)
      Logger::info("WineHuaRenderPass: further records suppressed");
  }

  /* Render-target capture is deliberately separate from the normal sampled
   * trace. It is enabled for one selected frame only and must never become a
   * product rendering path. */
  inline bool winehuaRenderTargetDumpEnabled() {
    const char* value = std::getenv("WINEHUA_DXVK_DUMP_RT");
    return value && value[0] == '1';
  }

  inline uint64_t winehuaRenderTargetDumpFrame() {
    static uint64_t frame = UINT64_MAX;
    if (frame == UINT64_MAX) {
      const char* value = std::getenv("WINEHUA_DXVK_DUMP_FRAME");
      char* end = nullptr;
      frame = value && value[0]
        ? std::strtoull(value, &end, 10) : 0;
      if (!end || *end != '\0')
        frame = 0;
    }
    return frame;
  }

  inline bool winehuaDrawTraceEnabled() {
    static int enabled = -1;
    if (enabled < 0) {
      const char* value = std::getenv("WINEHUA_DXVK_TRACE_DRAWS");
      enabled = value && value[0] == '1' ? 1 : 0;
    }
    return enabled != 0;
  }

  inline uint32_t winehuaDrawTracePass() {
    static uint32_t pass = UINT32_MAX;
    static bool initialized = false;
    if (!initialized) {
      const char* value = std::getenv("WINEHUA_DXVK_TRACE_PASS");
      char* end = nullptr;
      if (value && value[0]) {
        const unsigned long parsed = std::strtoul(value, &end, 10);
        pass = end && *end == '\0' ? uint32_t(parsed) : UINT32_MAX;
      }
      initialized = true;
    }
    return pass;
  }

  inline uint32_t winehuaDrawTraceSecondPass() {
    static uint32_t pass = UINT32_MAX;
    static bool initialized = false;
    if (!initialized) {
      const char* value = std::getenv("WINEHUA_DXVK_TRACE_PASS_SECOND");
      char* end = nullptr;
      if (value && value[0]) {
        const unsigned long parsed = std::strtoul(value, &end, 10);
        pass = end && *end == '\0' ? uint32_t(parsed) : UINT32_MAX;
      }
      initialized = true;
    }
    return pass;
  }

  inline uint32_t winehuaDrawTraceMaxDraws() {
    static uint32_t count = UINT32_MAX;
    if (count == UINT32_MAX) {
      const char* value = std::getenv("WINEHUA_DXVK_TRACE_DRAW_MAX");
      char* end = nullptr;
      count = value && value[0]
        ? uint32_t(std::strtoul(value, &end, 10)) : 2048u;
      if (!end || *end != '\0' || !count)
        count = 2048u;
    }
    return count;
  }

  inline uint32_t winehuaRenderTargetDumpDraw() {
    static uint32_t draw = UINT32_MAX;
    static bool initialized = false;
    if (!initialized) {
      const char* value = std::getenv("WINEHUA_DXVK_DUMP_DRAW");
      char* end = nullptr;
      if (value && value[0]) {
        const unsigned long parsed = std::strtoul(value, &end, 10);
        draw = end && *end == '\0' ? uint32_t(parsed) : UINT32_MAX;
      }
      initialized = true;
    }
    return draw;
  }

  inline const char* winehuaRenderTargetDumpFragmentShader() {
    const char* value = std::getenv("WINEHUA_DXVK_DUMP_FS");
    return value && value[0] ? value : "";
  }

  inline uint64_t winehuaRenderTargetDumpIndexCount() {
    static uint64_t count = UINT64_MAX;
    if (count == UINT64_MAX) {
      const char* value = std::getenv("WINEHUA_DXVK_DUMP_INDEX_COUNT");
      char* end = nullptr;
      if (value && value[0]) {
        const unsigned long long parsed = std::strtoull(value, &end, 10);
        count = end && *end == '\0' ? parsed : UINT64_MAX;
      }
    }
    return count;
  }

  inline uint64_t winehuaRenderTargetDumpFirstIndex() {
    static uint64_t first = UINT64_MAX;
    if (first == UINT64_MAX) {
      const char* value = std::getenv("WINEHUA_DXVK_DUMP_FIRST_INDEX");
      char* end = nullptr;
      if (value && value[0]) {
        const unsigned long long parsed = std::strtoull(value, &end, 10);
        first = end && *end == '\0' ? parsed : UINT64_MAX;
      }
    }
    return first;
  }

  inline int64_t winehuaRenderTargetDumpVertexOffset() {
    static int64_t offset = INT64_MIN;
    static bool initialized = false;
    if (!initialized) {
      const char* value = std::getenv("WINEHUA_DXVK_DUMP_VERTEX_OFFSET");
      char* end = nullptr;
      if (value && value[0]) {
        const long long parsed = std::strtoll(value, &end, 10);
        offset = end && *end == '\0' ? parsed : INT64_MIN;
      }
      initialized = true;
    }
    return offset;
  }

  inline bool winehuaTargetDrawCaptureEnabled() {
    return winehuaDrawTraceEnabled()
        && winehuaRenderTargetDumpEnabled()
        && (winehuaRenderTargetDumpDraw() != UINT32_MAX
         || winehuaRenderTargetDumpFragmentShader()[0]);
  }

  inline uint32_t winehuaRenderTargetDumpMaxAttachments() {
    static uint32_t count = UINT32_MAX;
    if (count == UINT32_MAX) {
      const char* value = std::getenv("WINEHUA_DXVK_DUMP_RT_MAX");
      char* end = nullptr;
      count = value && value[0]
        ? uint32_t(std::strtoul(value, &end, 10)) : 12u;
      if (!end || *end != '\0' || !count)
        count = 12u;
    }
    return count;
  }

  inline bool winehuaRenderTargetDumpSampledEnabled() {
    const char* value = std::getenv("WINEHUA_DXVK_DUMP_SAMPLED");
    return !value || value[0] != '0';
  }

  inline uint32_t winehuaRenderTargetDumpFirstPass() {
    static uint32_t pass = UINT32_MAX;
    if (pass == UINT32_MAX) {
      const char* value = std::getenv("WINEHUA_DXVK_DUMP_PASS_START");
      char* end = nullptr;
      pass = value && value[0]
        ? uint32_t(std::strtoul(value, &end, 10)) : 0u;
      if (!end || *end != '\0')
        pass = 0u;
    }
    return pass;
  }

  inline uint64_t winehuaRenderTargetDumpMaxBytes() {
    static uint64_t bytes = UINT64_MAX;
    if (bytes == UINT64_MAX) {
      const char* value = std::getenv("WINEHUA_DXVK_DUMP_RT_MAX_BYTES");
      char* end = nullptr;
      bytes = value && value[0]
        ? std::strtoull(value, &end, 10) : (64ull << 20);
      if (!end || *end != '\0' || !bytes)
        bytes = 64ull << 20;
    }
    return bytes;
  }

  inline uint64_t winehuaGeometryDumpMaxBytes() {
    static uint64_t bytes = UINT64_MAX;
    if (bytes == UINT64_MAX) {
      const char* value = std::getenv("WINEHUA_DXVK_DUMP_GEOMETRY_MAX_BYTES");
      char* end = nullptr;
      bytes = value && value[0]
        ? std::strtoull(value, &end, 10) : (32ull << 20);
      if (!end || *end != '\0' || !bytes)
        bytes = 32ull << 20;
    }
    return bytes;
  }

  inline const char* winehuaRenderTargetDumpPath() {
    const char* value = std::getenv("WINEHUA_DXVK_DUMP_RT_PATH");
    return value && value[0] ? value : ".";
  }

  inline bool winehuaForceSampledGeneral() {
    const char* value = std::getenv("DXVK_WINEHUA_FORCE_SAMPLED_GENERAL");
    return value && value[0] == '1';
  }

  inline bool winehuaCommandQueryReset() {
    const char* value = std::getenv("DXVK_WINEHUA_COMMAND_QUERY_RESET");
    return value && value[0] == '1';
  }

  inline bool winehuaQueryTraceEnabled() {
    const char* value = std::getenv("DXVK_WINEHUA_TRACE_QUERY");
    return value && value[0] == '1';
  }

  inline void winehuaQueryTrace(const std::string& message) {
    if (!winehuaQueryTraceEnabled())
      return;

    static std::atomic<uint32_t> emitted { 0 };
    const uint32_t index = emitted.fetch_add(1, std::memory_order_relaxed);
    if (index < 512)
      Logger::info("WineHuaQuery: " + message);
    else if (index == 512)
      Logger::info("WineHuaQuery: further records suppressed");
  }

  /* Bounded startup-flow diagnostics. These are deliberately opt-in because
   * shader and pipeline creation can happen on multiple worker threads. */
  inline bool winehuaFlowTraceEnabled() {
    const char* value = std::getenv("DXVK_WINEHUA_TRACE_FLOW");
    return value && value[0] == '1';
  }

  inline void winehuaFlowTrace(const std::string& message) {
    if (!winehuaFlowTraceEnabled())
      return;

    static std::atomic<uint32_t> emitted { 0 };
    const uint32_t index = emitted.fetch_add(1, std::memory_order_relaxed);
    if (index < 4096)
      Logger::info("WineHuaFlow: " + message);
    else if (index == 4096)
      Logger::info("WineHuaFlow: further records suppressed");
  }

  inline bool winehuaFlushDynamicMapped() {
    const char* value = std::getenv("DXVK_WINEHUA_FLUSH_DYNAMIC_MAPPED");
    return value && value[0] == '1';
  }

  inline bool winehuaPreciseShadowEnabled() {
    const char* value = std::getenv("DXVK_WINEHUA_PRECISE_SHADOW");
    return value && value[0] == '1';
  }

  inline bool winehuaForceHeavenPass2DepthAlways() {
    const char* value = std::getenv(
      "WINEHUA_DXVK_FORCE_HEAVEN_PASS2_DEPTH_ALWAYS");
    return value && value[0] == '1';
  }

}
