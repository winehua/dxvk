#include "util_shared_res.h"

#include "winioctl.h"

#include <cstdlib>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace dxvk {

  // ---- WineHua 进程内伪共享 ----
  namespace {

    std::mutex g_fakeSharedMutex;
    std::unordered_map<HANDLE, std::vector<uint8_t>> g_fakeSharedMetadata;

    bool fakeSharedEnabledImpl() {
      // A/B 对比: 默认关闭, 显式设 1 才启用 fake-share, 验证
      // fake-share 是否是 Unity 渲染 1 帧后卡住的引入点。
      const char* v = std::getenv("DXVK_WINEHUA_FAKE_SHARED");
      return v && v[0] == '1';
    }
  }

  bool winehuaFakeSharedEnabled() {
    static const bool enabled = fakeSharedEnabledImpl();
    return enabled;
  }

  bool winehuaFakeSharedStore(HANDLE fakeHandle, const void *buf, uint32_t bufSize) {
    if (fakeHandle == INVALID_HANDLE_VALUE || !buf)
      return false;
    std::lock_guard<std::mutex> lock(g_fakeSharedMutex);
    const uint8_t* p = static_cast<const uint8_t*>(buf);
    g_fakeSharedMetadata[fakeHandle] = std::vector<uint8_t>(p, p + bufSize);
    return true;
  }

  bool winehuaFakeSharedLoad(HANDLE fakeHandle, void *buf, uint32_t bufSize, uint32_t *metadataSize) {
    std::lock_guard<std::mutex> lock(g_fakeSharedMutex);
    auto it = g_fakeSharedMetadata.find(fakeHandle);
    if (it == g_fakeSharedMetadata.end())
      return false;
    if (bufSize < it->second.size())
      return false;
    if (buf)
      std::memcpy(buf, it->second.data(), it->second.size());
    if (metadataSize)
      *metadataSize = static_cast<uint32_t>(it->second.size());
    return true;
  }

  bool winehuaFakeSharedContains(HANDLE fakeHandle) {
    std::lock_guard<std::mutex> lock(g_fakeSharedMutex);
    return g_fakeSharedMetadata.find(fakeHandle) != g_fakeSharedMetadata.end();
  }

  #define IOCTL_SHARED_GPU_RESOURCE_OPEN             CTL_CODE(FILE_DEVICE_VIDEO, 1, METHOD_BUFFERED, FILE_WRITE_ACCESS)

  HANDLE openKmtHandle(HANDLE kmt_handle) {
    HANDLE handle = ::CreateFileA("\\\\.\\SharedGpuResource", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE)
      return handle;

    struct
    {
        unsigned int kmt_handle;
        WCHAR name[1];
    } shared_resource_open = {0};
    shared_resource_open.kmt_handle = reinterpret_cast<uintptr_t>(kmt_handle);

    bool succeed = ::DeviceIoControl(handle, IOCTL_SHARED_GPU_RESOURCE_OPEN, &shared_resource_open, sizeof(shared_resource_open), NULL, 0, NULL, NULL);
    if (!succeed) {
      ::CloseHandle(handle);
      return INVALID_HANDLE_VALUE;
    }
    return handle; 
  }

  #define IOCTL_SHARED_GPU_RESOURCE_SET_METADATA           CTL_CODE(FILE_DEVICE_VIDEO, 4, METHOD_BUFFERED, FILE_WRITE_ACCESS)

  bool setSharedMetadata(HANDLE handle, void *buf, uint32_t bufSize) {
    DWORD retSize;
    return ::DeviceIoControl(handle, IOCTL_SHARED_GPU_RESOURCE_SET_METADATA, buf, bufSize, NULL, 0, &retSize, NULL);
  }

  #define IOCTL_SHARED_GPU_RESOURCE_GET_METADATA           CTL_CODE(FILE_DEVICE_VIDEO, 5, METHOD_BUFFERED, FILE_READ_ACCESS)

  bool getSharedMetadata(HANDLE handle, void *buf, uint32_t bufSize, uint32_t *metadataSize) {
    DWORD retSize;
    bool ret = ::DeviceIoControl(handle, IOCTL_SHARED_GPU_RESOURCE_GET_METADATA, NULL, 0, buf, bufSize, &retSize, NULL);
    if (metadataSize)
      *metadataSize = retSize;
    return ret;
  }

}
