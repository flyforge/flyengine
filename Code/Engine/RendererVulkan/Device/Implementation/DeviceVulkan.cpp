#include <RendererVulkan/RendererVulkanPCH.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

#include <Core/System/Window.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/DeviceFactory.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/RendererReflection.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/CommandEncoder/CommandEncoderImplVulkan.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Device/InitContext.h>
#include <RendererVulkan/Device/PassVulkan.h>
#include <RendererVulkan/Device/SwapChainVulkan.h>
#include <RendererVulkan/Pools/CommandBufferPoolVulkan.h>
#include <RendererVulkan/Pools/DescriptorSetPoolVulkan.h>
#include <RendererVulkan/Pools/FencePoolVulkan.h>
#include <RendererVulkan/Pools/QueryPoolVulkan.h>
#include <RendererVulkan/Pools/SemaphorePoolVulkan.h>
#include <RendererVulkan/Pools/StagingBufferPoolVulkan.h>
#include <RendererVulkan/Resources/BufferVulkan.h>
#include <RendererVulkan/Resources/FallbackResourcesVulkan.h>
#include <RendererVulkan/Resources/QueryVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Resources/ResourceViewVulkan.h>
#include <RendererVulkan/Resources/SharedTextureVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Resources/UnorderedAccessViewVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ImageCopyVulkan.h>
#include <RendererVulkan/Utils/PipelineBarrierVulkan.h>

#if PL_ENABLED(PL_SUPPORTS_GLFW)
#  include <GLFW/glfw3.h>
#endif

#if PL_ENABLED(PL_PLATFORM_LINUX)
#  include <errno.h>
#  include <unistd.h>
#endif

PL_DEFINE_AS_POD_TYPE(VkLayerProperties);

namespace
{
  VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
  {
    switch (messageSeverity)
    {
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        plLog::Debug("VK: {}", pCallbackData->pMessage);
        break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        plLog::Info("VK: {}", pCallbackData->pMessage);
        break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        plLog::Warning("VK: {}", pCallbackData->pMessage);
        break;
      case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        plLog::Error("VK: {}", pCallbackData->pMessage);
        break;
      default:
        break;
    }
    // Only layers are allowed to return true here.
    return VK_FALSE;
  }

  bool isInstanceLayerPresent(const char* layerName)
  {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    plDynamicArray<VkLayerProperties> availableLayers;
    availableLayers.SetCountUninitialized(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.GetData());

    for (const auto& layerProperties : availableLayers)
    {
      if (strcmp(layerName, layerProperties.layerName) == 0)
      {
        return true;
      }
    }

    return false;
  }
} // namespace

// Need to implement these extension functions so vulkan hpp can call them.
// They're basically just adapters calling the function pointer retrieved previously.

PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXTFunc;
PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabelEXTFunc;
PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndDebugUtilsLabelEXTFunc;
PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXTFunc;
PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXTFunc;
PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXTFunc;


VkResult vkSetDebugUtilsObjectNameEXT(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pObjectName)
{
  return vkSetDebugUtilsObjectNameEXTFunc(device, pObjectName);
}

void vkQueueBeginDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT* pLabelInfo)
{
  return vkQueueBeginDebugUtilsLabelEXTFunc(queue, pLabelInfo);
}

void vkQueueEndDebugUtilsLabelEXT(VkQueue queue)
{
  return vkQueueEndDebugUtilsLabelEXTFunc(queue);
}
//
// void vkQueueInsertDebugUtilsLabelEXT(VkQueue queue, const VkDebugUtilsLabelEXT* pLabelInfo)
//{
//  return vkQueueInsertDebugUtilsLabelEXTFunc(queue, pLabelInfo);
//}

void vkCmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo)
{
  return vkCmdBeginDebugUtilsLabelEXTFunc(commandBuffer, pLabelInfo);
}

void vkCmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer)
{
  return vkCmdEndDebugUtilsLabelEXTFunc(commandBuffer);
}

void vkCmdInsertDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo)
{
  return vkCmdInsertDebugUtilsLabelEXTFunc(commandBuffer, pLabelInfo);
}

plInternal::NewInstance<plGALDevice> CreateVulkanDevice(plAllocator* pAllocator, const plGALDeviceCreationDescription& Description)
{
  return PL_NEW(pAllocator, plGALDeviceVulkan, Description);
}

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(RendererVulkan, DeviceFactory)

ON_CORESYSTEMS_STARTUP
{
  plGALDeviceFactory::RegisterCreatorFunc("Vulkan", &CreateVulkanDevice, "VULKAN", "plShaderCompilerDXC");
}

ON_CORESYSTEMS_SHUTDOWN
{
  plGALDeviceFactory::UnregisterCreatorFunc("Vulkan");
}

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

plGALDeviceVulkan::plGALDeviceVulkan(const plGALDeviceCreationDescription& Description)
  : plGALDevice(Description)
{
}

plGALDeviceVulkan::~plGALDeviceVulkan() = default;

// Init & shutdown functions


vk::Result plGALDeviceVulkan::SelectInstanceExtensions(plHybridArray<const char*, 6>& extensions)
{
  // Fetch the list of extensions supported by the runtime.
  plUInt32 extensionCount;
  VK_SUCCEED_OR_RETURN_LOG(vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
  plDynamicArray<vk::ExtensionProperties> extensionProperties;
  extensionProperties.SetCount(extensionCount);
  VK_SUCCEED_OR_RETURN_LOG(vk::enumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.GetData()));

  PL_LOG_BLOCK("InstanceExtensions");
  for (auto& ext : extensionProperties)
  {
    plLog::Info("{}", ext.extensionName.data());
  }

  // Add a specific extension to the list of extensions to be enabled, if it is supported.
  auto AddExtIfSupported = [&](const char* extensionName, bool& enableFlag) -> vk::Result {
    auto it = std::find_if(begin(extensionProperties), end(extensionProperties), [&](const vk::ExtensionProperties& prop) { return plStringUtils::IsEqual(prop.extensionName.data(), extensionName); });
    if (it != end(extensionProperties))
    {
      extensions.PushBack(extensionName);
      enableFlag = true;
      return vk::Result::eSuccess;
    }
    enableFlag = false;
    return vk::Result::eErrorExtensionNotPresent;
  };

  VK_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(VK_KHR_SURFACE_EXTENSION_NAME, m_extensions.m_bSurface));
#if PL_ENABLED(PL_SUPPORTS_GLFW)
  uint32_t iNumGlfwExtensions = 0;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&iNumGlfwExtensions);
  bool dummy = false;
  for (uint32_t i = 0; i < iNumGlfwExtensions; ++i)
  {
    VK_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(glfwExtensions[i], dummy));
  }
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
  VK_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, m_extensions.m_bWin32Surface));
#else
#  error "Vulkan platform not supported"
#endif
  VK_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, m_extensions.m_bDebugUtils));
  m_extensions.m_bDebugUtilsMarkers = m_extensions.m_bDebugUtils;

  return vk::Result::eSuccess;
}


vk::Result plGALDeviceVulkan::SelectDeviceExtensions(vk::DeviceCreateInfo& deviceCreateInfo, plHybridArray<const char*, 6>& extensions)
{
  // Fetch the list of extensions supported by the runtime.
  plUInt32 extensionCount;
  VK_SUCCEED_OR_RETURN_LOG(m_physicalDevice.enumerateDeviceExtensionProperties(nullptr, &extensionCount, nullptr));
  plDynamicArray<vk::ExtensionProperties> extensionProperties;
  extensionProperties.SetCount(extensionCount);
  VK_SUCCEED_OR_RETURN_LOG(m_physicalDevice.enumerateDeviceExtensionProperties(nullptr, &extensionCount, extensionProperties.GetData()));

  PL_LOG_BLOCK("DeviceExtensions");
  for (auto& ext : extensionProperties)
  {
    plLog::Info("{}", ext.extensionName.data());
  }

  // Add a specific extension to the list of extensions to be enabled, if it is supported.
  auto AddExtIfSupported = [&](const char* extensionName, bool& enableFlag) -> vk::Result {
    auto it = std::find_if(begin(extensionProperties), end(extensionProperties), [&](const vk::ExtensionProperties& prop) { return plStringUtils::IsEqual(prop.extensionName.data(), extensionName); });
    if (it != end(extensionProperties))
    {
      extensions.PushBack(extensionName);
      enableFlag = true;
      return vk::Result::eSuccess;
    }
    enableFlag = false;
    plLog::Warning("Extension '{}' not supported", extensionName);
    return vk::Result::eErrorExtensionNotPresent;
  };

  VK_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(VK_KHR_SWAPCHAIN_EXTENSION_NAME, m_extensions.m_bDeviceSwapChain));
  AddExtIfSupported(VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME, m_extensions.m_bShaderViewportIndexLayer);

  vk::PhysicalDeviceFeatures2 features;
  features.pNext = &m_extensions.m_borderColorEXT;
  m_physicalDevice.getFeatures2(&features);

  m_supportedStages = vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eComputeShader;
  if (features.features.geometryShader)
  {
    m_supportedStages |= vk::PipelineStageFlagBits::eGeometryShader;
  }
  else
  {
    plLog::Warning("Geometry shaders are not supported.");
  }

  if (features.features.tessellationShader)
  {
    m_supportedStages |= vk::PipelineStageFlagBits::eTessellationControlShader | vk::PipelineStageFlagBits::eTessellationEvaluationShader;
  }
  else
  {
    plLog::Warning("Tessellation shaders are not supported.");
  }

  // Only use the extension if it allows us to not specify a format or we would need to create different samplers for every texture.
  if (m_extensions.m_borderColorEXT.customBorderColors && m_extensions.m_borderColorEXT.customBorderColorWithoutFormat)
  {
    AddExtIfSupported(VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME, m_extensions.m_bBorderColorFloat);
    if (m_extensions.m_bBorderColorFloat)
    {
      m_extensions.m_borderColorEXT.pNext = const_cast<void*>(deviceCreateInfo.pNext);
      deviceCreateInfo.pNext = &m_extensions.m_borderColorEXT;
    }
  }

  AddExtIfSupported(VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME, m_extensions.m_bImageFormatList);

  AddExtIfSupported(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME, m_extensions.m_bTimelineSemaphore);

  if (m_extensions.m_bTimelineSemaphore)
  {
    m_extensions.m_timelineSemaphoresEXT.pNext = const_cast<void*>(deviceCreateInfo.pNext);
    deviceCreateInfo.pNext = &m_extensions.m_timelineSemaphoresEXT;
    m_extensions.m_timelineSemaphoresEXT.timelineSemaphore = true;
  }

#if PL_ENABLED(PL_PLATFORM_LINUX)
  AddExtIfSupported(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME, m_extensions.m_bExternalMemoryFd);
  AddExtIfSupported(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME, m_extensions.m_bExternalSemaphoreFd);
#elif PL_ENABLED(PL_PLATFORM_WINDOWS)
  AddExtIfSupported(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME, m_extensions.m_bExternalMemoryWin32);
  AddExtIfSupported(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME, m_extensions.m_bExternalSemaphoreWin32);
