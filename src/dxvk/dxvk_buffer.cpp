#include "dxvk_barrier.h"
#include "dxvk_buffer.h"
#include "dxvk_cmdlist.h"
#include "dxvk_device.h"

#include "../util/util_winehua_api_trace.h"

#include <algorithm>

namespace dxvk {
  
  DxvkBuffer::DxvkBuffer(
          DxvkDevice*           device,
    const DxvkBufferCreateInfo& createInfo,
          DxvkMemoryAllocator&  allocator,
          VkMemoryPropertyFlags memFlags)
  : m_vkd           (device->vkd()),
    m_allocator     (&allocator),
    m_properties    (memFlags),
    m_nonCoherentAtomSize(device->properties().core.properties.limits.nonCoherentAtomSize),
    m_shaderStages  (util::shaderStages(createInfo.stages)),
    m_sharingMode   (device->getSharingMode()),
    m_info          (createInfo),
    m_forceMappedFlush(device->adapter()->isWineHuaVenus()) {
    m_allocator->registerResource(this);

    // Assign debug name to buffer
    if (device->debugFlags().test(DxvkDebugFlag::Capture)) {
      m_debugName = createDebugName(createInfo.debugName);
      m_info.debugName = m_debugName.c_str();
    } else {
      m_info.debugName = nullptr;
    }

    // Create and assign actual buffer resource
    assignStorage(allocateStorage());
  }


  DxvkBuffer::DxvkBuffer(
          DxvkDevice*           device,
    const DxvkBufferCreateInfo& createInfo,
    const DxvkBufferImportInfo& importInfo,
          DxvkMemoryAllocator&  allocator,
          VkMemoryPropertyFlags memFlags)
  : m_vkd           (device->vkd()),
    m_allocator     (&allocator),
    m_properties    (memFlags),
    m_nonCoherentAtomSize(device->properties().core.properties.limits.nonCoherentAtomSize),
    m_shaderStages  (util::shaderStages(createInfo.stages)),
    m_sharingMode   (device->getSharingMode()),
    m_info          (createInfo),
    m_stableAddress (true),
    m_forceMappedFlush(device->adapter()->isWineHuaVenus()) {
    m_allocator->registerResource(this);

    DxvkAllocationInfo allocationInfo = { };
    allocationInfo.resourceCookie = cookie();

    VkBufferCreateInfo info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    info.flags = m_info.flags;
    info.usage = m_info.usage;
    info.size = m_info.size;
    m_sharingMode.fill(info);

    assignStorage(allocator.importBufferResource(info, allocationInfo, importInfo));
  }


  DxvkBuffer::~DxvkBuffer() {
    m_allocator->unregisterResource(this);
  }


  bool DxvkBuffer::canRelocate() const {
    return !m_bufferInfo.mapPtr && !m_stableAddress
        && !(m_info.flags & VK_BUFFER_CREATE_SPARSE_BINDING_BIT);
  }


  VkResult DxvkBuffer::flushMappedSlice(
    const DxvkBufferSliceHandle& slice,
          DxvkCommandList*       commandList) const {
    return flushMappedSlice(m_storage, slice, commandList);
  }


  VkResult DxvkBuffer::flushMappedSlice(
    const Rc<DxvkResourceAllocation>& storage,
    const DxvkBufferSliceHandle&      slice,
          DxvkCommandList*            commandList) const {
    if (!m_forceMappedFlush || !slice.length)
      return VK_SUCCESS;

    if (!storage)
      return VK_ERROR_MEMORY_MAP_FAILED;

    const auto bufferInfo = storage->getBufferInfo();
    const auto memoryInfo = storage->getMemoryInfo();

    if (!memoryInfo.memory || !bufferInfo.mapPtr
     || slice.handle != bufferInfo.buffer
     || slice.offset < bufferInfo.offset)
      return VK_ERROR_MEMORY_MAP_FAILED;

    const VkDeviceSize sliceOffset = slice.offset - bufferInfo.offset;
    if (sliceOffset > memoryInfo.size || slice.length > memoryInfo.size - sliceOffset)
      return VK_ERROR_MEMORY_MAP_FAILED;

    const VkDeviceSize atom = std::max(VkDeviceSize(1u), m_nonCoherentAtomSize);
    const VkDeviceSize sliceBegin = memoryInfo.offset + sliceOffset;
    const VkDeviceSize rangeOffset = (sliceBegin / atom) * atom;
    const VkDeviceSize rangeEnd = align(sliceBegin + slice.length, atom);

    VkMappedMemoryRange range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
    range.memory = memoryInfo.memory;
    range.offset = rangeOffset;
    range.size = rangeEnd - rangeOffset;

    const VkResult result = winehuaBatchMappedFlushEnabled() && commandList
      ? commandList->queueWineHuaMappedFlush(storage, range)
      : m_vkd->vkFlushMappedMemoryRanges(m_vkd->device(), 1, &range);

    static std::atomic<uint64_t> mappedTraceCounter { 0 };
    uint64_t traceSequence = 0;
    if (winehuaMappedTraceSample(mappedTraceCounter, traceSequence)) {
      Logger::info(str::format(
        "WineHua mapped-trace: vkFlush#", traceSequence,
        " storage=0x", std::hex, reinterpret_cast<uintptr_t>(storage.ptr()),
        " buffer=0x", slice.handle,
        " bufferInfoOffset=", std::dec, bufferInfo.offset,
        " sliceOffset=", slice.offset,
        " sliceLength=", slice.length,
        " localOffset=", sliceOffset,
        " memory=0x", std::hex, memoryInfo.memory,
        " memoryOffset=", std::dec, memoryInfo.offset,
        " memorySize=", memoryInfo.size,
        " rangeOffset=", range.offset,
        " rangeSize=", range.size,
        " atom=", atom,
        " result=", int32_t(result)));
    }

    return result;
  }


