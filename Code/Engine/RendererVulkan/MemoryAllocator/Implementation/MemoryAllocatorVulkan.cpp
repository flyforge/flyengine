#include <RendererVulkan/RendererVulkanPCH.h>


VKAPI_ATTR void VKAPI_CALL vkGetDeviceBufferMemoryRequirements(
  VkDevice device,
  const VkDeviceBufferMemoryRequirements* pInfo,
  VkMemoryRequirements2* pMemoryRequirements)
{
  PLASMA_REPORT_FAILURE("FIXME: Added to prevent the error: The procedure entry point vkGetDeviceBufferMemoryRequirements could not be located in the dynamic link library plRendererVulkan.dll.");
}

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/Types/UniquePtr.h>

#define VMA_VULKAN_VERSION 1001000
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_STATS_STRING_ENABLED 1


//
// #define VMA_DEBUG_LOG(format, ...)   \
//  do                                 \
//  {                                  \
//    plStringBuilder tmp;             \
//    tmp.Printf(format, __VA_ARGS__); \
//    plLog::Error("{}", tmp);         \
//  } while (false)

#include <RendererVulkan/MemoryAllocator/MemoryAllocatorVulkan.h>

#define VMA_IMPLEMENTATION

#ifndef VA_IGNORE_THIS_FILE
#  define VA_INCLUDE_HIDDEN <vma/vk_mem_alloc.h>
#else
#  define VA_INCLUDE_HIDDEN ""
#endif

#include VA_INCLUDE_HIDDEN

PLASMA_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT == plVulkanAllocationCreateFlags::DedicatedMemory);
PLASMA_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT == plVulkanAllocationCreateFlags::NeverAllocate);
PLASMA_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_MAPPED_BIT == plVulkanAllocationCreateFlags::Mapped);
PLASMA_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT == plVulkanAllocationCreateFlags::CanAlias);
PLASMA_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT == plVulkanAllocationCreateFlags::HostAccessSequentialWrite);
PLASMA_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT == plVulkanAllocationCreateFlags::HostAccessRandom);
PLASMA_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT == plVulkanAllocationCreateFlags::StrategyMinMemory);
PLASMA_CHECK_AT_COMPILETIME(VMA_ALLOCATION_CREATE_STRATEGY_MIN_TIME_BIT == plVulkanAllocationCreateFlags::StrategyMinTime);

PLASMA_CHECK_AT_COMPILETIME(VMA_MEMORY_USAGE_UNKNOWN == plVulkanMemoryUsage::Unknown);
PLASMA_CHECK_AT_COMPILETIME(VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED == plVulkanMemoryUsage::GpuLazilyAllocated);
PLASMA_CHECK_AT_COMPILETIME(VMA_MEMORY_USAGE_AUTO == plVulkanMemoryUsage::Auto);
PLASMA_CHECK_AT_COMPILETIME(VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE == plVulkanMemoryUsage::AutoPreferDevice);
PLASMA_CHECK_AT_COMPILETIME(VMA_MEMORY_USAGE_AUTO_PREFER_HOST == plVulkanMemoryUsage::AutoPreferHost);

PLASMA_CHECK_AT_COMPILETIME(sizeof(plVulkanAllocation) == sizeof(VmaAllocation));

PLASMA_CHECK_AT_COMPILETIME(sizeof(plVulkanAllocationInfo) == sizeof(VmaAllocationInfo));

PLASMA_DEFINE_AS_POD_TYPE(VkExportMemoryAllocateInfo);

namespace
{
  struct ExportedSharedPool
  {
    VmaPool m_pool = nullptr;
    plUniquePtr<vk::ExportMemoryAllocateInfo> m_exportInfo; // must outlive the pool and remain at the same address.
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
    plUniquePtr<vk::ExportMemoryWin32HandleInfoKHR> m_exportInfoWin32;
#endif
  };
} // namespace

struct plMemoryAllocatorVulkan::Impl
{
  VmaAllocator m_allocator;
  plMutex m_exportedSharedPoolsMutex;
  plHashTable<uint32_t, ExportedSharedPool> m_exportedSharedPools;
};