#endif

  AddExtIfSupported(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, m_extensions.m_bAccelerationStructure);
  AddExtIfSupported(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, m_extensions.m_bRayTracing);
  AddExtIfSupported(VK_KHR_RAY_QUERY_EXTENSION_NAME, m_extensions.m_bRayQuery);

  return vk::Result::eSuccess;
}

#define PL_GET_INSTANCE_PROC_ADDR(name) m_extensions.pfn_##name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(m_instance, #name));

plResult plGALDeviceVulkan::InitPlatform()
{
  PL_LOG_BLOCK("plGALDeviceVulkan::InitPlatform");

  const char* layers[] = {"VK_LAYER_KHRONOS_validation"};
  {
    // Create instance
    // We use Vulkan 1.1 because of two features:
    // 1. Descriptor set pools return vk::Result::eErrorOutOfPoolMemory if exhaused. Removing the requirement to count usage yourself.
    // 2. Viewport height can be negative which performs y-inversion of the clip-space to framebuffer-space transform.
    vk::ApplicationInfo applicationInfo = {};
    applicationInfo.apiVersion = VK_API_VERSION_1_1;
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0); // TODO put plEngine version here
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);      // TODO put plEngine version here
    applicationInfo.pApplicationName = "plEngine";
    applicationInfo.pEngineName = "plEngine";

    plHybridArray<const char*, 6> instanceExtensions;
    VK_SUCCEED_OR_RETURN_PL_FAILURE(SelectInstanceExtensions(instanceExtensions));

    vk::InstanceCreateInfo instanceCreateInfo;
    // enabling support for win32 surfaces
    instanceCreateInfo.pApplicationInfo = &applicationInfo;

    instanceCreateInfo.enabledExtensionCount = instanceExtensions.GetCount();
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.GetData();

    instanceCreateInfo.enabledLayerCount = 0;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (m_Description.m_bDebugDevice)
    {
      debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
      debugCreateInfo.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
      debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
      debugCreateInfo.pfnUserCallback = debugCallback;
      debugCreateInfo.pUserData = nullptr;

      if (isInstanceLayerPresent(layers[0]))
      {
        instanceCreateInfo.enabledLayerCount = PL_ARRAY_SIZE(layers);
        instanceCreateInfo.ppEnabledLayerNames = layers;
      }
      else
      {
        plLog::Warning("The khronos validation layer is not supported on this device. Will run without validation layer.");
      }

      if (m_extensions.m_bDebugUtils)
        instanceCreateInfo.pNext = &debugCreateInfo;
    }

    m_instance = vk::createInstance(instanceCreateInfo);

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
    if (m_extensions.m_bDebugUtils)
    {
      PL_GET_INSTANCE_PROC_ADDR(vkCreateDebugUtilsMessengerEXT);
      PL_GET_INSTANCE_PROC_ADDR(vkDestroyDebugUtilsMessengerEXT);
      PL_GET_INSTANCE_PROC_ADDR(vkSetDebugUtilsObjectNameEXT);
      VK_SUCCEED_OR_RETURN_PL_FAILURE(m_extensions.pfn_vkCreateDebugUtilsMessengerEXT(m_instance, &debugCreateInfo, nullptr, &m_debugMessenger));
    }
#endif

    if (!m_instance)
    {
      plLog::Error("Failed to create Vulkan instance!");
      return PL_FAILURE;
    }
  }

  {
    // physical device
    plUInt32 physicalDeviceCount = 0;
    plHybridArray<vk::PhysicalDevice, 2> physicalDevices;
    VK_SUCCEED_OR_RETURN_PL_FAILURE(m_instance.enumeratePhysicalDevices(&physicalDeviceCount, nullptr));
    if (physicalDeviceCount == 0)
    {
      plLog::Error("No available physical device to create a Vulkan device on!");
      return PL_FAILURE;
    }

    physicalDevices.SetCount(physicalDeviceCount);
    VK_SUCCEED_OR_RETURN_PL_FAILURE(m_instance.enumeratePhysicalDevices(&physicalDeviceCount, physicalDevices.GetData()));

    // TODO choosable physical device?
    // TODO making sure we have a hardware device?
    m_physicalDevice = physicalDevices[0];
    m_properties = m_physicalDevice.getProperties();
    plLog::Info("Selected physical device \"{}\" for device creation.", m_properties.deviceName);

    // This is a workaround for broken lavapipe drivers which cannot handle label scopes that span across multiple command buffers.
    plStringBuilder sDeviceName = plStringUtf8(m_properties.deviceName).GetView();
    if (sDeviceName.Compare_NoCase("LLVMPIPE"))
    {
      m_extensions.m_bDebugUtilsMarkers = false;
    }
    // TODO call vkGetPhysicalDeviceFeatures2 with VkPhysicalDeviceTimelineSemaphoreFeatures and figure out if time
  }

  plHybridArray<vk::QueueFamilyProperties, 4> queueFamilyProperties;
  {
    // Device
    plUInt32 queueFamilyPropertyCount = 0;
    m_physicalDevice.getQueueFamilyProperties(&queueFamilyPropertyCount, nullptr);
    if (queueFamilyPropertyCount == 0)
    {
      plLog::Error("No available device queues on physical device!");
      return PL_FAILURE;
    }
    queueFamilyProperties.SetCount(queueFamilyPropertyCount);
    m_physicalDevice.getQueueFamilyProperties(&queueFamilyPropertyCount, queueFamilyProperties.GetData());

    {
      PL_LOG_BLOCK("Queue Families");
      for (plUInt32 i = 0; i < queueFamilyProperties.GetCount(); ++i)
      {
        const vk::QueueFamilyProperties& queueFamilyProperty = queueFamilyProperties[i];
        plLog::Info("Queue count: {}, flags: {}", queueFamilyProperty.queueCount, vk::to_string(queueFamilyProperty.queueFlags).data());
      }
    }

    // Select best queue family for graphics and transfers.
    for (plUInt32 i = 0; i < queueFamilyProperties.GetCount(); ++i)
    {
      const vk::QueueFamilyProperties& queueFamilyProperty = queueFamilyProperties[i];
      if (queueFamilyProperty.queueCount == 0)
        continue;
      constexpr auto graphicsFlags = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute;
      if ((queueFamilyProperty.queueFlags & graphicsFlags) == graphicsFlags)
      {
        m_graphicsQueue.m_uiQueueFamily = i;
      }
      if (queueFamilyProperty.queueFlags & vk::QueueFlagBits::eTransfer)
      {
        if (m_transferQueue.m_uiQueueFamily == -1)
        {
          m_transferQueue.m_uiQueueFamily = i;
        }
        else if ((queueFamilyProperty.queueFlags & graphicsFlags) == vk::QueueFlagBits())
        {
          // Prefer a queue that can't be used for graphics.
          m_transferQueue.m_uiQueueFamily = i;
        }
      }
    }
    if (m_graphicsQueue.m_uiQueueFamily == -1)
    {
      plLog::Error("No graphics queue found.");
      return PL_FAILURE;
    }
    if (m_transferQueue.m_uiQueueFamily == -1)
    {
      plLog::Error("No transfer queue found.");
      return PL_FAILURE;
    }

    constexpr float queuePriority = 0.f;

    plHybridArray<vk::DeviceQueueCreateInfo, 2> queues;

    vk::DeviceQueueCreateInfo& graphicsQueueCreateInfo = queues.ExpandAndGetRef();
    graphicsQueueCreateInfo.pQueuePriorities = &queuePriority;
    graphicsQueueCreateInfo.queueCount = 1;
    graphicsQueueCreateInfo.queueFamilyIndex = m_graphicsQueue.m_uiQueueFamily;
    if (m_graphicsQueue.m_uiQueueFamily != m_transferQueue.m_uiQueueFamily)
    {
      vk::DeviceQueueCreateInfo& transferQueueCreateInfo = queues.ExpandAndGetRef();
      transferQueueCreateInfo.pQueuePriorities = &queuePriority;
      transferQueueCreateInfo.queueCount = 1;
      transferQueueCreateInfo.queueFamilyIndex = m_transferQueue.m_uiQueueFamily;
    }

    // #TODO_VULKAN test that this returns the same as 'layers' passed into the instance.
    plUInt32 uiLayers;
    VK_SUCCEED_OR_RETURN_PL_FAILURE(m_physicalDevice.enumerateDeviceLayerProperties(&uiLayers, nullptr));
    plDynamicArray<vk::LayerProperties> deviceLayers;
    deviceLayers.SetCount(uiLayers);
    VK_SUCCEED_OR_RETURN_PL_FAILURE(m_physicalDevice.enumerateDeviceLayerProperties(&uiLayers, deviceLayers.GetData()));

    vk::DeviceCreateInfo deviceCreateInfo = {};
    plHybridArray<const char*, 6> deviceExtensions;
    VK_SUCCEED_OR_RETURN_PL_FAILURE(SelectDeviceExtensions(deviceCreateInfo, deviceExtensions));

    deviceCreateInfo.enabledExtensionCount = deviceExtensions.GetCount();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.GetData();
    // Device layers are deprecated but provided (same as in instance) for backwards compatibility.
    deviceCreateInfo.enabledLayerCount = PL_ARRAY_SIZE(layers);
    deviceCreateInfo.ppEnabledLayerNames = layers;

    vk::PhysicalDeviceFeatures physicalDeviceFeatures = m_physicalDevice.getFeatures();
    deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures; // Enabling all available features for now
    deviceCreateInfo.queueCreateInfoCount = queues.GetCount();
    deviceCreateInfo.pQueueCreateInfos = queues.GetData();

    VK_SUCCEED_OR_RETURN_PL_FAILURE(m_physicalDevice.createDevice(&deviceCreateInfo, nullptr, &m_device));
    m_device.getQueue(m_graphicsQueue.m_uiQueueFamily, m_graphicsQueue.m_uiQueueIndex, &m_graphicsQueue.m_queue);
    m_device.getQueue(m_transferQueue.m_uiQueueFamily, m_transferQueue.m_uiQueueIndex, &m_transferQueue.m_queue);

    m_dispatchContext.Init(*this);
  }

  VK_SUCCEED_OR_RETURN_PL_FAILURE(plMemoryAllocatorVulkan::Initialize(m_physicalDevice, m_device, m_instance));

  vkSetDebugUtilsObjectNameEXTFunc = (PFN_vkSetDebugUtilsObjectNameEXT)m_device.getProcAddr("vkSetDebugUtilsObjectNameEXT");
  vkQueueBeginDebugUtilsLabelEXTFunc = (PFN_vkQueueBeginDebugUtilsLabelEXT)m_device.getProcAddr("vkQueueBeginDebugUtilsLabelEXT");
  vkQueueEndDebugUtilsLabelEXTFunc = (PFN_vkQueueEndDebugUtilsLabelEXT)m_device.getProcAddr("vkQueueEndDebugUtilsLabelEXT");
  vkCmdBeginDebugUtilsLabelEXTFunc = (PFN_vkCmdBeginDebugUtilsLabelEXT)m_device.getProcAddr("vkCmdBeginDebugUtilsLabelEXT");
  vkCmdEndDebugUtilsLabelEXTFunc = (PFN_vkCmdEndDebugUtilsLabelEXT)m_device.getProcAddr("vkCmdEndDebugUtilsLabelEXT");
  vkCmdInsertDebugUtilsLabelEXTFunc = (PFN_vkCmdInsertDebugUtilsLabelEXT)m_device.getProcAddr("vkCmdInsertDebugUtilsLabelEXT");


  m_memoryProperties = m_physicalDevice.getMemoryProperties();

  // Fill lookup table
  FillFormatLookupTable();

  plClipSpaceDepthRange::Default = plClipSpaceDepthRange::ZeroToOne;
  // We use plClipSpaceYMode::Regular and rely in the Vulkan 1.1 feature that a negative height performs y-inversion of the clip-space to framebuffer-space transform.
  // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VK_KHR_maintenance1.html
  plClipSpaceYMode::RenderToTextureDefault = plClipSpaceYMode::Regular;

  m_pPipelineBarrier = PL_NEW(&m_Allocator, plPipelineBarrierVulkan);
  m_pCommandBufferPool = PL_NEW(&m_Allocator, plCommandBufferPoolVulkan);
  m_pCommandBufferPool->Initialize(m_device, m_graphicsQueue.m_uiQueueFamily);
  m_pStagingBufferPool = PL_NEW(&m_Allocator, plStagingBufferPoolVulkan);
  m_pStagingBufferPool->Initialize(this);
  m_pQueryPool = PL_NEW(&m_Allocator, plQueryPoolVulkan);
  m_pQueryPool->Initialize(this, queueFamilyProperties[m_graphicsQueue.m_uiQueueFamily].timestampValidBits);
  m_pInitContext = PL_NEW(&m_Allocator, plInitContextVulkan, this);

  plSemaphorePoolVulkan::Initialize(m_device);
  plFencePoolVulkan::Initialize(m_device);
  plResourceCacheVulkan::Initialize(this, m_device);
  plDescriptorSetPoolVulkan::Initialize(m_device);
  plImageCopyVulkan::Initialize(*this);

  m_pDefaultPass = PL_NEW(&m_Allocator, plGALPassVulkan, *this);

  plGALWindowSwapChain::SetFactoryMethod([this](const plGALWindowSwapChainCreationDescription& desc) -> plGALSwapChainHandle { return CreateSwapChain([this, &desc](plAllocator* pAllocator) -> plGALSwapChain* { return PL_NEW(pAllocator, plGALSwapChainVulkan, desc); }); });

  return PL_SUCCESS;
}

