#pragma once

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
    if (winehuaQueryTraceEnabled())
      Logger::info("WineHuaQuery: " + message);
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
