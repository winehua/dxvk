#pragma once

#include <atomic>
#include <cstdlib>

#include "../util/log/log.h"

namespace dxvk {

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

}