void plGALDeviceVulkan::SetDebugName(const vk::DebugUtilsObjectNameInfoEXT& info, plVulkanAllocation allocation)
{
#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  if (m_extensions.m_bDebugUtils)
  {
    m_device.setDebugUtilsObjectNameEXT(info);
  }
  if (allocation)
    plMemoryAllocatorVulkan::SetAllocationUserData(allocation, info.pObjectName);
#endif
}

void plGALDeviceVulkan::ReportLiveGpuObjects()
{
  // This is automatically done in the validation layer and can't be easily done manually.
}

void plGALDeviceVulkan::UploadBufferStaging(plStagingBufferPoolVulkan* pStagingBufferPool, plPipelineBarrierVulkan* pPipelineBarrier, vk::CommandBuffer commandBuffer, const plGALBufferVulkan* pBuffer, plArrayPtr<const plUInt8> pInitialData, vk::DeviceSize dstOffset)
{
  void* pData = nullptr;

  // #TODO_VULKAN Use transfer queue
  plStagingBufferVulkan stagingBuffer = pStagingBufferPool->AllocateBuffer(0, pInitialData.GetCount());
  // plMemoryUtils::Copy(reinterpret_cast<plUInt8*>(stagingBuffer.m_allocInfo.m_pMappedData), pInitialData.GetPtr(), pInitialData.GetCount());
  plMemoryAllocatorVulkan::MapMemory(stagingBuffer.m_alloc, &pData);
  plMemoryUtils::Copy(reinterpret_cast<plUInt8*>(pData), pInitialData.GetPtr(), pInitialData.GetCount());
  plMemoryAllocatorVulkan::UnmapMemory(stagingBuffer.m_alloc);

  vk::BufferCopy region;
  region.srcOffset = 0;
  region.dstOffset = dstOffset;
  region.size = pInitialData.GetCount();

  // #TODO_VULKAN atomic min size violation?
  commandBuffer.copyBuffer(stagingBuffer.m_buffer, pBuffer->GetVkBuffer(), 1, &region);

  pPipelineBarrier->AccessBuffer(pBuffer, region.dstOffset, region.size, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite, pBuffer->GetUsedByPipelineStage(), pBuffer->GetAccessMask());

  // #TODO_VULKAN Custom delete later / return to plStagingBufferPoolVulkan once this is on the transfer queue and runs async to graphics queue.
  pStagingBufferPool->ReclaimBuffer(stagingBuffer);
}

void plGALDeviceVulkan::UploadTextureStaging(plStagingBufferPoolVulkan* pStagingBufferPool, plPipelineBarrierVulkan* pPipelineBarrier, vk::CommandBuffer commandBuffer, const plGALTextureVulkan* pTexture, const vk::ImageSubresourceLayers& subResource, const plGALSystemMemoryDescription& data)
{
  const vk::Offset3D imageOffset = {0, 0, 0};
  const vk::Extent3D imageExtent = pTexture->GetMipLevelSize(subResource.mipLevel);

  auto getRange = [](const vk::ImageSubresourceLayers& layers) -> vk::ImageSubresourceRange {
    vk::ImageSubresourceRange range;
    range.aspectMask = layers.aspectMask;
    range.baseMipLevel = layers.mipLevel;
    range.levelCount = 1;
    range.baseArrayLayer = layers.baseArrayLayer;
    range.layerCount = layers.layerCount;
    return range;
  };

  pPipelineBarrier->EnsureImageLayout(pTexture, getRange(subResource), vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits::eTransfer, vk::AccessFlagBits::eTransferWrite);
  pPipelineBarrier->Flush();

  for (plUInt32 i = 0; i < subResource.layerCount; i++)
  {
    auto pLayerData = reinterpret_cast<const plUInt8*>(data.m_pData) + i * data.m_uiSlicePitch;
    const vk::Format format = pTexture->GetImageFormat();
    const plUInt8 uiBlockSize = vk::blockSize(format);
    const auto blockExtent = vk::blockExtent(format);
    const VkExtent3D blockCount = {
      (imageExtent.width + blockExtent[0] - 1) / blockExtent[0],
      (imageExtent.height + blockExtent[1] - 1) / blockExtent[1],
      (imageExtent.depth + blockExtent[2] - 1) / blockExtent[2]};

    const vk::DeviceSize uiTotalSize = uiBlockSize * blockCount.width * blockCount.height * blockCount.depth;
    plStagingBufferVulkan stagingBuffer = pStagingBufferPool->AllocateBuffer(0, uiTotalSize);

    const plUInt32 uiBufferRowPitch = uiBlockSize * blockCount.width;
    const plUInt32 uiBufferSlicePitch = uiBufferRowPitch * blockCount.height;
    //PL_ASSERT_DEV(uiBufferRowPitch == data.m_uiRowPitch, "Row pitch with padding is not implemented yet.");
    //PL_ASSERT_DEV(uiBufferSlicePitch == data.m_uiSlicePitch, "Row pitch with padding is not implemented yet.");

    void* pData = nullptr;
    plMemoryAllocatorVulkan::MapMemory(stagingBuffer.m_alloc, &pData);
    plMemoryUtils::Copy(reinterpret_cast<plUInt8*>(pData), pLayerData, uiTotalSize);
    plMemoryAllocatorVulkan::UnmapMemory(stagingBuffer.m_alloc);

    vk::BufferImageCopy region = {};
    region.imageSubresource = subResource;
    region.imageOffset = imageOffset;
    region.imageExtent = imageExtent;

    region.bufferOffset = 0;
    region.bufferRowLength = blockExtent[0] * uiBufferRowPitch / uiBlockSize;
    region.bufferImageHeight = blockExtent[1] * uiBufferSlicePitch / uiBufferRowPitch;

    // #TODO_VULKAN atomic min size violation?
    commandBuffer.copyBufferToImage(stagingBuffer.m_buffer, pTexture->GetImage(), pTexture->GetPreferredLayout(vk::ImageLayout::eTransferDstOptimal), 1, &region);
    pStagingBufferPool->ReclaimBuffer(stagingBuffer);
  }

  pPipelineBarrier->EnsureImageLayout(pTexture, getRange(subResource), pTexture->GetPreferredLayout(), pTexture->GetUsedByPipelineStage(), pTexture->GetAccessMask());
}

plResult plGALDeviceVulkan::ShutdownPlatform()
{
  plImageCopyVulkan::DeInitialize(*this);
  DestroyDeadObjects(); // plImageCopyVulkan might add dead objects, so make sure the list is cleared again

  plGALWindowSwapChain::SetFactoryMethod({});
  if (m_lastCommandBufferFinished)
    ReclaimLater(m_lastCommandBufferFinished, m_pCommandBufferPool.Borrow());
  auto& pCommandEncoder = m_pDefaultPass->m_pCommandEncoderImpl;

  // We couldn't create a device in the first place, so early out of shutdown
  if (!m_device)
  {
    return PL_SUCCESS;
  }

  WaitIdlePlatform();

  m_pDefaultPass = nullptr;
  m_pPipelineBarrier = nullptr;
  m_pCommandBufferPool->DeInitialize();
  m_pCommandBufferPool = nullptr;
  m_pStagingBufferPool->DeInitialize();
  m_pStagingBufferPool = nullptr;
  m_pQueryPool->DeInitialize();
  m_pQueryPool = nullptr;
  m_pInitContext = nullptr;

  plSemaphorePoolVulkan::DeInitialize();
  plFencePoolVulkan::DeInitialize();
  plResourceCacheVulkan::DeInitialize();
  plDescriptorSetPoolVulkan::DeInitialize();

  plMemoryAllocatorVulkan::DeInitialize();

  m_device.waitIdle();
  m_device.destroy();

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  if (m_extensions.m_bDebugUtils && m_extensions.pfn_vkDestroyDebugUtilsMessengerEXT != nullptr)
  {
    m_extensions.pfn_vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
  }
#endif

  m_instance.destroy();
  ReportLiveGpuObjects();

  return PL_SUCCESS;
}

// Pipeline & Pass functions

vk::CommandBuffer& plGALDeviceVulkan::GetCurrentCommandBuffer()
{
  vk::CommandBuffer& commandBuffer = m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer;
  if (!commandBuffer)
  {
    // Restart new command buffer if none is active already.
    commandBuffer = m_pCommandBufferPool->RequestCommandBuffer();
    vk::CommandBufferBeginInfo beginInfo;
    VK_ASSERT_DEBUG(commandBuffer.begin(&beginInfo));
    GetCurrentPipelineBarrier().SetCommandBuffer(&commandBuffer);

    m_pDefaultPass->SetCurrentCommandBuffer(&commandBuffer, m_pPipelineBarrier.Borrow());
    // We can't carry state across individual command buffers.
    m_pDefaultPass->MarkDirty();
  }
  return commandBuffer;
}

