#pragma once

#include <cstdint>

#include "./com/com_include.h"

#include <d3d11_4.h>

namespace dxvk {

    HANDLE openKmtHandle(HANDLE kmt_handle);

    bool setSharedMetadata(HANDLE handle, void *buf, uint32_t bufSize);
    bool getSharedMetadata(HANDLE handle, void *buf, uint32_t bufSize, uint32_t *metadataSize);

    // WineHua: 进程内伪共享 (Venus/Maleoon 不支持 VK_KHR_external_memory_win32 时,
    // Unity WindowsVideoMedia 等需要 D3D11 共享纹理的程序使用)。fake handle 编码
    // 同进程 DxvkImage 指针, 元数据存入进程内表, 不依赖内核 SharedGpuResource 设备。
    bool winehuaFakeSharedEnabled();
    bool winehuaFakeSharedStore(HANDLE fakeHandle, const void *buf, uint32_t bufSize);
    bool winehuaFakeSharedLoad(HANDLE fakeHandle, void *buf, uint32_t bufSize, uint32_t *metadataSize);
    bool winehuaFakeSharedContains(HANDLE fakeHandle);

    struct DxvkSharedTextureMetadata {
      UINT             Width;
      UINT             Height;
      UINT             MipLevels;
      UINT             ArraySize;
      DXGI_FORMAT      Format;
      DXGI_SAMPLE_DESC SampleDesc;
      D3D11_USAGE      Usage;
      UINT             BindFlags;
      UINT             CPUAccessFlags;
      UINT             MiscFlags;
      D3D11_TEXTURE_LAYOUT TextureLayout;
    };

}
