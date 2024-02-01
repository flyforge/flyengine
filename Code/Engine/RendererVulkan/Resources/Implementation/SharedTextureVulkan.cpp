#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/System/Process.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Resources/SharedTextureVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>

#if PL_ENABLED(PL_PLATFORM_LINUX)
#  include <errno.h>
#  include <sys/syscall.h>
#  include <unistd.h>
#endif

plGALSharedTextureVulkan::plGALSharedTextureVulkan(const plGALTextureCreationDescription& Description, plEnum<plGALSharedTextureType> sharedType, plGALPlatformSharedHandle hSharedHandle)
  : plGALTextureVulkan(Description, false, false)
  , m_SharedType(sharedType)
  , m_hSharedHandle(hSharedHandle)
{
}

plGALSharedTextureVulkan::~plGALSharedTextureVulkan()
{
}

plResult plGALSharedTextureVulkan::InitPlatform(plGALDevice* pDevice, plArrayPtr<plGALSystemMemoryDescription> pInitialData)
{
  m_pDevice = static_cast<plGALDeviceVulkan*>(pDevice);

  vk::ImageFormatListCreateInfo imageFormats;
  vk::ImageCreateInfo createInfo = {};

  m_imageFormat = ComputeImageFormat(m_pDevice, m_Description.m_Format, createInfo, imageFormats, m_bStaging);

  ComputeCreateInfo(m_pDevice, m_Description, createInfo, m_stages, m_access, m_preferredLayout);
  if (m_bLinearCPU)
  {
    ComputeCreateInfoLinear(createInfo, m_stages, m_access);
  }

  if (m_Description.m_pExisitingNativeObject == nullptr)
  {
    vk::ExternalMemoryImageCreateInfo extMemoryCreateInfo;
    if (m_SharedType == plGALSharedTextureType::Exported || m_SharedType == plGALSharedTextureType::Imported)
    {
#if PL_ENABLED(PL_PLATFORM_LINUX)
      extMemoryCreateInfo.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
#elif PL_ENABLED(PL_PLATFORM_WINDOWS)
      extMemoryCreateInfo.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32;
#endif
      extMemoryCreateInfo.pNext = createInfo.pNext;
      createInfo.pNext = &extMemoryCreateInfo;

      m_preferredLayout = vk::ImageLayout::eGeneral;
    }

    if (m_SharedType == plGALSharedTextureType::None || m_SharedType == plGALSharedTextureType::Exported)
    {

      plVulkanAllocationCreateInfo allocInfo;
      ComputeAllocInfo(m_bLinearCPU, allocInfo);

      if (m_SharedType == plGALSharedTextureType::Exported)
      {
        allocInfo.m_bExportSharedAllocation = true;
      }

      vk::ImageFormatProperties props2;
      VK_ASSERT_DEBUG(m_pDevice->GetVulkanPhysicalDevice().getImageFormatProperties(createInfo.format, createInfo.imageType, createInfo.tiling, createInfo.usage, createInfo.flags, &props2));
      VK_SUCCEED_OR_RETURN_PL_FAILURE(plMemoryAllocatorVulkan::CreateImage(createInfo, allocInfo, m_image, m_alloc, &m_allocInfo));

      if (m_SharedType == plGALSharedTextureType::Exported)
      {
        if (!m_pDevice->GetExtensions().m_bTimelineSemaphore)
        {
          plLog::Error("Can not create shared textures because timeline semaphores are not supported");
          return PL_FAILURE;
        }

#if PL_ENABLED(PL_PLATFORM_LINUX)
        if (!m_pDevice->GetExtensions().m_bExternalMemoryFd)
        {
          plLog::Error("Can not create shared textures because external memory fd is not supported");
          return PL_FAILURE;
        }

        if (!m_pDevice->GetExtensions().m_bExternalSemaphoreFd)
        {
          plLog::Error("Can not create shared textures because external semaphore fd is not supported");
          return PL_FAILURE;
        }

        vk::MemoryGetFdInfoKHR getWin32HandleInfo{m_allocInfo.m_deviceMemory, vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd};
        int fd = -1;
        vk::Device device = m_pDevice->GetVulkanDevice();
        VK_SUCCEED_OR_RETURN_PL_FAILURE(device.getMemoryFdKHR(&getWin32HandleInfo, &fd, m_pDevice->GetDispatchContext()));
        m_hSharedHandle.m_uiProcessId = plProcess::GetCurrentProcessID();
        m_hSharedHandle.m_hSharedTexture = (size_t)fd;
        m_hSharedHandle.m_uiMemoryTypeIndex = m_allocInfo.m_memoryType;
        m_hSharedHandle.m_uiSize = m_allocInfo.m_size;

        vk::ExportSemaphoreCreateInfoKHR exportInfo{vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd};
        vk::SemaphoreTypeCreateInfoKHR semTypeCreateInfo{vk::SemaphoreType::eTimeline, 0, &exportInfo};
        vk::SemaphoreCreateInfo semCreateInfo{{}, &semTypeCreateInfo};
        m_SharedSemaphore = device.createSemaphore(semCreateInfo);

        int semaphoreFd = -1;
        vk::SemaphoreGetFdInfoKHR getSemaphoreWin32Info{m_SharedSemaphore, vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd};
        VK_SUCCEED_OR_RETURN_PL_FAILURE(device.getSemaphoreFdKHR(&getSemaphoreWin32Info, &semaphoreFd, m_pDevice->GetDispatchContext()));
        m_hSharedHandle.m_hSemaphore = (size_t)semaphoreFd;
#elif PL_ENABLED(PL_PLATFORM_WINDOWS)
        if (!m_pDevice->GetExtensions().m_bExternalMemoryWin32)
        {
          plLog::Error("Can not create shared textures because external memory win32 is not supported");
          return PL_FAILURE;
        }

        if (!m_pDevice->GetExtensions().m_bExternalSemaphoreWin32)
        {
          plLog::Error("Can not create shared textures because external semaphore win32 is not supported");
          return PL_FAILURE;
        }

        vk::Device device = m_pDevice->GetVulkanDevice();
        vk::MemoryGetWin32HandleInfoKHR getWin32HandleInfo{m_allocInfo.m_deviceMemory, vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32};
        HANDLE handle = 0;
        VK_SUCCEED_OR_RETURN_PL_FAILURE(device.getMemoryWin32HandleKHR(&getWin32HandleInfo, &handle, m_pDevice->GetDispatchContext()));
        m_hSharedHandle.m_uiProcessId = plProcess::GetCurrentProcessID();
        m_hSharedHandle.m_hSharedTexture = (size_t)handle;
        m_hSharedHandle.m_uiMemoryTypeIndex = m_allocInfo.m_memoryType;
        m_hSharedHandle.m_uiSize = m_allocInfo.m_size;

        vk::ExportSemaphoreWin32HandleInfoKHR exportInfoWin32;
        exportInfoWin32.dwAccess = GENERIC_ALL;
        vk::ExportSemaphoreCreateInfoKHR exportInfo{vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32, &exportInfoWin32};
        vk::SemaphoreTypeCreateInfoKHR semTypeCreateInfo{vk::SemaphoreType::eTimeline, 0, &exportInfo};
        vk::SemaphoreCreateInfo semCreateInfo{{}, &semTypeCreateInfo};
        m_SharedSemaphore = device.createSemaphore(semCreateInfo);

        HANDLE semaphoreHandle = 0;
        vk::SemaphoreGetWin32HandleInfoKHR getSemaphoreWin32Info{m_SharedSemaphore, vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32};
        VK_SUCCEED_OR_RETURN_PL_FAILURE(device.getSemaphoreWin32HandleKHR(&getSemaphoreWin32Info, &semaphoreHandle, m_pDevice->GetDispatchContext()));
        m_hSharedHandle.m_hSemaphore = (size_t)semaphoreHandle;
#else
        PL_ASSERT_NOT_IMPLEMENTED
#endif
      }
    }
    else if (m_SharedType == plGALSharedTextureType::Imported)
    {
      if (!m_pDevice->GetExtensions().m_bTimelineSemaphore)
      {
        plLog::Error("Can not open shared texture: timeline semaphores not supported");
        return PL_FAILURE;
      }

#if PL_ENABLED(PL_PLATFORM_LINUX)
      if (m_hSharedHandle.m_hSharedTexture == 0 || m_hSharedHandle.m_hSemaphore == 0)
      {
        plLog::Error("Can not open shared texture: invalid handle given");
        return PL_FAILURE;
      }


      if (!m_pDevice->GetExtensions().m_bExternalMemoryFd)
      {
        plLog::Error("Can not open shared texture: external memory fd not supported");
        return PL_FAILURE;
      }

      if (!m_pDevice->GetExtensions().m_bExternalSemaphoreFd)
      {
        plLog::Error("Can not open shared texture: external semaphore fd not supported");
        return PL_FAILURE;
      }

      bool bNeedToImportForeignProcessFileDescriptors = m_hSharedHandle.m_uiProcessId != plProcess::GetCurrentProcessID();
      if (bNeedToImportForeignProcessFileDescriptors)
      {
        int processFd = syscall(SYS_pidfd_open, m_hSharedHandle.m_uiProcessId, 0);
        if (processFd == -1)
        {
          plLog::Error("SYS_pidfd_open failed with errno: {}", plArgErrno(errno));
          m_hSharedHandle.m_hSharedTexture = 0;
          m_hSharedHandle.m_hSemaphore = 0;
          return PL_FAILURE;
        }
        PL_SCOPE_EXIT(close(processFd));

        m_hSharedHandle.m_hSharedTexture = syscall(SYS_pidfd_getfd, processFd, m_hSharedHandle.m_hSharedTexture, 0);
        if (m_hSharedHandle.m_hSharedTexture == -1)
        {
          plLog::Error("SYS_pidfd_getfd for texture failed with errno: {}", plArgErrno(errno));
          m_hSharedHandle.m_hSharedTexture = 0;
          m_hSharedHandle.m_hSemaphore = 0;
          return PL_FAILURE;
        }

        m_hSharedHandle.m_hSemaphore = syscall(SYS_pidfd_getfd, processFd, m_hSharedHandle.m_hSemaphore, 0);
        if (m_hSharedHandle.m_hSemaphore == -1)
        {
          plLog::Error("SYS_pidfd_getfd for semaphore failed with errno: {}", plArgErrno(errno));
          m_hSharedHandle.m_hSemaphore = 0;
          return PL_FAILURE;
        }
      }

      vk::Device device = m_pDevice->GetVulkanDevice();

      // Import semaphore
      vk::SemaphoreTypeCreateInfoKHR semTypeCreateInfo{vk::SemaphoreType::eTimeline, 0};
      vk::SemaphoreCreateInfo semCreateInfo{{}, &semTypeCreateInfo};
      m_SharedSemaphore = device.createSemaphore(semCreateInfo);

      vk::ImportSemaphoreFdInfoKHR importSemaphoreInfo{m_SharedSemaphore, {}, vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd, static_cast<int>(m_hSharedHandle.m_hSemaphore)};
      VK_SUCCEED_OR_RETURN_PL_FAILURE(device.importSemaphoreFdKHR(&importSemaphoreInfo, m_pDevice->GetDispatchContext()));

      // Create Image
      VK_SUCCEED_OR_RETURN_PL_FAILURE(device.createImage(&createInfo, nullptr, &m_image));

      vk::ImageMemoryRequirementsInfo2 imageRequirementsInfo{m_image};
      vk::MemoryRequirements2 imageMemoryRequirements;
      device.getImageMemoryRequirements2(&imageRequirementsInfo, &imageMemoryRequirements);

      // Import memory
      PL_ASSERT_DEBUG(imageMemoryRequirements.memoryRequirements.size == m_hSharedHandle.m_uiSize, "");

      vk::ImportMemoryFdInfoKHR fdInfo{vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd, static_cast<int>(m_hSharedHandle.m_hSharedTexture)};
      vk::MemoryAllocateInfo allocateInfo{imageMemoryRequirements.memoryRequirements.size, m_hSharedHandle.m_uiMemoryTypeIndex, &fdInfo};

      m_allocInfo = {};
      VK_SUCCEED_OR_RETURN_PL_FAILURE(device.allocateMemory(&allocateInfo, nullptr, &m_allocInfo.m_deviceMemory));
      m_allocInfo.m_offset = 0;
      m_allocInfo.m_size = imageMemoryRequirements.memoryRequirements.size;
      m_allocInfo.m_memoryType = m_hSharedHandle.m_uiMemoryTypeIndex;

      device.bindImageMemory(m_image, m_allocInfo.m_deviceMemory, 0);
#elif PL_ENABLED(PL_PLATFORM_WINDOWS)
      if (m_hSharedHandle.m_hSharedTexture == 0 || m_hSharedHandle.m_hSemaphore == 0)
      {
        plLog::Error("Can not open shared texture: invalid handle given");
        return PL_FAILURE;
      }


      if (!m_pDevice->GetExtensions().m_bExternalMemoryWin32)
      {
        plLog::Error("Can not open shared texture: external memory win32 not supported");
        return PL_FAILURE;
      }

      if (!m_pDevice->GetExtensions().m_bExternalSemaphoreWin32)
      {
        plLog::Error("Can not open shared texture: external semaphore win32 not supported");
        return PL_FAILURE;
      }

      bool bNeedToImportForeignProcessFileDescriptors = m_hSharedHandle.m_uiProcessId != plProcess::GetCurrentProcessID();
      if (bNeedToImportForeignProcessFileDescriptors)
      {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, m_hSharedHandle.m_uiProcessId);
        if (hProcess == 0)
        {
          plLog::Error("OpenProcess failed with error: {}", plArgErrorCode(GetLastError()));
          m_hSharedHandle.m_hSharedTexture = 0;
          m_hSharedHandle.m_hSemaphore = 0;
          return PL_FAILURE;
        }

        HANDLE duplicateA = 0;
        BOOL res = DuplicateHandle(hProcess, reinterpret_cast<HANDLE>(m_hSharedHandle.m_hSharedTexture), GetCurrentProcess(), &duplicateA, 0, FALSE, DUPLICATE_SAME_ACCESS);
        m_hSharedHandle.m_hSharedTexture = reinterpret_cast<plUInt64>(duplicateA);
        if (res == FALSE)
        {
          plLog::Error("DuplicateHandle failed with error: {}", plArgErrorCode(GetLastError()));
          m_hSharedHandle.m_hSharedTexture = 0;
          m_hSharedHandle.m_hSemaphore = 0;
          return PL_FAILURE;
        }

        HANDLE duplicateB = 0;
        res = DuplicateHandle(hProcess, reinterpret_cast<HANDLE>(m_hSharedHandle.m_hSemaphore), GetCurrentProcess(), &duplicateB, 0, FALSE, DUPLICATE_SAME_ACCESS);
        m_hSharedHandle.m_hSemaphore = reinterpret_cast<plUInt64>(duplicateB);
        if (res == FALSE)
        {
          plLog::Error("DuplicateHandle failed with error: {}", plArgErrorCode(GetLastError()));
          m_hSharedHandle.m_hSemaphore = 0;
          return PL_FAILURE;
        }
      }

      vk::Device device = m_pDevice->GetVulkanDevice();

      // Import semaphore
      vk::SemaphoreTypeCreateInfoKHR semTypeCreateInfo{vk::SemaphoreType::eTimeline, 0};
      vk::SemaphoreCreateInfo semCreateInfo{{}, &semTypeCreateInfo};
      m_SharedSemaphore = device.createSemaphore(semCreateInfo);

      vk::ImportSemaphoreWin32HandleInfoKHR importSemaphoreInfo{m_SharedSemaphore, {}, vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueWin32, reinterpret_cast<HANDLE>(m_hSharedHandle.m_hSemaphore)};
      vk::Result res = device.importSemaphoreWin32HandleKHR(&importSemaphoreInfo, m_pDevice->GetDispatchContext());
      VK_SUCCEED_OR_RETURN_PL_FAILURE(res);

      // Create Image
      VK_SUCCEED_OR_RETURN_PL_FAILURE(device.createImage(&createInfo, nullptr, &m_image));

      vk::ImageMemoryRequirementsInfo2 imageRequirementsInfo{m_image};
      vk::MemoryRequirements2 imageMemoryRequirements;
      device.getImageMemoryRequirements2(&imageRequirementsInfo, &imageMemoryRequirements);

      // Import memory
      PL_ASSERT_DEBUG(imageMemoryRequirements.memoryRequirements.size == m_hSharedHandle.m_uiSize, "");

      vk::ImportMemoryWin32HandleInfoKHR fdInfo{vk::ExternalMemoryHandleTypeFlagBits::eOpaqueWin32, reinterpret_cast<HANDLE>(m_hSharedHandle.m_hSharedTexture)};
      vk::MemoryAllocateInfo allocateInfo{imageMemoryRequirements.memoryRequirements.size, m_hSharedHandle.m_uiMemoryTypeIndex, &fdInfo};

      m_allocInfo = {};
      VK_SUCCEED_OR_RETURN_PL_FAILURE(device.allocateMemory(&allocateInfo, nullptr, &m_allocInfo.m_deviceMemory));
      m_allocInfo.m_offset = 0;
      m_allocInfo.m_size = imageMemoryRequirements.memoryRequirements.size;
      m_allocInfo.m_memoryType = m_hSharedHandle.m_uiMemoryTypeIndex;

      device.bindImageMemory(m_image, m_allocInfo.m_deviceMemory, 0);
#else
      PL_ASSERT_NOT_IMPLEMENTED
#endif
    }
  }
  else
  {
    m_image = static_cast<VkImage>(m_Description.m_pExisitingNativeObject);
  }
  m_pDevice->GetInitContext().InitTexture(this, createInfo, pInitialData);

  if (m_Description.m_ResourceAccess.m_bReadBack)
  {
    return CreateStagingBuffer(createInfo);
  }

  return PL_SUCCESS;
}