plPipelineBarrierVulkan& plGALDeviceVulkan::GetCurrentPipelineBarrier()
{
  vk::CommandBuffer& commandBuffer = m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer;
  if (!commandBuffer)
  {
    GetCurrentCommandBuffer();
  }
  return *m_pPipelineBarrier.Borrow();
}

plQueryPoolVulkan& plGALDeviceVulkan::GetQueryPool() const
{
  return *m_pQueryPool.Borrow();
}

plStagingBufferPoolVulkan& plGALDeviceVulkan::GetStagingBufferPool() const
{
  return *m_pStagingBufferPool.Borrow();
}

plInitContextVulkan& plGALDeviceVulkan::GetInitContext() const
{
  return *m_pInitContext.Borrow();
}

plProxyAllocator& plGALDeviceVulkan::GetAllocator()
{
  return m_Allocator;
}

plGALTextureHandle plGALDeviceVulkan::CreateTextureInternal(const plGALTextureCreationDescription& Description, plArrayPtr<plGALSystemMemoryDescription> pInitialData, bool bLinearCPU, bool bStaging)
{
  plGALTextureVulkan* pTexture = PL_NEW(&m_Allocator, plGALTextureVulkan, Description, bLinearCPU, bStaging);

  if (!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    PL_DELETE(&m_Allocator, pTexture);
    return plGALTextureHandle();
  }

  return FinalizeTextureInternal(Description, pTexture);
}

plGALBufferHandle plGALDeviceVulkan::CreateBufferInternal(const plGALBufferCreationDescription& Description, plArrayPtr<const plUInt8> pInitialData, bool bCPU)
{
  plGALBufferVulkan* pBuffer = PL_NEW(&m_Allocator, plGALBufferVulkan, Description, bCPU);

  if (!pBuffer->InitPlatform(this, pInitialData).Succeeded())
  {
    PL_DELETE(&m_Allocator, pBuffer);
    return plGALBufferHandle();
  }

  return FinalizeBufferInternal(Description, pBuffer);
}

bool plGALDeviceVulkan::GetRayTracingSupported()
{
  return m_extensions.m_bAccelerationStructure && m_extensions.m_bRayTracing && m_extensions.m_bRayQuery;
}

void plGALDeviceVulkan::BeginPipelinePlatform(const char* szName, plGALSwapChain* pSwapChain)
{
  PL_PROFILE_SCOPE("BeginPipelinePlatform");

  GetCurrentCommandBuffer();
#if PL_ENABLED(PL_USE_PROFILING)
  m_pPipelineTimingScope = plProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), szName);
#endif

  if (pSwapChain)
  {
    pSwapChain->AcquireNextRenderTarget(this);
  }
}

void plGALDeviceVulkan::EndPipelinePlatform(plGALSwapChain* pSwapChain)
{
  PL_PROFILE_SCOPE("EndPipelinePlatform");

#if PL_ENABLED(PL_USE_PROFILING)
  plProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pPipelineTimingScope);
#endif
  if (pSwapChain)
  {
    pSwapChain->PresentRenderTarget(this);
  }

  // Render context is reset on every end pipeline so it will re-submit all state change for the next render pass. Thus it is safe at this point to do a full reset.
  // Technically don't have to reset here, MarkDirty would also be fine but we do need to do a Reset at the end of the frame as pointers held by the plGALCommandEncoderImplVulkan may not be valid in the next frame.
  m_pDefaultPass->Reset();
}

vk::Fence plGALDeviceVulkan::Submit()
{
  m_pDefaultPass->SetCurrentCommandBuffer(nullptr, nullptr);

  vk::CommandBuffer initCommandBuffer = m_pInitContext->GetFinishedCommandBuffer();
  bool bHasCmdBuffer = initCommandBuffer || m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer;

  plHybridArray<vk::CommandBuffer, 2> buffers;
  vk::SubmitInfo submitInfo = {};
  if (bHasCmdBuffer)
  {
    vk::CommandBuffer mainCommandBuffer = m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer;
    if (initCommandBuffer)
    {
      // Any background loading that happened up to this point needs to be submitted first.
      // The main render command buffer assumes that all new resources are in their default state which is made sure by submitting this command buffer.
      buffers.PushBack(initCommandBuffer);
    }
    if (mainCommandBuffer)
    {
      GetCurrentPipelineBarrier().Submit();
      mainCommandBuffer.end();
      buffers.PushBack(mainCommandBuffer);
    }
    submitInfo.commandBufferCount = buffers.GetCount();
    submitInfo.pCommandBuffers = buffers.GetData();
  }

  if (m_lastCommandBufferFinished)
  {
    AddWaitSemaphore(plGALDeviceVulkan::SemaphoreInfo::MakeWaitSemaphore(m_lastCommandBufferFinished, vk::PipelineStageFlagBits::eAllCommands));
    ReclaimLater(m_lastCommandBufferFinished);
  }

  m_lastCommandBufferFinished = plSemaphorePoolVulkan::RequestSemaphore();
  AddSignalSemaphore(plGALDeviceVulkan::SemaphoreInfo::MakeSignalSemaphore(m_lastCommandBufferFinished));

  vk::Fence renderFence = plFencePoolVulkan::RequestFence();

  plHybridArray<vk::Semaphore, 3> waitSemaphores;
  plHybridArray<vk::PipelineStageFlags, 3> waitStages;
  plHybridArray<vk::Semaphore, 3> signalSemaphores;

  plHybridArray<plUInt64, 3> waitSemaphoreValues;
  plHybridArray<plUInt64, 3> signalSemaphoreValues;
  for (const SemaphoreInfo sem : m_waitSemaphores)
  {
    waitSemaphores.PushBack(sem.m_semaphore);
    if (sem.m_type == vk::SemaphoreType::eTimeline)
    {
      waitSemaphoreValues.PushBack(sem.m_uiValue);
    }
    waitStages.PushBack(vk::PipelineStageFlagBits::eAllCommands);
  }
  m_waitSemaphores.Clear();

  for (const SemaphoreInfo sem : m_signalSemaphores)
  {
    signalSemaphores.PushBack(sem.m_semaphore);
    if (sem.m_type == vk::SemaphoreType::eTimeline)
    {
      signalSemaphoreValues.PushBack(sem.m_uiValue);
    }
  }
  m_signalSemaphores.Clear();


  // If a timeline semaphore is present, all semaphores need a value, even binary ones because validation says so.
  if (waitSemaphoreValues.GetCount() > 0)
  {
    waitSemaphoreValues.SetCount(waitSemaphores.GetCount());
  }
  if (signalSemaphoreValues.GetCount() > 0)
  {
    signalSemaphoreValues.SetCount(signalSemaphores.GetCount());
  }

  vk::TimelineSemaphoreSubmitInfo timelineInfo;
  timelineInfo.waitSemaphoreValueCount = waitSemaphoreValues.GetCount();
  PL_CHECK_AT_COMPILETIME(sizeof(plUInt64) == sizeof(uint64_t));
  timelineInfo.pWaitSemaphoreValues = reinterpret_cast<const uint64_t*>(waitSemaphoreValues.GetData());
  timelineInfo.signalSemaphoreValueCount = signalSemaphoreValues.GetCount();
  timelineInfo.pSignalSemaphoreValues = reinterpret_cast<const uint64_t*>(signalSemaphoreValues.GetData());

  if (timelineInfo.waitSemaphoreValueCount > 0 || timelineInfo.signalSemaphoreValueCount > 0)
  {
    // Only add timeline info if we have a timeline semaphore or validation layer complains.
    submitInfo.pNext = &timelineInfo;

    PL_ASSERT_DEBUG(timelineInfo.waitSemaphoreValueCount == 0 || waitSemaphores.GetCount() == waitSemaphoreValues.GetCount(), "If a timeline semaphore is present, all semaphores need a wait value.");
    PL_ASSERT_DEBUG(timelineInfo.signalSemaphoreValueCount == 0 || signalSemaphores.GetCount() == signalSemaphoreValues.GetCount(), "If a timeline semaphore is present, all semaphores need a signal value.");
  }
  PL_ASSERT_DEBUG(waitSemaphores.GetCount() == waitStages.GetCount(), "Each wait semaphore needs a wait stage");

  submitInfo.waitSemaphoreCount = waitSemaphores.GetCount();
  submitInfo.pWaitSemaphores = waitSemaphores.GetData();
  submitInfo.pWaitDstStageMask = waitStages.GetData();
  submitInfo.signalSemaphoreCount = signalSemaphores.GetCount();
  submitInfo.pSignalSemaphores = signalSemaphores.GetData();

  {
    m_PerFrameData[m_uiCurrentPerFrameData].m_CommandBufferFences.PushBack(renderFence);
    m_graphicsQueue.m_queue.submit(1, &submitInfo, renderFence);
  }

  auto res = renderFence;
  ReclaimLater(renderFence);
  if (m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer)
  {
    ReclaimLater(m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer, m_pCommandBufferPool.Borrow());
  }
  return res;
}

plGALPass* plGALDeviceVulkan::BeginPassPlatform(const char* szName)
{
  GetCurrentCommandBuffer();
#if PL_ENABLED(PL_USE_PROFILING)
  m_pPassTimingScope = plProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), szName);
#endif
  return m_pDefaultPass.Borrow();
}

void plGALDeviceVulkan::EndPassPlatform(plGALPass* pPass)
{
#if PL_ENABLED(PL_USE_PROFILING)
  plProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pPassTimingScope);
#endif
}


// State creation functions

plGALBlendState* plGALDeviceVulkan::CreateBlendStatePlatform(const plGALBlendStateCreationDescription& Description)
{
  plGALBlendStateVulkan* pState = PL_NEW(&m_Allocator, plGALBlendStateVulkan, Description);

  if (pState->InitPlatform(this).Succeeded())
  {
    return pState;
  }
  else
  {
    PL_DELETE(&m_Allocator, pState);
    return nullptr;
  }
}

void plGALDeviceVulkan::DestroyBlendStatePlatform(plGALBlendState* pBlendState)
{
  plGALBlendStateVulkan* pState = static_cast<plGALBlendStateVulkan*>(pBlendState);
  plResourceCacheVulkan::ResourceDeleted(pState);
  pState->DeInitPlatform(this).IgnoreResult();
  PL_DELETE(&m_Allocator, pState);
}

plGALDepthStencilState* plGALDeviceVulkan::CreateDepthStencilStatePlatform(const plGALDepthStencilStateCreationDescription& Description)
{
  plGALDepthStencilStateVulkan* pVulkanDepthStencilState = PL_NEW(&m_Allocator, plGALDepthStencilStateVulkan, Description);

  if (pVulkanDepthStencilState->InitPlatform(this).Succeeded())
  {
    return pVulkanDepthStencilState;
  }
  else
  {
    PL_DELETE(&m_Allocator, pVulkanDepthStencilState);
    return nullptr;
  }
}