  VkResult DxvkBuffer::invalidateMappedSlice(
    const DxvkBufferSliceHandle& slice) const {
    if (!m_forceMappedFlush || !slice.length)
      return VK_SUCCESS;

    const auto bufferInfo = m_storage->getBufferInfo();
    const auto memoryInfo = m_storage->getMemoryInfo();

    if (!memoryInfo.memory || !bufferInfo.mapPtr
     || slice.handle != bufferInfo.buffer
     || slice.offset < bufferInfo.offset)
      return VK_ERROR_MEMORY_MAP_FAILED;

    const VkDeviceSize sliceOffset = slice.offset - bufferInfo.offset;
    if (sliceOffset > memoryInfo.size || slice.length > memoryInfo.size - sliceOffset)
      return VK_ERROR_MEMORY_MAP_FAILED;

    const VkDeviceSize atom = std::max(VkDeviceSize(1u), m_nonCoherentAtomSize);
    const VkDeviceSize sliceBegin = memoryInfo.offset + sliceOffset;
    const VkDeviceSize rangeOffset = (sliceBegin / atom) * atom;
    const VkDeviceSize rangeEnd = align(sliceBegin + slice.length, atom);

    VkMappedMemoryRange range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
    range.memory = memoryInfo.memory;
    range.offset = rangeOffset;
    range.size = rangeEnd - rangeOffset;

    return m_vkd->vkInvalidateMappedMemoryRanges(m_vkd->device(), 1, &range);
  }


  Rc<DxvkBufferView> DxvkBuffer::createView(
    const DxvkBufferViewKey& info) {
    std::unique_lock lock(m_viewMutex);

    auto entry = m_views.emplace(std::piecewise_construct,
      std::make_tuple(info), std::make_tuple(this, info));

    return &entry.first->second;
  }


  DxvkSparsePageTable* DxvkBuffer::getSparsePageTable() {
    return m_storage->getSparsePageTable();
  }


  Rc<DxvkResourceAllocation> DxvkBuffer::relocateStorage(
          DxvkAllocationModes         mode) {
    // The resource may become non-relocatable even after we allocate new
    // backing storage, but if it already is then don't waste memory.
    if (!canRelocate())
      return nullptr;

    DxvkAllocationInfo allocationInfo = { };
    allocationInfo.resourceCookie = cookie();
    allocationInfo.properties = m_properties;
    allocationInfo.mode = mode;

    VkBufferCreateInfo info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    info.flags = m_info.flags;
    info.usage = m_info.usage;
    info.size = m_info.size;
    m_sharingMode.fill(info);

    return m_allocator->createBufferResource(info, allocationInfo, nullptr);
  }


  void DxvkBuffer::setDebugName(const char* name) {
    if (likely(!m_info.debugName))
      return;

    m_debugName = createDebugName(name);
    m_info.debugName = m_debugName.c_str();

    updateDebugName();
  }


  void DxvkBuffer::updateDebugName() {
    if (m_storage->flags().test(DxvkAllocationFlag::OwnsBuffer)) {
      VkDebugUtilsObjectNameInfoEXT nameInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
      nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
      nameInfo.objectHandle = vk::getObjectHandle(m_bufferInfo.buffer);
      nameInfo.pObjectName = m_info.debugName;

      m_vkd->vkSetDebugUtilsObjectNameEXT(m_vkd->device(), &nameInfo);
    }
  }


  std::string DxvkBuffer::createDebugName(const char* name) const {
    return str::format(vk::isValidDebugName(name) ? name : "Buffer", " (", cookie(), ")");
  }

}