plMemoryAllocatorVulkan::Impl* plMemoryAllocatorVulkan::m_pImpl = nullptr;

vk::Result plMemoryAllocatorVulkan::Initialize(vk::PhysicalDevice physicalDevice, vk::Device device, vk::Instance instance)
{
  PLASMA_ASSERT_DEV(m_pImpl == nullptr, "plMemoryAllocatorVulkan::Initialize was already called");
  m_pImpl = PLASMA_DEFAULT_NEW(Impl);

  VmaVulkanFunctions vulkanFunctions = {};
  vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
  vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

  VmaAllocatorCreateInfo allocatorCreateInfo = {};
  allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_1;
  allocatorCreateInfo.physicalDevice = physicalDevice;
  allocatorCreateInfo.device = device;
  allocatorCreateInfo.instance = instance;
  allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

  vk::Result res = (vk::Result)vmaCreateAllocator(&allocatorCreateInfo, &m_pImpl->m_allocator);
  if (res != vk::Result::eSuccess)
  {
    PLASMA_DEFAULT_DELETE(m_pImpl);
  }

  return res;
}

void plMemoryAllocatorVulkan::DeInitialize()
{
  PLASMA_ASSERT_DEV(m_pImpl != nullptr, "plMemoryAllocatorVulkan is not initialized.");

  for (auto it : m_pImpl->m_exportedSharedPools)
  {
    vmaDestroyPool(m_pImpl->m_allocator, it.Value().m_pool);
  }
  m_pImpl->m_exportedSharedPools.Clear();

  // Uncomment below to debug leaks in VMA.
  /*
  char* pStats = nullptr;
  vmaBuildStatsString(m_pImpl->m_allocator, &pStats, true);
  */
  vmaDestroyAllocator(m_pImpl->m_allocator);
  PLASMA_DEFAULT_DELETE(m_pImpl);
}

vk::Result plMemoryAllocatorVulkan::CreateImage(const vk::ImageCreateInfo& imageCreateInfo, const plVulkanAllocationCreateInfo& allocationCreateInfo, vk::Image& out_image, plVulkanAllocation& out_alloc, plVulkanAllocationInfo* pAllocInfo)
{
  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = (VmaMemoryUsage)allocationCreateInfo.m_usage.GetValue();
  allocCreateInfo.flags = allocationCreateInfo.m_flags.GetValue() | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
  allocCreateInfo.pUserData = (void*)allocationCreateInfo.m_pUserData;

  if (allocationCreateInfo.m_bExportSharedAllocation)
  {
    allocCreateInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    PLASMA_LOCK(m_pImpl->m_exportedSharedPoolsMutex);

    uint32_t memoryTypeIndex = 0;
    if (auto res = vmaFindMemoryTypeIndexForImageInfo(m_pImpl->m_allocator, reinterpret_cast<const VkImageCreateInfo*>(&imageCreateInfo), &allocCreateInfo, &memoryTypeIndex); res != VK_SUCCESS)
    {
      return (vk::Result)res;
    }

    ExportedSharedPool* pool = m_pImpl->m_exportedSharedPools.GetValue(memoryTypeIndex);
    if (pool == nullptr)
    {
      ExportedSharedPool newPool;
      {
        newPool.m_exportInfo = PLASMA_DEFAULT_NEW(vk::ExportMemoryAllocateInfo);
        vk::ExportMemoryAllocateInfo& exportInfo = *newPool.m_exportInfo.Borrow();
#if PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
        exportInfo.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
        newPool.m_exportInfoWin32 = PLASMA_DEFAULT_NEW(vk::ExportMemoryWin32HandleInfoKHR);
        vk::ExportMemoryWin32HandleInfoKHR& exportInfoWin = *newPool.m_exportInfoWin32.Borrow();
        exportInfoWin.dwAccess = GENERIC_ALL;

        exportInfo.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
        exportInfo.pNext = &exportInfoWin;
#else
        PLASMA_ASSERT_NOT_IMPLEMENTED
#endif
      }

      VmaPoolCreateInfo poolCreateInfo = {};
      poolCreateInfo.memoryTypeIndex = memoryTypeIndex;
      poolCreateInfo.pMemoryAllocateNext = newPool.m_exportInfo.Borrow();

      if (auto res = vmaCreatePool(m_pImpl->m_allocator, &poolCreateInfo, &newPool.m_pool); res != VK_SUCCESS)
      {
        return (vk::Result)res;
      }
      m_pImpl->m_exportedSharedPools.Insert(memoryTypeIndex, std::move(newPool));
      pool = m_pImpl->m_exportedSharedPools.GetValue(memoryTypeIndex);
    }

    allocCreateInfo.pool = pool->m_pool;
  }

  return (vk::Result)vmaCreateImage(m_pImpl->m_allocator, reinterpret_cast<const VkImageCreateInfo*>(&imageCreateInfo), &allocCreateInfo, reinterpret_cast<VkImage*>(&out_image), reinterpret_cast<VmaAllocation*>(&out_alloc), reinterpret_cast<VmaAllocationInfo*>(pAllocInfo));
}

