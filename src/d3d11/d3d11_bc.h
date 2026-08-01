#pragma once

#include <vector>

#include "../dxvk/dxvk_include.h"

namespace dxvk {

  struct D3D11CpuImage {
    std::vector<uint8_t> data;
    VkDeviceSize rowPitch = 0;
    VkDeviceSize slicePitch = 0;
  };

  /**
   * Decompresses one BC image subresource into the uncompressed Vulkan
   * format selected by WineHua's DXGI format fallback.  This is deliberately
   * an upload-time operation: no frame readback and no per-sample shader
   * replacement is involved.
   */
  bool DecodeD3D11BcImage(
          VkFormat                 format,
          VkExtent3D               extent,
    const void*                    source,
          VkDeviceSize             sourceRowPitch,
          VkDeviceSize             sourceSlicePitch,
          D3D11CpuImage&           result);

}