void plGALDeviceVulkan::DestroyDepthStencilStatePlatform(plGALDepthStencilState* pDepthStencilState)
{
  plGALDepthStencilStateVulkan* pVulkanDepthStencilState = static_cast<plGALDepthStencilStateVulkan*>(pDepthStencilState);
  plResourceCacheVulkan::ResourceDeleted(pVulkanDepthStencilState);
  pVulkanDepthStencilState->DeInitPlatform(this).IgnoreResult();
  PL_DELETE(&m_Allocator, pVulkanDepthStencilState);
}

plGALRasterizerState* plGALDeviceVulkan::CreateRasterizerStatePlatform(const plGALRasterizerStateCreationDescription& Description)
{
  plGALRasterizerStateVulkan* pVulkanRasterizerState = PL_NEW(&m_Allocator, plGALRasterizerStateVulkan, Description);

  if (pVulkanRasterizerState->InitPlatform(this).Succeeded())
  {
    return pVulkanRasterizerState;
  }
  else
  {
    PL_DELETE(&m_Allocator, pVulkanRasterizerState);
    return nullptr;
  }
}

void plGALDeviceVulkan::DestroyRasterizerStatePlatform(plGALRasterizerState* pRasterizerState)
{
  plGALRasterizerStateVulkan* pVulkanRasterizerState = static_cast<plGALRasterizerStateVulkan*>(pRasterizerState);
  plResourceCacheVulkan::ResourceDeleted(pVulkanRasterizerState);
  pVulkanRasterizerState->DeInitPlatform(this).IgnoreResult();
  PL_DELETE(&m_Allocator, pVulkanRasterizerState);
}

plGALSamplerState* plGALDeviceVulkan::CreateSamplerStatePlatform(const plGALSamplerStateCreationDescription& Description)
{
  plGALSamplerStateVulkan* pVulkanSamplerState = PL_NEW(&m_Allocator, plGALSamplerStateVulkan, Description);

  if (pVulkanSamplerState->InitPlatform(this).Succeeded())
  {
    return pVulkanSamplerState;
  }
  else
  {
    PL_DELETE(&m_Allocator, pVulkanSamplerState);
    return nullptr;
  }
}

void plGALDeviceVulkan::DestroySamplerStatePlatform(plGALSamplerState* pSamplerState)
{
  plGALSamplerStateVulkan* pVulkanSamplerState = static_cast<plGALSamplerStateVulkan*>(pSamplerState);
  pVulkanSamplerState->DeInitPlatform(this).IgnoreResult();
  PL_DELETE(&m_Allocator, pVulkanSamplerState);
}


// Resource creation functions

plGALShader* plGALDeviceVulkan::CreateShaderPlatform(const plGALShaderCreationDescription& Description)
{
  plGALShaderVulkan* pShader = PL_NEW(&m_Allocator, plGALShaderVulkan, Description);

  if (!pShader->InitPlatform(this).Succeeded())
  {
    PL_DELETE(&m_Allocator, pShader);
    return nullptr;
  }

  return pShader;
}

void plGALDeviceVulkan::DestroyShaderPlatform(plGALShader* pShader)
{
  plGALShaderVulkan* pVulkanShader = static_cast<plGALShaderVulkan*>(pShader);
  plResourceCacheVulkan::ShaderDeleted(pVulkanShader);
  pVulkanShader->DeInitPlatform(this).IgnoreResult();
  PL_DELETE(&m_Allocator, pVulkanShader);
}

plGALBuffer* plGALDeviceVulkan::CreateBufferPlatform(
  const plGALBufferCreationDescription& Description, plArrayPtr<const plUInt8> pInitialData)
{
  plGALBufferVulkan* pBuffer = PL_NEW(&m_Allocator, plGALBufferVulkan, Description);

  if (!pBuffer->InitPlatform(this, pInitialData).Succeeded())
  {
    PL_DELETE(&m_Allocator, pBuffer);
    return nullptr;
  }

  return pBuffer;
}

void plGALDeviceVulkan::DestroyBufferPlatform(plGALBuffer* pBuffer)
{
  plGALBufferVulkan* pVulkanBuffer = static_cast<plGALBufferVulkan*>(pBuffer);
  GetCurrentPipelineBarrier().BufferDestroyed(pVulkanBuffer);
  pVulkanBuffer->DeInitPlatform(this).IgnoreResult();
  PL_DELETE(&m_Allocator, pVulkanBuffer);
}

plGALTexture* plGALDeviceVulkan::CreateTexturePlatform(const plGALTextureCreationDescription& Description, plArrayPtr<plGALSystemMemoryDescription> pInitialData)
{
  plGALTextureVulkan* pTexture = PL_NEW(&m_Allocator, plGALTextureVulkan, Description, false, false);

  if (!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    PL_DELETE(&m_Allocator, pTexture);
    return nullptr;
  }

  return pTexture;
}

void plGALDeviceVulkan::DestroyTexturePlatform(plGALTexture* pTexture)
{
  plGALTextureVulkan* pVulkanTexture = static_cast<plGALTextureVulkan*>(pTexture);
  GetCurrentPipelineBarrier().TextureDestroyed(pVulkanTexture);
  m_pInitContext->TextureDestroyed(pVulkanTexture);

  pVulkanTexture->DeInitPlatform(this).IgnoreResult();
  PL_DELETE(&m_Allocator, pVulkanTexture);
}

plGALTexture* plGALDeviceVulkan::CreateSharedTexturePlatform(const plGALTextureCreationDescription& Description, plArrayPtr<plGALSystemMemoryDescription> pInitialData, plEnum<plGALSharedTextureType> sharedType, plGALPlatformSharedHandle handle)
{
  plGALSharedTextureVulkan* pTexture = PL_NEW(&m_Allocator, plGALSharedTextureVulkan, Description, sharedType, handle);

  if (!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    PL_DELETE(&m_Allocator, pTexture);
    return nullptr;
  }

  return pTexture;
}

void plGALDeviceVulkan::DestroySharedTexturePlatform(plGALTexture* pTexture)
{
  plGALSharedTextureVulkan* pVulkanTexture = static_cast<plGALSharedTextureVulkan*>(pTexture);
  GetCurrentPipelineBarrier().TextureDestroyed(pVulkanTexture);
  m_pInitContext->TextureDestroyed(pVulkanTexture);

  pVulkanTexture->DeInitPlatform(this).IgnoreResult();
  PL_DELETE(&m_Allocator, pVulkanTexture);
}

plGALResourceView* plGALDeviceVulkan::CreateResourceViewPlatform(
  plGALResourceBase* pResource, const plGALResourceViewCreationDescription& Description)
{
  plGALResourceViewVulkan* pResourceView = PL_NEW(&m_Allocator, plGALResourceViewVulkan, pResource, Description);

  if (!pResourceView->InitPlatform(this).Succeeded())
  {
    PL_DELETE(&m_Allocator, pResourceView);
    return nullptr;
  }

  return pResourceView;
}

void plGALDeviceVulkan::DestroyResourceViewPlatform(plGALResourceView* pResourceView)
{
  plGALResourceViewVulkan* pVulkanResourceView = static_cast<plGALResourceViewVulkan*>(pResourceView);
  pVulkanResourceView->DeInitPlatform(this).IgnoreResult();
  PL_DELETE(&m_Allocator, pVulkanResourceView);
}

plGALRenderTargetView* plGALDeviceVulkan::CreateRenderTargetViewPlatform(
  plGALTexture* pTexture, const plGALRenderTargetViewCreationDescription& Description)
{
  plGALRenderTargetViewVulkan* pRTView = PL_NEW(&m_Allocator, plGALRenderTargetViewVulkan, pTexture, Description);

  if (!pRTView->InitPlatform(this).Succeeded())
  {
    PL_DELETE(&m_Allocator, pRTView);
    return nullptr;
  }

  return pRTView;
}

void plGALDeviceVulkan::DestroyRenderTargetViewPlatform(plGALRenderTargetView* pRenderTargetView)
{
  plGALRenderTargetViewVulkan* pVulkanRenderTargetView = static_cast<plGALRenderTargetViewVulkan*>(pRenderTargetView);
  pVulkanRenderTargetView->DeInitPlatform(this).IgnoreResult();
  PL_DELETE(&m_Allocator, pVulkanRenderTargetView);
}

plGALUnorderedAccessView* plGALDeviceVulkan::CreateUnorderedAccessViewPlatform(
  plGALResourceBase* pTextureOfBuffer, const plGALUnorderedAccessViewCreationDescription& Description)
{
  plGALUnorderedAccessViewVulkan* pUnorderedAccessView = PL_NEW(&m_Allocator, plGALUnorderedAccessViewVulkan, pTextureOfBuffer, Description);

  if (!pUnorderedAccessView->InitPlatform(this).Succeeded())
  {
    PL_DELETE(&m_Allocator, pUnorderedAccessView);
    return nullptr;
  }

  return pUnorderedAccessView;
}

void plGALDeviceVulkan::DestroyUnorderedAccessViewPlatform(plGALUnorderedAccessView* pUnorderedAccessView)
{
  plGALUnorderedAccessViewVulkan* pUnorderedAccessViewVulkan = static_cast<plGALUnorderedAccessViewVulkan*>(pUnorderedAccessView);
  pUnorderedAccessViewVulkan->DeInitPlatform(this).IgnoreResult();
  PL_DELETE(&m_Allocator, pUnorderedAccessViewVulkan);
}



// Other rendering creation functions
plGALQuery* plGALDeviceVulkan::CreateQueryPlatform(const plGALQueryCreationDescription& Description)
{
  plGALQueryVulkan* pQuery = PL_NEW(&m_Allocator, plGALQueryVulkan, Description);

  if (!pQuery->InitPlatform(this).Succeeded())
  {
    PL_DELETE(&m_Allocator, pQuery);
    return nullptr;
  }

  return pQuery;
}

void plGALDeviceVulkan::DestroyQueryPlatform(plGALQuery* pQuery)
{
  plGALQueryVulkan* pQueryVulkan = static_cast<plGALQueryVulkan*>(pQuery);
  pQueryVulkan->DeInitPlatform(this).IgnoreResult();
  PL_DELETE(&m_Allocator, pQueryVulkan);
}

plGALVertexDeclaration* plGALDeviceVulkan::CreateVertexDeclarationPlatform(const plGALVertexDeclarationCreationDescription& Description)
{
  plGALVertexDeclarationVulkan* pVertexDeclaration = PL_NEW(&m_Allocator, plGALVertexDeclarationVulkan, Description);

  if (pVertexDeclaration->InitPlatform(this).Succeeded())
  {
    return pVertexDeclaration;
  }
  else
  {
    PL_DELETE(&m_Allocator, pVertexDeclaration);
    return nullptr;
  }
}

void plGALDeviceVulkan::DestroyVertexDeclarationPlatform(plGALVertexDeclaration* pVertexDeclaration)
{
  plGALVertexDeclarationVulkan* pVertexDeclarationVulkan = static_cast<plGALVertexDeclarationVulkan*>(pVertexDeclaration);
  plResourceCacheVulkan::ResourceDeleted(pVertexDeclarationVulkan);
  pVertexDeclarationVulkan->DeInitPlatform(this).IgnoreResult();
  PL_DELETE(&m_Allocator, pVertexDeclarationVulkan);
}

