#pragma once

#include <vulkan/vulkan.h>

class plGALDeviceVulkan;

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
#  define PLASMA_DISPATCH_CONTEXT_MEMBER_NAME(Name) m_p##Name
#else
#  define PLASMA_DISPATCH_CONTEXT_MEMBER_NAME(Name) Name
#endif

// A vulkan hpp compatible dispatch context.
class plVulkanDispatchContext
{
public:
  void Init(plGALDeviceVulkan& device);

  plUInt32 getVkHeaderVersion() const { return VK_HEADER_VERSION; }

#if PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
  // VK_KHR_external_memory_fd
  PFN_vkGetMemoryFdKHR PLASMA_DISPATCH_CONTEXT_MEMBER_NAME(vkGetMemoryFdKHR) = nullptr;
  PFN_vkGetMemoryFdPropertiesKHR PLASMA_DISPATCH_CONTEXT_MEMBER_NAME(vkGetMemoryFdPropertiesKHR) = nullptr;

  // VK_KHR_external_semaphore_fd
  PFN_vkGetSemaphoreFdKHR PLASMA_DISPATCH_CONTEXT_MEMBER_NAME(vkGetSemaphoreFdKHR) = nullptr;
  PFN_vkImportSemaphoreFdKHR PLASMA_DISPATCH_CONTEXT_MEMBER_NAME(vkImportSemaphoreFdKHR) = nullptr;
#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
  // VK_KHR_external_memory_win32
  PFN_vkGetMemoryWin32HandleKHR PLASMA_DISPATCH_CONTEXT_MEMBER_NAME(vkGetMemoryWin32HandleKHR) = nullptr;
  PFN_vkGetMemoryWin32HandlePropertiesKHR PLASMA_DISPATCH_CONTEXT_MEMBER_NAME(vkGetMemoryWin32HandlePropertiesKHR) = nullptr;

  // VK_KHR_external_semaphore_win32
  PFN_vkGetSemaphoreWin32HandleKHR PLASMA_DISPATCH_CONTEXT_MEMBER_NAME(vkGetSemaphoreWin32HandleKHR) = nullptr;
  PFN_vkImportSemaphoreWin32HandleKHR PLASMA_DISPATCH_CONTEXT_MEMBER_NAME(vkImportSemaphoreWin32HandleKHR) = nullptr;
#endif

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
#  if PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
  // VK_KHR_external_memory_fd
  VkResult vkGetMemoryFdKHR(VkDevice device, const VkMemoryGetFdInfoKHR* pGetFdInfo, int* pFd) const;
  VkResult vkGetMemoryFdPropertiesKHR(VkDevice device, VkExternalMemoryHandleTypeFlagBits handleType, int fd, VkMemoryFdPropertiesKHR* pMemoryFdProperties) const;

  // VK_KHR_external_semaphore_fd
  VkResult vkGetSemaphoreFdKHR(VkDevice device, const VkSemaphoreGetFdInfoKHR* pGetFdInfo, int* pFd) const;
  VkResult vkImportSemaphoreFdKHR(VkDevice device, const VkImportSemaphoreFdInfoKHR* pImportSemaphoreFdInfo) const;
#  elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
  // VK_KHR_external_memory_win32
  VkResult vkGetMemoryWin32HandleKHR(VkDevice device, const VkMemoryGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pWin32Handle) const;
  VkResult vkGetMemoryWin32HandlePropertiesKHR(VkDevice device, VkExternalMemoryHandleTypeFlagBits handleType, HANDLE Win32Handle, VkMemoryWin32HandlePropertiesKHR* pMemoryWin32HandleProperties) const;

  // VK_KHR_external_semaphore_win32
  VkResult vkGetSemaphoreWin32HandleKHR(VkDevice device, const VkSemaphoreGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pWin32Handle) const;
  VkResult vkImportSemaphoreWin32HandleKHR(VkDevice device, const VkImportSemaphoreWin32HandleInfoKHR* pImportSemaphoreWin32HandleInfo) const;
#  endif
#endif

private:
  plGALDeviceVulkan* m_pDevice = nullptr;
};
