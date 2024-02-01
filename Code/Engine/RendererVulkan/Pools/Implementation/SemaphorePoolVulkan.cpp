#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Pools/SemaphorePoolVulkan.h>

vk::Device plSemaphorePoolVulkan::s_device;
plHybridArray<vk::Semaphore, 4> plSemaphorePoolVulkan::s_semaphores;

void plSemaphorePoolVulkan::Initialize(vk::Device device)
{
  s_device = device;
}

void plSemaphorePoolVulkan::DeInitialize()
{
  for (vk::Semaphore& semaphore : s_semaphores)
  {
    s_device.destroySemaphore(semaphore, nullptr);
  }
  s_semaphores.Clear();
  s_semaphores.Compact();

  s_device = nullptr;
}

vk::Semaphore plSemaphorePoolVulkan::RequestSemaphore()
{
  PL_ASSERT_DEBUG(s_device, "plSemaphorePoolVulkan::Initialize not called");
  if (!s_semaphores.IsEmpty())
  {
    vk::Semaphore semaphore = s_semaphores.PeekBack();
    s_semaphores.PopBack();
    return semaphore;
  }
  else
  {
    vk::Semaphore semaphore;
    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    VK_ASSERT_DEV(s_device.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphore));
    return semaphore;
  }
}

void plSemaphorePoolVulkan::ReclaimSemaphore(vk::Semaphore& semaphore)
{
  PL_ASSERT_DEBUG(s_device, "plSemaphorePoolVulkan::Initialize not called");
  s_semaphores.PushBack(semaphore);
}