plGALTimestampHandle plGALDeviceVulkan::GetTimestampPlatform()
{
  return m_pQueryPool->GetTimestamp();
}

plResult plGALDeviceVulkan::GetTimestampResultPlatform(plGALTimestampHandle hTimestamp, plTime& result)
{
  return m_pQueryPool->GetTimestampResult(hTimestamp, result);
}

// Misc functions

void plGALDeviceVulkan::BeginFramePlatform(const plUInt64 uiRenderFrame)
{
  auto& pCommandEncoder = m_pDefaultPass->m_pCommandEncoderImpl;

  // check if fence is reached
  if (m_PerFrameData[m_uiCurrentPerFrameData].m_uiFrame != ((plUInt64)-1))
  {
    auto& perFrameData = m_PerFrameData[m_uiCurrentPerFrameData];
    for (vk::Fence fence : perFrameData.m_CommandBufferFences)
    {
      vk::Result fenceStatus = m_device.getFenceStatus(fence);
      if (fenceStatus == vk::Result::eNotReady)
      {
        m_device.waitForFences(1, &fence, true, 1000000000);
      }
    }
    perFrameData.m_CommandBufferFences.Clear();

    {
      PL_LOCK(m_PerFrameData[m_uiCurrentPerFrameData].m_pendingDeletionsMutex);
      DeletePendingResources(m_PerFrameData[m_uiCurrentPerFrameData].m_pendingDeletionsPrevious);
    }
    {
      PL_LOCK(m_PerFrameData[m_uiCurrentPerFrameData].m_reclaimResourcesMutex);
      ReclaimResources(m_PerFrameData[m_uiCurrentPerFrameData].m_reclaimResourcesPrevious);
    }
    m_uiSafeFrame = m_PerFrameData[m_uiCurrentPerFrameData].m_uiFrame;
  }
  {
    auto& perFrameData = m_PerFrameData[m_uiNextPerFrameData];
    perFrameData.m_fInvTicksPerSecond = -1.0f;
  }

  m_PerFrameData[m_uiCurrentPerFrameData].m_uiFrame = m_uiFrameCounter;

  m_pQueryPool->BeginFrame(GetCurrentCommandBuffer());
  GetCurrentCommandBuffer();

#if PL_ENABLED(PL_USE_PROFILING)
  plStringBuilder sb;
  sb.SetFormat("Frame {}", uiRenderFrame);
  m_pFrameTimingScope = plProfilingScopeAndMarker::Start(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), sb);
#endif
}

void plGALDeviceVulkan::EndFramePlatform()
{
#if PL_ENABLED(PL_USE_PROFILING)
  {
    // #TODO_VULKAN This is very wasteful, in normal cases the last endPipeline will have submitted the command buffer via the swapchain. Thus, we start and submit a command buffer here with only the timestamp in it.
    GetCurrentCommandBuffer();
    plProfilingScopeAndMarker::Stop(m_pDefaultPass->m_pRenderCommandEncoder.Borrow(), m_pFrameTimingScope);
  }
#endif

  if (m_PerFrameData[m_uiCurrentPerFrameData].m_currentCommandBuffer)
  {
    Submit();
  }

  {
    // Resources can be added to deletion / reclaim outside of the render frame. These will not be covered by the fences. To handle this, we swap the resources arrays so for any newly added resources we know they are not part of the batch that is deleted / reclaimed with the frame.
    auto& currentFrameData = m_PerFrameData[m_uiCurrentPerFrameData];
    {
      PL_LOCK(currentFrameData.m_pendingDeletionsMutex);
      currentFrameData.m_pendingDeletionsPrevious.Swap(currentFrameData.m_pendingDeletions);
    }
    {
      PL_LOCK(currentFrameData.m_reclaimResourcesMutex);
      currentFrameData.m_reclaimResourcesPrevious.Swap(currentFrameData.m_reclaimResources);
    }
  }
  m_uiCurrentPerFrameData = (m_uiCurrentPerFrameData + 1) % PL_ARRAY_SIZE(m_PerFrameData);
  m_uiNextPerFrameData = (m_uiCurrentPerFrameData + 1) % PL_ARRAY_SIZE(m_PerFrameData);
  ++m_uiFrameCounter;
}