plResult plGALSharedTextureVulkan::DeInitPlatform(plGALDevice* pDevice)
{
  plGALDeviceVulkan* pVulkanDevice = static_cast<plGALDeviceVulkan*>(pDevice);

  if (m_SharedType == plGALSharedTextureType::Imported)
  {
    pVulkanDevice->DeleteLater(m_image, m_allocInfo.m_deviceMemory);
    pVulkanDevice->DeleteLater(m_SharedSemaphore);
  }

  auto res = SUPER::DeInitPlatform(pDevice);

#if PL_ENABLED(PL_PLATFORM_LINUX)
  if (m_hSharedHandle.m_hSharedTexture != 0)
  {
    pVulkanDevice->DeleteLaterImpl({vk::ObjectType::eUnknown, {plGALDeviceVulkan::PendingDeletionFlags::IsFileDescriptor}, (void*)static_cast<size_t>(m_hSharedHandle.m_hSharedTexture), nullptr});
    m_hSharedHandle.m_hSharedTexture = 0;
  }
  if (m_hSharedHandle.m_hSemaphore != 0)
  {
    pVulkanDevice->DeleteLaterImpl({vk::ObjectType::eUnknown, {plGALDeviceVulkan::PendingDeletionFlags::IsFileDescriptor}, (void*)static_cast<size_t>(m_hSharedHandle.m_hSemaphore), nullptr});
    m_hSharedHandle.m_hSemaphore = 0;
  }
#endif
  return res;
}

plGALPlatformSharedHandle plGALSharedTextureVulkan::GetSharedHandle() const
{
  return m_hSharedHandle;
}

void plGALSharedTextureVulkan::WaitSemaphoreGPU(plUInt64 uiValue) const
{
  m_pDevice->AddWaitSemaphore(plGALDeviceVulkan::SemaphoreInfo::MakeWaitSemaphore(m_SharedSemaphore, vk::PipelineStageFlagBits::eAllCommands, vk::SemaphoreType::eTimeline, uiValue));
}

void plGALSharedTextureVulkan::SignalSemaphoreGPU(plUInt64 uiValue) const
{
  m_pDevice->AddSignalSemaphore(plGALDeviceVulkan::SemaphoreInfo::MakeSignalSemaphore(m_SharedSemaphore, vk::SemaphoreType::eTimeline, uiValue));
  // TODO, transition texture into GENERAL layout
  m_pDevice->GetCurrentPipelineBarrier().EnsureImageLayout(this, GetPreferredLayout(), GetUsedByPipelineStage(), GetAccessMask());
}

PL_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_SharedTextureVulkan);