void plMemoryAllocatorVulkan::DestroyImage(vk::Image& image, plVulkanAllocation& alloc)
{
  vmaSetAllocationUserData(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), nullptr);
  vmaDestroyImage(m_pImpl->m_allocator, reinterpret_cast<VkImage&>(image), reinterpret_cast<VmaAllocation&>(alloc));
  image = nullptr;
  alloc = nullptr;
}

vk::Result plMemoryAllocatorVulkan::CreateBuffer(const vk::BufferCreateInfo& bufferCreateInfo, const plVulkanAllocationCreateInfo& allocationCreateInfo, vk::Buffer& out_buffer, plVulkanAllocation& out_alloc, plVulkanAllocationInfo* pAllocInfo)
{
  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = (VmaMemoryUsage)allocationCreateInfo.m_usage.GetValue();
  allocCreateInfo.flags = allocationCreateInfo.m_flags.GetValue() | VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT;
  allocCreateInfo.pUserData = (void*)allocationCreateInfo.m_pUserData;

  return (vk::Result)vmaCreateBuffer(m_pImpl->m_allocator, reinterpret_cast<const VkBufferCreateInfo*>(&bufferCreateInfo), &allocCreateInfo, reinterpret_cast<VkBuffer*>(&out_buffer), reinterpret_cast<VmaAllocation*>(&out_alloc), reinterpret_cast<VmaAllocationInfo*>(pAllocInfo));
}

void plMemoryAllocatorVulkan::DestroyBuffer(vk::Buffer& buffer, plVulkanAllocation& alloc)
{
  vmaSetAllocationUserData(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), nullptr);
  vmaDestroyBuffer(m_pImpl->m_allocator, reinterpret_cast<VkBuffer&>(buffer), reinterpret_cast<VmaAllocation&>(alloc));
  buffer = nullptr;
  alloc = nullptr;
}

plVulkanAllocationInfo plMemoryAllocatorVulkan::GetAllocationInfo(plVulkanAllocation alloc)
{
  VmaAllocationInfo info;
  vmaGetAllocationInfo(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), &info);

  return reinterpret_cast<plVulkanAllocationInfo&>(info);
}

void plMemoryAllocatorVulkan::SetAllocationUserData(plVulkanAllocation alloc, const char* pUserData)
{
  vmaSetAllocationUserData(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), (void*)pUserData);
}

vk::Result plMemoryAllocatorVulkan::MapMemory(plVulkanAllocation alloc, void** pData)
{
  return (vk::Result)vmaMapMemory(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), pData);
}

void plMemoryAllocatorVulkan::UnmapMemory(plVulkanAllocation alloc)
{
  vmaUnmapMemory(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc));
}

vk::Result plMemoryAllocatorVulkan::FlushAllocation(plVulkanAllocation alloc, vk::DeviceSize offset, vk::DeviceSize size)
{
  return (vk::Result)vmaFlushAllocation(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), offset, size);
}

vk::Result plMemoryAllocatorVulkan::InvalidateAllocation(plVulkanAllocation alloc, vk::DeviceSize offset, vk::DeviceSize size)
{
  return (vk::Result)vmaInvalidateAllocation(m_pImpl->m_allocator, reinterpret_cast<VmaAllocation&>(alloc), offset, size);
}