void plGALDeviceVulkan::FillCapabilitiesPlatform()
{
  vk::PhysicalDeviceMemoryProperties memProperties = m_physicalDevice.getMemoryProperties();
  vk::PhysicalDeviceFeatures features = m_physicalDevice.getFeatures();

  plUInt64 dedicatedMemory = 0;
  plUInt64 systemMemory = 0;
  for (uint32_t i = 0; i < memProperties.memoryHeapCount; ++i)
  {
    if (memProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
    {
      dedicatedMemory += memProperties.memoryHeaps[i].size;
    }
    else
    {
      systemMemory += memProperties.memoryHeaps[i].size;
    }
  }

  {
    m_Capabilities.m_sAdapterName = plStringUtf8(m_properties.deviceName).GetData();
    m_Capabilities.m_uiDedicatedVRAM = static_cast<plUInt64>(dedicatedMemory);
    m_Capabilities.m_uiDedicatedSystemRAM = static_cast<plUInt64>(systemMemory);
    m_Capabilities.m_uiSharedSystemRAM = static_cast<plUInt64>(0); // TODO
    m_Capabilities.m_bHardwareAccelerated = m_properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
  }

  m_Capabilities.m_bMultithreadedResourceCreation = true;

  m_Capabilities.m_bNoOverwriteBufferUpdate = true; // TODO how to check

  m_Capabilities.m_bShaderStageSupported[plGALShaderStage::VertexShader] = true;
  m_Capabilities.m_bShaderStageSupported[plGALShaderStage::HullShader] = features.tessellationShader;
  m_Capabilities.m_bShaderStageSupported[plGALShaderStage::DomainShader] = features.tessellationShader;
  m_Capabilities.m_bShaderStageSupported[plGALShaderStage::GeometryShader] = features.geometryShader;
  m_Capabilities.m_bShaderStageSupported[plGALShaderStage::PixelShader] = true;
  m_Capabilities.m_bShaderStageSupported[plGALShaderStage::ComputeShader] = true; // we check this when creating the queue, always has to be supported
  m_Capabilities.m_bInstancing = true;
  m_Capabilities.m_b32BitIndices = true;
  m_Capabilities.m_bIndirectDraw = true;
  m_Capabilities.m_uiMaxConstantBuffers = plMath::Min(m_properties.limits.maxDescriptorSetUniformBuffers, (plUInt32)plMath::MaxValue<plUInt16>());
  m_Capabilities.m_uiMaxPushConstantsSize = plMath::Min(m_properties.limits.maxPushConstantsSize, (plUInt32)plMath::MaxValue<plUInt16>());;
  m_Capabilities.m_bTextureArrays = true;
  m_Capabilities.m_bCubemapArrays = true;
#if PL_ENABLED(PL_PLATFORM_LINUX)
  m_Capabilities.m_bSharedTextures = m_extensions.m_bTimelineSemaphore && m_extensions.m_bExternalMemoryFd && m_extensions.m_bExternalSemaphoreFd;
#elif PL_ENABLED(PL_PLATFORM_WINDOWS)
  m_Capabilities.m_bSharedTextures = m_extensions.m_bTimelineSemaphore && m_extensions.m_bExternalMemoryWin32 && m_extensions.m_bExternalSemaphoreWin32;
#else
  PL_ASSERT_NOT_IMPLEMENTED;
#endif
  m_Capabilities.m_uiMaxTextureDimension = m_properties.limits.maxImageDimension1D;
  m_Capabilities.m_uiMaxCubemapDimension = m_properties.limits.maxImageDimensionCube;
  m_Capabilities.m_uiMax3DTextureDimension = m_properties.limits.maxImageDimension3D;
  m_Capabilities.m_uiMaxAnisotropy = static_cast<plUInt16>(m_properties.limits.maxSamplerAnisotropy);
  m_Capabilities.m_uiMaxRendertargets = m_properties.limits.maxColorAttachments;
  m_Capabilities.m_uiUAVCount = plMath::Min(plMath::Min(m_properties.limits.maxDescriptorSetStorageBuffers, m_properties.limits.maxDescriptorSetStorageImages), (plUInt32)plMath::MaxValue<plUInt16>());
  m_Capabilities.m_bAlphaToCoverage = true;
  m_Capabilities.m_bVertexShaderRenderTargetArrayIndex = m_extensions.m_bShaderViewportIndexLayer;

  m_Capabilities.m_bConservativeRasterization = false; // need to query for VK_EXT_CONSERVATIVE_RASTERIZATION

  m_Capabilities.m_FormatSupport.SetCount(plGALResourceFormat::ENUM_COUNT);
  for (plUInt32 i = 0; i < plGALResourceFormat::ENUM_COUNT; i++)
  {
    plGALResourceFormat::Enum format = (plGALResourceFormat::Enum)i;
    const plGALFormatLookupEntryVulkan& entry = m_FormatLookupTable.GetFormatInfo(format);
    const vk::FormatProperties formatProps = GetVulkanPhysicalDevice().getFormatProperties(entry.m_format);

    if (formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage)
    {
      m_Capabilities.m_FormatSupport[i].Add(plGALResourceFormatSupport::Sample);
      vk::ImageFormatProperties props;
      vk::Result res = GetVulkanPhysicalDevice().getImageFormatProperties(entry.m_format, vk::ImageType::e2D, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled, {}, &props);
      if (res == vk::Result::eSuccess)
      {
        if (props.sampleCounts & vk::SampleCountFlagBits::e2)
          m_Capabilities.m_FormatSupport[i].Add(plGALResourceFormatSupport::MSAA2x);
        if (props.sampleCounts & vk::SampleCountFlagBits::e4)
          m_Capabilities.m_FormatSupport[i].Add(plGALResourceFormatSupport::MSAA4x);
        if (props.sampleCounts & vk::SampleCountFlagBits::e8)
          m_Capabilities.m_FormatSupport[i].Add(plGALResourceFormatSupport::MSAA8x);
      }
    }
    if (formatProps.bufferFeatures & vk::FormatFeatureFlagBits::eVertexBuffer)
      m_Capabilities.m_FormatSupport[i].Add(plGALResourceFormatSupport::VertexAttribute);
    if (plGALResourceFormat::IsDepthFormat(format))
    {
      if (formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        m_Capabilities.m_FormatSupport[i].Add(plGALResourceFormatSupport::Render);
    }
    else
    {
      if (formatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment)
        m_Capabilities.m_FormatSupport[i].Add(plGALResourceFormatSupport::Render);
    }
  }
}

void plGALDeviceVulkan::FlushPlatform()
{
  Submit();
}

void plGALDeviceVulkan::WaitIdlePlatform()
{
  // Make sure command buffers get flushed.
  Submit();
  m_device.waitIdle();
  DestroyDeadObjects();
  for (plUInt32 i = 0; i < PL_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    // First, we wait for all fences for all submit calls. This is necessary to make sure no resources of the frame are still in use by the GPU.
    auto& perFrameData = m_PerFrameData[i];
    for (vk::Fence fence : perFrameData.m_CommandBufferFences)
    {
      vk::Result fenceStatus = m_device.getFenceStatus(fence);
      if (fenceStatus == vk::Result::eNotReady)
      {
        m_device.waitForFences(1, &fence, true, 1000000000);
      }
    }
    perFrameData.m_CommandBufferFences.Clear();
  }

  for (plUInt32 i = 0; i < PL_ARRAY_SIZE(m_PerFrameData); ++i)
  {
    {
      PL_LOCK(m_PerFrameData[i].m_pendingDeletionsMutex);
      DeletePendingResources(m_PerFrameData[i].m_pendingDeletionsPrevious);
      DeletePendingResources(m_PerFrameData[i].m_pendingDeletions);
    }
    {
      PL_LOCK(m_PerFrameData[i].m_reclaimResourcesMutex);
      ReclaimResources(m_PerFrameData[i].m_reclaimResourcesPrevious);
      ReclaimResources(m_PerFrameData[i].m_reclaimResources);
    }
  }
}

vk::PipelineStageFlags plGALDeviceVulkan::GetSupportedStages() const
{
  return m_supportedStages;
}

plInt32 plGALDeviceVulkan::GetMemoryIndex(vk::MemoryPropertyFlags properties, const vk::MemoryRequirements& requirements) const
{

  for (plUInt32 i = 0; i < m_memoryProperties.memoryTypeCount; ++i)
  {
    const vk::MemoryType& type = m_memoryProperties.memoryTypes[i];
    if (requirements.memoryTypeBits & (1 << i) && (type.propertyFlags & properties))
    {
      return i;
    }
  }

  return -1;
}

void plGALDeviceVulkan::DeleteLaterImpl(const PendingDeletion& deletion)
{
  PL_LOCK(m_PerFrameData[m_uiCurrentPerFrameData].m_pendingDeletionsMutex);
  m_PerFrameData[m_uiCurrentPerFrameData].m_pendingDeletions.PushBack(deletion);
}

void plGALDeviceVulkan::ReclaimLater(const ReclaimResource& reclaim)
{
  PL_LOCK(m_PerFrameData[m_uiCurrentPerFrameData].m_reclaimResourcesMutex);
  m_PerFrameData[m_uiCurrentPerFrameData].m_reclaimResources.PushBack(reclaim);
}

void plGALDeviceVulkan::DeletePendingResources(plDeque<PendingDeletion>& pendingDeletions)
{
  for (PendingDeletion& deletion : pendingDeletions)
  {
    switch (deletion.m_type)
    {
      case vk::ObjectType::eUnknown:
        if (deletion.m_flags.IsSet(PendingDeletionFlags::IsFileDescriptor))
        {
#if PL_ENABLED(PL_PLATFORM_LINUX)
          int fileDescriptor = static_cast<int>(reinterpret_cast<size_t>(deletion.m_pObject));
          int res = close(fileDescriptor);
          if (res == -1)
          {
            plLog::Error("close() failed on file descriptor with errno: {}", plArgErrno(errno));
          }
#else
          PL_ASSERT_NOT_IMPLEMENTED;
#endif
        }
        else
        {
          PL_REPORT_FAILURE("Unknown pending deletion");
        }
        break;
      case vk::ObjectType::eImageView:
        m_device.destroyImageView(reinterpret_cast<vk::ImageView&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eImage:
      {
        auto& image = reinterpret_cast<vk::Image&>(deletion.m_pObject);
        OnBeforeImageDestroyed.Broadcast(OnBeforeImageDestroyedData{image, *this});
        if (deletion.m_flags.IsSet(PendingDeletionFlags::UsesExternalMemory))
        {
          m_device.destroyImage(image);
          auto& deviceMemory = reinterpret_cast<vk::DeviceMemory&>(deletion.m_pContext);
          m_device.freeMemory(deviceMemory);
        }
        else
        {
          plMemoryAllocatorVulkan::DestroyImage(image, deletion.m_allocation);
        }
      }
      break;
      case vk::ObjectType::eBuffer:
        plMemoryAllocatorVulkan::DestroyBuffer(reinterpret_cast<vk::Buffer&>(deletion.m_pObject), deletion.m_allocation);
        break;
      case vk::ObjectType::eBufferView:
        m_device.destroyBufferView(reinterpret_cast<vk::BufferView&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eFramebuffer:
        m_device.destroyFramebuffer(reinterpret_cast<vk::Framebuffer&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eRenderPass:
        m_device.destroyRenderPass(reinterpret_cast<vk::RenderPass&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eSampler:
        m_device.destroySampler(reinterpret_cast<vk::Sampler&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eSemaphore:
        m_device.destroySemaphore(reinterpret_cast<vk::Semaphore&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eSwapchainKHR:
        m_device.destroySwapchainKHR(reinterpret_cast<vk::SwapchainKHR&>(deletion.m_pObject));
        break;
      case vk::ObjectType::eSurfaceKHR:
        m_instance.destroySurfaceKHR(reinterpret_cast<vk::SurfaceKHR&>(deletion.m_pObject));
        if (plWindowBase* pWindow = reinterpret_cast<plWindowBase*>(deletion.m_pContext))
        {
          pWindow->RemoveReference();
        }
        break;
      case vk::ObjectType::eShaderModule:
        m_device.destroyShaderModule(reinterpret_cast<vk::ShaderModule&>(deletion.m_pObject));
        break;
      case vk::ObjectType::ePipeline:
        m_device.destroyPipeline(reinterpret_cast<vk::Pipeline&>(deletion.m_pObject));
        break;
      default:
        PL_REPORT_FAILURE("This object type is not implemented");
        break;
    }
  }
  pendingDeletions.Clear();
}

void plGALDeviceVulkan::ReclaimResources(plDeque<ReclaimResource>& resources)
{
  for (ReclaimResource& resource : resources)
  {
    switch (resource.m_type)
    {
      case vk::ObjectType::eSemaphore:
        plSemaphorePoolVulkan::ReclaimSemaphore(reinterpret_cast<vk::Semaphore&>(resource.m_pObject));
        break;
      case vk::ObjectType::eFence:
        plFencePoolVulkan::ReclaimFence(reinterpret_cast<vk::Fence&>(resource.m_pObject));
        break;
      case vk::ObjectType::eCommandBuffer:
        static_cast<plCommandBufferPoolVulkan*>(resource.m_pContext)->ReclaimCommandBuffer(reinterpret_cast<vk::CommandBuffer&>(resource.m_pObject));
        break;
      case vk::ObjectType::eDescriptorPool:
        plDescriptorSetPoolVulkan::ReclaimPool(reinterpret_cast<vk::DescriptorPool&>(resource.m_pObject));
        break;
      default:
        PL_REPORT_FAILURE("This object type is not implemented");
        break;
    }
  }
  resources.Clear();
}

void plGALDeviceVulkan::FillFormatLookupTable()
{
  /// The list below is in the same order as the plGALResourceFormat enum. No format should be missing except the ones that are just different names for the same enum value.
  vk::Format R32G32B32A32_Formats[] = {vk::Format::eR32G32B32A32Sfloat, vk::Format::eR32G32B32A32Uint, vk::Format::eR32G32B32A32Sint};
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAFloat, plGALFormatLookupEntryVulkan(vk::Format::eR32G32B32A32Sfloat, R32G32B32A32_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAUInt, plGALFormatLookupEntryVulkan(vk::Format::eR32G32B32A32Uint, R32G32B32A32_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAInt, plGALFormatLookupEntryVulkan(vk::Format::eR32G32B32A32Sint, R32G32B32A32_Formats));

  vk::Format R32G32B32_Formats[] = {vk::Format::eR32G32B32Sfloat, vk::Format::eR32G32B32Uint, vk::Format::eR32G32B32Sint};
  // TODO 3-channel formats are not really supported under vulkan judging by experience
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBFloat, plGALFormatLookupEntryVulkan(vk::Format::eR32G32B32Sfloat, R32G32B32_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBUInt, plGALFormatLookupEntryVulkan(vk::Format::eR32G32B32Uint, R32G32B32_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBInt, plGALFormatLookupEntryVulkan(vk::Format::eR32G32B32Sint, R32G32B32_Formats));

  // TODO dunno if these are actually supported for the respective Vulkan device
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::B5G6R5UNormalized, plGALFormatLookupEntryVulkan(vk::Format::eR5G6B5UnormPack16));

  vk::Format B8G8R8A8_Formats[] = {vk::Format::eB8G8R8A8Unorm, vk::Format::eB8G8R8A8Srgb};
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BGRAUByteNormalized, plGALFormatLookupEntryVulkan(vk::Format::eB8G8R8A8Unorm, B8G8R8A8_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BGRAUByteNormalizedsRGB, plGALFormatLookupEntryVulkan(vk::Format::eB8G8R8A8Srgb, B8G8R8A8_Formats));

  vk::Format R16G16B16A16_Formats[] = {vk::Format::eR16G16B16A16Sfloat, vk::Format::eR16G16B16A16Uint, vk::Format::eR16G16B16A16Unorm, vk::Format::eR16G16B16A16Sint, vk::Format::eR16G16B16A16Snorm};
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAHalf, plGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Sfloat, R16G16B16A16_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAUShort, plGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Uint, R16G16B16A16_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAUShortNormalized, plGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Unorm, R16G16B16A16_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAShort, plGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Sint, R16G16B16A16_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAShortNormalized, plGALFormatLookupEntryVulkan(vk::Format::eR16G16B16A16Snorm, R16G16B16A16_Formats));

  vk::Format R32G32_Formats[] = {vk::Format::eR32G32Sfloat, vk::Format::eR32G32Uint, vk::Format::eR32G32Sint};
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGFloat, plGALFormatLookupEntryVulkan(vk::Format::eR32G32Sfloat, R32G32_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGUInt, plGALFormatLookupEntryVulkan(vk::Format::eR32G32Uint, R32G32_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGInt, plGALFormatLookupEntryVulkan(vk::Format::eR32G32Sint, R32G32_Formats));

  vk::Format R10G10B10A2_Formats[] = {vk::Format::eA2B10G10R10UintPack32, vk::Format::eA2B10G10R10UnormPack32};
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGB10A2UInt, plGALFormatLookupEntryVulkan(vk::Format::eA2B10G10R10UintPack32, R10G10B10A2_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGB10A2UIntNormalized, plGALFormatLookupEntryVulkan(vk::Format::eA2B10G10R10UnormPack32, R10G10B10A2_Formats));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RG11B10Float, plGALFormatLookupEntryVulkan(vk::Format::eB10G11R11UfloatPack32));

  vk::Format R8G8B8A8_Formats[] = {vk::Format::eR8G8B8A8Unorm, vk::Format::eR8G8B8A8Srgb, vk::Format::eR8G8B8A8Uint, vk::Format::eR8G8B8A8Snorm, vk::Format::eR8G8B8A8Sint};
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAUByteNormalized, plGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Unorm, R8G8B8A8_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAUByteNormalizedsRGB, plGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Srgb, R8G8B8A8_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAUByte, plGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Uint, R8G8B8A8_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAByteNormalized, plGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Snorm, R8G8B8A8_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGBAByte, plGALFormatLookupEntryVulkan(vk::Format::eR8G8B8A8Sint, R8G8B8A8_Formats));

  vk::Format R16G16_Formats[] = {vk::Format::eR16G16Sfloat, vk::Format::eR16G16Uint, vk::Format::eR16G16Unorm, vk::Format::eR16G16Sint, vk::Format::eR16G16Snorm};
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGHalf, plGALFormatLookupEntryVulkan(vk::Format::eR16G16Sfloat, R16G16_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGUShort, plGALFormatLookupEntryVulkan(vk::Format::eR16G16Uint, R16G16_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGUShortNormalized, plGALFormatLookupEntryVulkan(vk::Format::eR16G16Unorm, R16G16_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGShort, plGALFormatLookupEntryVulkan(vk::Format::eR16G16Sint, R16G16_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGShortNormalized, plGALFormatLookupEntryVulkan(vk::Format::eR16G16Snorm, R16G16_Formats));

  vk::Format R8G8_Formats[] = {vk::Format::eR8G8Uint, vk::Format::eR8G8Unorm, vk::Format::eR8G8Sint, vk::Format::eR8G8Snorm};
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGUByte, plGALFormatLookupEntryVulkan(vk::Format::eR8G8Uint, R8G8_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGUByteNormalized, plGALFormatLookupEntryVulkan(vk::Format::eR8G8Unorm, R8G8_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGByte, plGALFormatLookupEntryVulkan(vk::Format::eR8G8Sint, R8G8_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RGByteNormalized, plGALFormatLookupEntryVulkan(vk::Format::eR8G8Snorm, R8G8_Formats));

  vk::Format R32_Formats[] = {vk::Format::eR32Sfloat, vk::Format::eR32Uint, vk::Format::eR32Sint};
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RFloat, plGALFormatLookupEntryVulkan(vk::Format::eR32Sfloat, R32_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RUInt, plGALFormatLookupEntryVulkan(vk::Format::eR32Uint, R32_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RInt, plGALFormatLookupEntryVulkan(vk::Format::eR32Sint, R32_Formats));

  vk::Format R16_Formats[] = {vk::Format::eR16Sfloat, vk::Format::eR16Uint, vk::Format::eR16Unorm, vk::Format::eR16Sint, vk::Format::eR16Snorm};
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RHalf, plGALFormatLookupEntryVulkan(vk::Format::eR16Sfloat, R16_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RUShort, plGALFormatLookupEntryVulkan(vk::Format::eR16Uint, R16_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RUShortNormalized, plGALFormatLookupEntryVulkan(vk::Format::eR16Unorm, R16_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RShort, plGALFormatLookupEntryVulkan(vk::Format::eR16Sint, R16_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RShortNormalized, plGALFormatLookupEntryVulkan(vk::Format::eR16Snorm, R16_Formats));

  vk::Format R8_Formats[] = {vk::Format::eR8Uint, vk::Format::eR8Unorm, vk::Format::eR8Sint, vk::Format::eR8Snorm};
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RUByte, plGALFormatLookupEntryVulkan(vk::Format::eR8Uint, R8_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RUByteNormalized, plGALFormatLookupEntryVulkan(vk::Format::eR8Unorm, R8_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RByte, plGALFormatLookupEntryVulkan(vk::Format::eR8Sint, R8_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::RByteNormalized, plGALFormatLookupEntryVulkan(vk::Format::eR8Snorm, R8_Formats));

  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::AUByteNormalized, plGALFormatLookupEntryVulkan(vk::Format::eR8Unorm));

  auto SelectDepthFormat = [&](const std::vector<vk::Format>& list) -> vk::Format {
    for (auto& format : list)
    {
      vk::FormatProperties formatProperties;
      m_physicalDevice.getFormatProperties(format, &formatProperties);
      if (formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
        return format;
    }
    return vk::Format::eUndefined;
  };

  auto SelectStorageFormat = [](vk::Format depthFormat) -> vk::Format {
    switch (depthFormat)
    {
      case vk::Format::eD16Unorm:
        return vk::Format::eR16Unorm;
      case vk::Format::eD16UnormS8Uint:
        return vk::Format::eUndefined;
      case vk::Format::eD24UnormS8Uint:
        return vk::Format::eUndefined;
      case vk::Format::eD32Sfloat:
        return vk::Format::eR32Sfloat;
      case vk::Format::eD32SfloatS8Uint:
        return vk::Format::eR32Sfloat;
      default:
        return vk::Format::eUndefined;
    }
  };

  // Select smallest available depth format.  #TODO_VULKAN support packed eX8D24UnormPack32?
  vk::Format depthFormat = SelectDepthFormat({vk::Format::eD16Unorm, vk::Format::eD24UnormS8Uint, vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint});
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::D16, plGALFormatLookupEntryVulkan(depthFormat).R(SelectStorageFormat(depthFormat)));

  // Select closest depth stencil format.
  depthFormat = SelectDepthFormat({vk::Format::eD24UnormS8Uint, vk::Format::eD32SfloatS8Uint, vk::Format::eD16UnormS8Uint});
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::D24S8, plGALFormatLookupEntryVulkan(depthFormat).R(SelectStorageFormat(depthFormat)));

  // Select biggest depth format.
  depthFormat = SelectDepthFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint, vk::Format::eD16Unorm});
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::DFloat, plGALFormatLookupEntryVulkan(depthFormat).R(SelectStorageFormat(depthFormat)));

  vk::Format BC1_Formats[] = {vk::Format::eBc1RgbaUnormBlock, vk::Format::eBc1RgbaSrgbBlock};
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC1, plGALFormatLookupEntryVulkan(vk::Format::eBc1RgbaUnormBlock, BC1_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC1sRGB, plGALFormatLookupEntryVulkan(vk::Format::eBc1RgbaSrgbBlock, BC1_Formats));

  vk::Format BC2_Formats[] = {vk::Format::eBc2UnormBlock, vk::Format::eBc2SrgbBlock};
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC2, plGALFormatLookupEntryVulkan(vk::Format::eBc2UnormBlock, BC2_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC2sRGB, plGALFormatLookupEntryVulkan(vk::Format::eBc2SrgbBlock, BC2_Formats));

  vk::Format BC3_Formats[] = {vk::Format::eBc3UnormBlock, vk::Format::eBc3SrgbBlock};
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC3, plGALFormatLookupEntryVulkan(vk::Format::eBc3UnormBlock, BC3_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC3sRGB, plGALFormatLookupEntryVulkan(vk::Format::eBc3SrgbBlock, BC3_Formats));

  vk::Format BC4_Formats[] = {vk::Format::eBc4UnormBlock, vk::Format::eBc4SnormBlock};
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC4UNormalized, plGALFormatLookupEntryVulkan(vk::Format::eBc4UnormBlock, BC4_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC4Normalized, plGALFormatLookupEntryVulkan(vk::Format::eBc4SnormBlock, BC4_Formats));

  vk::Format BC5_Formats[] = {vk::Format::eBc5UnormBlock, vk::Format::eBc5SnormBlock};
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC5UNormalized, plGALFormatLookupEntryVulkan(vk::Format::eBc5UnormBlock, BC5_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC5Normalized, plGALFormatLookupEntryVulkan(vk::Format::eBc5SnormBlock, BC5_Formats));

  vk::Format BC6_Formats[] = {vk::Format::eBc6HUfloatBlock, vk::Format::eBc6HSfloatBlock};
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC6UFloat, plGALFormatLookupEntryVulkan(vk::Format::eBc6HUfloatBlock, BC6_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC6Float, plGALFormatLookupEntryVulkan(vk::Format::eBc6HSfloatBlock, BC6_Formats));

  vk::Format BC7_Formats[] = {vk::Format::eBc7UnormBlock, vk::Format::eBc7SrgbBlock};
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC7UNormalized, plGALFormatLookupEntryVulkan(vk::Format::eBc7UnormBlock, BC7_Formats));
  m_FormatLookupTable.SetFormatInfo(plGALResourceFormat::BC7UNormalizedsRGB, plGALFormatLookupEntryVulkan(vk::Format::eBc7SrgbBlock, BC7_Formats));

  if (false)
  {
    PL_LOG_BLOCK("GAL Resource Formats");
    for (plUInt32 i = 1; i < plGALResourceFormat::ENUM_COUNT; i++)
    {
      const plGALFormatLookupEntryVulkan& entry = m_FormatLookupTable.GetFormatInfo((plGALResourceFormat::Enum)i);

      vk::FormatProperties formatProperties;
      m_physicalDevice.getFormatProperties(entry.m_format, &formatProperties);

      const bool bSampled = static_cast<bool>(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImage);
      const bool bColorAttachment = static_cast<bool>(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eColorAttachment);
      const bool bDepthAttachment = static_cast<bool>(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment);

      const bool bTexel = static_cast<bool>(formatProperties.bufferFeatures & vk::FormatFeatureFlagBits::eUniformTexelBuffer);
      const bool bStorageTexel = static_cast<bool>(formatProperties.bufferFeatures & vk::FormatFeatureFlagBits::eStorageTexelBuffer);
      const bool bVertex = static_cast<bool>(formatProperties.bufferFeatures & vk::FormatFeatureFlagBits::eVertexBuffer);

      plStringBuilder sTemp;
      plReflectionUtils::EnumerationToString(plGetStaticRTTI<plGALResourceFormat>(), i, sTemp, plReflectionUtils::EnumConversionMode::ValueNameOnly);

      plLog::Info("OptTiling S: {}, CA: {}, DA: {}. Buffer: T: {}, ST: {}, V: {}, Format {} -> {}", bSampled ? 1 : 0, bColorAttachment ? 1 : 0, bDepthAttachment ? 1 : 0, bTexel ? 1 : 0, bStorageTexel ? 1 : 0, bVertex ? 1 : 0, sTemp, vk::to_string(entry.m_format).c_str());
    }
  }
}

const plGALSharedTexture* plGALDeviceVulkan::GetSharedTexture(plGALTextureHandle hTexture) const
{
  auto pTexture = GetTexture(hTexture);
  if (pTexture == nullptr)
  {
    return nullptr;
  }

  // Resolve proxy texture if any
  return static_cast<const plGALSharedTextureVulkan*>(pTexture->GetParentResource());
}

void plGALDeviceVulkan::AddWaitSemaphore(const SemaphoreInfo& waitSemaphore)
{
  // #TODO_VULKAN Assert is in render pipeline, thread safety
  if (waitSemaphore.m_type == vk::SemaphoreType::eTimeline)
    m_waitSemaphores.Insert(waitSemaphore, 0);
  else
    m_waitSemaphores.PushBack(waitSemaphore);
}

void plGALDeviceVulkan::AddSignalSemaphore(const SemaphoreInfo& signalSemaphore)
{
  // #TODO_VULKAN Assert is in render pipeline, thread safety
  if (signalSemaphore.m_type == vk::SemaphoreType::eTimeline)
    m_signalSemaphores.Insert(signalSemaphore, 0);
  else
    m_signalSemaphores.PushBack(signalSemaphore);
}


PL_STATICLINK_FILE(RendererVulkan, RendererVulkan_Device_Implementation_DeviceVulkan);
