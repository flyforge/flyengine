#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <GameEngine/GameApplication/GameApplication.h>
#include <OpenXRPlugin/OpenXRDeclarations.h>
#include <OpenXRPlugin/OpenXRHandTracking.h>
#include <OpenXRPlugin/OpenXRInputDevice.h>
#include <OpenXRPlugin/OpenXRRemoting.h>
#include <OpenXRPlugin/OpenXRSingleton.h>
#include <OpenXRPlugin/OpenXRSpatialAnchors.h>
#include <OpenXRPlugin/OpenXRSwapChain.h>

#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <Texture/Image/Formats/ImageFormatMappings.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/World/World.h>
#include <GameEngine/XR/StageSpaceComponent.h>
#include <GameEngine/XR/XRWindow.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
#  include <RendererDX11/Device/DeviceDX11.h>
#endif

#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
#  include <winrt/Windows.Graphics.Holographic.h>
#  include <winrt/Windows.UI.Core.h>
#  include <winrt/base.h>
#endif

#include <vector>

PL_CHECK_AT_COMPILETIME(plGALMSAASampleCount::None == 1);
PL_CHECK_AT_COMPILETIME(plGALMSAASampleCount::TwoSamples == 2);
PL_CHECK_AT_COMPILETIME(plGALMSAASampleCount::FourSamples == 4);
PL_CHECK_AT_COMPILETIME(plGALMSAASampleCount::EightSamples == 8);

PL_IMPLEMENT_SINGLETON(plOpenXR);

static plOpenXR g_OpenXRSingleton;

XrBool32 xrDebugCallback(XrDebugUtilsMessageSeverityFlagsEXT messageSeverity, XrDebugUtilsMessageTypeFlagsEXT messageTypes, const XrDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
  switch (messageSeverity)
  {
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      plLog::Debug("XR: {}", pCallbackData->message);
      break;
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      plLog::Info("XR: {}", pCallbackData->message);
      break;
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      plLog::Warning("XR: {}", pCallbackData->message);
      break;
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      plLog::Error("XR: {}", pCallbackData->message);
      break;
    default:
      break;
  }
  // Only layers are allowed to return true here.
  return XR_FALSE;
}

plOpenXR::plOpenXR()
  : m_SingletonRegistrar(this)
{
#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
  m_pRemoting = PL_DEFAULT_NEW(plOpenXRRemoting, this);
#endif
}

plOpenXR::~plOpenXR() = default;

bool plOpenXR::GetDepthComposition() const
{
  return m_Extensions.m_bDepthComposition;
}

bool plOpenXR::IsHmdPresent() const
{
  XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
  systemInfo.formFactor = XrFormFactor::XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
  uint64_t systemId = XR_NULL_SYSTEM_ID;
  XrResult res = xrGetSystem(m_pInstance, &systemInfo, &systemId);

  return res == XrResult::XR_SUCCESS;
}

XrResult plOpenXR::SelectExtensions(plHybridArray<const char*, 6>& extensions)
{
  // Fetch the list of extensions supported by the runtime.
  plUInt32 extensionCount;
  XR_SUCCEED_OR_RETURN_LOG(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr));
  std::vector<XrExtensionProperties> extensionProperties(extensionCount, {XR_TYPE_EXTENSION_PROPERTIES});
  XR_SUCCEED_OR_RETURN_LOG(xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensionProperties.data()));

  // Add a specific extension to the list of extensions to be enabled, if it is supported.
  auto AddExtIfSupported = [&](const char* extensionName, bool& enableFlag) -> XrResult {
    auto it = std::find_if(begin(extensionProperties), end(extensionProperties), [&](const XrExtensionProperties& prop) { return plStringUtils::IsEqual(prop.extensionName, extensionName); });
    if (it != end(extensionProperties))
    {
      extensions.PushBack(extensionName);
      enableFlag = true;
      return XR_SUCCESS;
    }
    enableFlag = false;
    return XR_ERROR_EXTENSION_NOT_PRESENT;
  };

  // D3D11 extension is required so check that it was added.
  XR_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(XR_KHR_D3D11_ENABLE_EXTENSION_NAME, m_Extensions.m_bD3D11));

  AddExtIfSupported(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME, m_Extensions.m_bDepthComposition);
  AddExtIfSupported(XR_MSFT_UNBOUNDED_REFERENCE_SPACE_EXTENSION_NAME, m_Extensions.m_bUnboundedReferenceSpace);
  AddExtIfSupported(XR_MSFT_SPATIAL_ANCHOR_EXTENSION_NAME, m_Extensions.m_bSpatialAnchor);
  AddExtIfSupported(XR_EXT_HAND_TRACKING_EXTENSION_NAME, m_Extensions.m_bHandTracking);
  AddExtIfSupported(XR_MSFT_HAND_INTERACTION_EXTENSION_NAME, m_Extensions.m_bHandInteraction);
  AddExtIfSupported(XR_MSFT_HAND_TRACKING_MESH_EXTENSION_NAME, m_Extensions.m_bHandTrackingMesh);

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  AddExtIfSupported(XR_EXT_DEBUG_UTILS_EXTENSION_NAME, m_Extensions.m_bDebugUtils);
#endif

#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
  AddExtIfSupported(XR_MSFT_HOLOGRAPHIC_WINDOW_ATTACHMENT_EXTENSION_NAME, m_Extensions.m_bHolographicWindowAttachment);
#endif

#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
  AddExtIfSupported(XR_MSFT_HOLOGRAPHIC_REMOTING_EXTENSION_NAME, m_Extensions.m_bRemoting);
#endif

#ifdef BUILDSYSTEM_ENABLE_OPENXR_PREVIEW_SUPPORT
#endif
  return XR_SUCCESS;
}

XrResult plOpenXR::SelectLayers(plHybridArray<const char*, 6>& layers)
{
  plUInt32 layerCount;
  XR_SUCCEED_OR_RETURN_LOG(xrEnumerateApiLayerProperties(0, &layerCount, nullptr));
  std::vector<XrApiLayerProperties> layerProperties(layerCount, {XR_TYPE_API_LAYER_PROPERTIES});
  XR_SUCCEED_OR_RETURN_LOG(xrEnumerateApiLayerProperties(layerCount, &layerCount, layerProperties.data()));

  // Add a specific extension to the list of extensions to be enabled, if it is supported.
  auto AddExtIfSupported = [&](const char* layerName, bool& enableFlag) -> XrResult {
    auto it = std::find_if(begin(layerProperties), end(layerProperties), [&](const XrApiLayerProperties& prop) { return plStringUtils::IsEqual(prop.layerName, layerName); });
    if (it != end(layerProperties))
    {
      layers.PushBack(layerName);
      enableFlag = true;
      return XR_SUCCESS;
    }
    enableFlag = false;
    return XR_ERROR_EXTENSION_NOT_PRESENT;
  };

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  AddExtIfSupported("XR_APILAYER_LUNARG_core_validation", m_Extensions.m_bValidation);
#endif

  return XR_SUCCESS;
}

#define PL_GET_INSTANCE_PROC_ADDR(name) (void)xrGetInstanceProcAddr(m_pInstance, #name, reinterpret_cast<PFN_xrVoidFunction*>(&m_Extensions.pfn_##name));

plResult plOpenXR::Initialize()
{
  if (m_pInstance != XR_NULL_HANDLE)
    return PL_SUCCESS;

  // Build out the extensions to enable. Some extensions are required and some are optional.
  plHybridArray<const char*, 6> enabledExtensions;
  if (SelectExtensions(enabledExtensions) != XR_SUCCESS)
    return PL_FAILURE;

  plHybridArray<const char*, 6> enabledLayers;
  if (SelectLayers(enabledLayers) != XR_SUCCESS)
    return PL_FAILURE;

  // Create the instance with desired extensions.
  XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
  createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.GetCount();
  createInfo.enabledExtensionNames = enabledExtensions.GetData();
  createInfo.enabledApiLayerCount = (uint32_t)enabledLayers.GetCount();
  createInfo.enabledApiLayerNames = enabledLayers.GetData();

  plStringUtils::Copy(createInfo.applicationInfo.applicationName, PL_ARRAY_SIZE(createInfo.applicationInfo.applicationName), plApplication::GetApplicationInstance()->GetApplicationName());
  plStringUtils::Copy(createInfo.applicationInfo.engineName, PL_ARRAY_SIZE(createInfo.applicationInfo.engineName), "plEngine");
  createInfo.applicationInfo.engineVersion = 1;
  createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
  createInfo.applicationInfo.applicationVersion = 1;
  XrResult res = xrCreateInstance(&createInfo, &m_pInstance);
  if (res != XR_SUCCESS)
  {
    plLog::Error("InitSystem xrCreateInstance failed: {}", res);
    Deinitialize();
    return PL_FAILURE;
  }
  XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
  res = xrGetInstanceProperties(m_pInstance, &instanceProperties);
  if (res != XR_SUCCESS)
  {
    plLog::Error("InitSystem xrGetInstanceProperties failed: {}", res);
    Deinitialize();
    return PL_FAILURE;
  }

  plStringBuilder sTemp;
  m_Info.m_sDeviceDriver = plConversionUtils::ToString(instanceProperties.runtimeVersion, sTemp);

  PL_GET_INSTANCE_PROC_ADDR(xrGetD3D11GraphicsRequirementsKHR);

  if (m_Extensions.m_bSpatialAnchor)
  {
    PL_GET_INSTANCE_PROC_ADDR(xrCreateSpatialAnchorMSFT);
    PL_GET_INSTANCE_PROC_ADDR(xrCreateSpatialAnchorSpaceMSFT);
    PL_GET_INSTANCE_PROC_ADDR(xrDestroySpatialAnchorMSFT);
  }

  if (m_Extensions.m_bHandTracking)
  {
    PL_GET_INSTANCE_PROC_ADDR(xrCreateHandTrackerEXT);
    PL_GET_INSTANCE_PROC_ADDR(xrDestroyHandTrackerEXT);
    PL_GET_INSTANCE_PROC_ADDR(xrLocateHandJointsEXT);
  }

  if (m_Extensions.m_bHandTrackingMesh)
  {
    PL_GET_INSTANCE_PROC_ADDR(xrCreateHandMeshSpaceMSFT);
    PL_GET_INSTANCE_PROC_ADDR(xrUpdateHandMeshMSFT);
  }

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  if (m_Extensions.m_bDebugUtils)
  {
    PL_GET_INSTANCE_PROC_ADDR(xrCreateDebugUtilsMessengerEXT);
    PL_GET_INSTANCE_PROC_ADDR(xrDestroyDebugUtilsMessengerEXT);
  }
#endif

#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
  if (m_Extensions.m_bRemoting)
  {
    PL_GET_INSTANCE_PROC_ADDR(xrRemotingSetContextPropertiesMSFT);
    PL_GET_INSTANCE_PROC_ADDR(xrRemotingConnectMSFT);
    PL_GET_INSTANCE_PROC_ADDR(xrRemotingDisconnectMSFT);
    PL_GET_INSTANCE_PROC_ADDR(xrRemotingGetConnectionStateMSFT);
  }
#endif

  m_pInput = PL_DEFAULT_NEW(plOpenXRInputDevice, this);

  m_ExecutionEventsId = plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(plMakeDelegate(&plOpenXR::GameApplicationEventHandler, this));

  res = InitSystem();
  if (res != XR_SUCCESS)
  {
    plLog::Error("InitSystem failed: {}", res);
    Deinitialize();
    return PL_FAILURE;
  }

  plLog::Success("OpenXR {0} v{1} initialized successfully.", instanceProperties.runtimeName, instanceProperties.runtimeVersion);
  return PL_SUCCESS;
}

void plOpenXR::Deinitialize()
{
#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
  m_pRemoting->Disconnect().IgnoreResult();
#endif

  if (m_ExecutionEventsId != 0)
  {
    plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(m_ExecutionEventsId);
  }

  plGALDevice::GetDefaultDevice()->DestroySwapChain(m_hSwapChain);
  m_hSwapChain.Invalidate();

  DeinitSession();
  DeinitSystem();

  m_pInput = nullptr;

  if (m_pInstance)
  {
    xrDestroyInstance(m_pInstance);
    m_pInstance = XR_NULL_HANDLE;
  }
}

bool plOpenXR::IsInitialized() const
{
  return m_pInstance != XR_NULL_HANDLE;
}

const plHMDInfo& plOpenXR::GetHmdInfo() const
{
  PL_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");
  return m_Info;
}

plXRInputDevice& plOpenXR::GetXRInput() const
{
  return *(m_pInput.Borrow());
}

plUniquePtr<plActor> plOpenXR::CreateActor(plView* pView, plGALMSAASampleCount::Enum msaaCount, plUniquePtr<plWindowBase> pCompanionWindow, plUniquePtr<plWindowOutputTargetGAL> pCompanionWindowOutput)
{
  PL_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");

  XrResult res = InitSession();
  if (res != XrResult::XR_SUCCESS)
  {
    plLog::Error("InitSession failed: {}", res);
    return {};
  }

  plGALXRSwapChain::SetFactoryMethod([this, msaaCount](plXRInterface* pXrInterface) -> plGALSwapChainHandle { return plGALDevice::GetDefaultDevice()->CreateSwapChain([this, pXrInterface, msaaCount](plAllocator* pAllocator) -> plGALSwapChain* { return PL_NEW(pAllocator, plGALOpenXRSwapChain, this, msaaCount); }); });
  PL_SCOPE_EXIT(plGALXRSwapChain::SetFactoryMethod({}););

  m_hSwapChain = plGALXRSwapChain::Create(this);
  if (m_hSwapChain.IsInvalidated())
  {
    DeinitSession();
    plLog::Error("InitSwapChain failed: {}", res);
    return {};
  }

  const plGALOpenXRSwapChain* pSwapChain = static_cast<const plGALOpenXRSwapChain*>(plGALDevice::GetDefaultDevice()->GetSwapChain(m_hSwapChain));
  m_Info.m_vEyeRenderTargetSize = pSwapChain->GetRenderTargetSize();

  {
    PL_ASSERT_DEV(pView->GetCamera() != nullptr, "The provided view requires a camera to be set.");
    SetHMDCamera(pView->GetCamera());
  }

  plUniquePtr<plActor> pActor = PL_DEFAULT_NEW(plActor, "OpenXR", this);

  PL_ASSERT_DEV((pCompanionWindow != nullptr) == (pCompanionWindowOutput != nullptr), "Both companionWindow and companionWindowOutput must either be null or valid.");
  PL_ASSERT_DEV(pCompanionWindow == nullptr || SupportsCompanionView(), "If a companionWindow is set, SupportsCompanionView() must be true.");

  plUniquePtr<plActorPluginWindowXR> pActorPlugin = PL_DEFAULT_NEW(plActorPluginWindowXR, this, std::move(pCompanionWindow), std::move(pCompanionWindowOutput));
  m_pCompanion = static_cast<plWindowOutputTargetXR*>(pActorPlugin->GetOutputTarget());
  pActor->AddPlugin(std::move(pActorPlugin));

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  m_hView = pView->GetHandle();

  pView->SetSwapChain(m_hSwapChain);

  pView->SetViewport(plRectFloat((float)m_Info.m_vEyeRenderTargetSize.width, (float)m_Info.m_vEyeRenderTargetSize.height));


  return std::move(pActor);
}

void plOpenXR::OnActorDestroyed()
{
  if (m_hView.IsInvalidated())
    return;

  m_pCompanion = nullptr;
  SetHMDCamera(nullptr);

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  plRenderWorld::RemoveMainView(m_hView);
  m_hView.Invalidate();

  // #TODO_XR DestroySwapChain will only queue the destruction. We either need to flush it somehow or postpone DeinitSession.
  plGALDevice::GetDefaultDevice()->DestroySwapChain(m_hSwapChain);
  m_hSwapChain.Invalidate();

  DeinitSession();
}

bool plOpenXR::SupportsCompanionView()
{
#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
  return true;
#else
  // E.g. on UWP OpenXR creates its own main window and other resources that conflict with our window.
  // Thus we must prevent the creation of a companion view or OpenXR crashes.
  return false;
#endif
}

XrSpace plOpenXR::GetBaseSpace() const
{
  return m_StageSpace == plXRStageSpace::Standing ? m_pSceneSpace : m_pLocalSpace;
}

XrResult plOpenXR::InitSystem()
{
  PL_ASSERT_DEV(m_SystemId == XR_NULL_SYSTEM_ID, "OpenXR actor already exists.");
  XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
  systemInfo.formFactor = XrFormFactor::XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
  XR_SUCCEED_OR_CLEANUP_LOG(xrGetSystem(m_pInstance, &systemInfo, &m_SystemId), DeinitSystem);

  XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
  XR_SUCCEED_OR_CLEANUP_LOG(xrGetSystemProperties(m_pInstance, m_SystemId, &systemProperties), DeinitSystem);
  m_Info.m_sDeviceName = systemProperties.systemName;

  return XrResult::XR_SUCCESS;
}

void plOpenXR::DeinitSystem()
{
  m_SystemId = XR_NULL_SYSTEM_ID;
}

XrResult plOpenXR::InitSession()
{
  PL_ASSERT_DEV(m_pSession == XR_NULL_HANDLE, "");

  plUInt32 count;
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateEnvironmentBlendModes(m_pInstance, m_SystemId, m_PrimaryViewConfigurationType, 0, &count, nullptr), DeinitSystem);

  plHybridArray<XrEnvironmentBlendMode, 4> environmentBlendModes;
  environmentBlendModes.SetCount(count);
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateEnvironmentBlendModes(m_pInstance, m_SystemId, m_PrimaryViewConfigurationType, count, &count, environmentBlendModes.GetData()), DeinitSession);

  // Select preferred blend mode.
  m_BlendMode = environmentBlendModes[0];

  XR_SUCCEED_OR_CLEANUP_LOG(InitGraphicsPlugin(), DeinitSession);
  XR_SUCCEED_OR_CLEANUP_LOG(InitDebugMessenger(), DeinitSession);

  XrSessionCreateInfo sessionCreateInfo{XR_TYPE_SESSION_CREATE_INFO};
  sessionCreateInfo.systemId = m_SystemId;
  sessionCreateInfo.next = &m_XrGraphicsBindingD3D11;

#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
  if (m_Extensions.m_bHolographicWindowAttachment)
  {
    // Creating a HolographicSpace before activating the CoreWindow to make it a holographic window
    winrt::Windows::UI::Core::CoreWindow window = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread();
    winrt::Windows::Graphics::Holographic::HolographicSpace holographicSpace = winrt::Windows::Graphics::Holographic::HolographicSpace::CreateForCoreWindow(window);
    window.Activate();

    XrHolographicWindowAttachmentMSFT holographicWindowAttachment{XR_TYPE_HOLOGRAPHIC_WINDOW_ATTACHMENT_MSFT};
    {
      holographicWindowAttachment.next = &m_XrGraphicsBindingD3D11;
      // TODO: The code in this block works around the fact that for some reason the commented out winrt equivalent does not compile although it works in every sample:
      // error C2131: expression did not evaluate to a constant.
      // holographicWindowAttachment.coreWindow = window.as<IUnknown>().get();
      // holographicWindowAttachment.holographicSpace = holographicSpace.as<IUnknown>().get();
      winrt::com_ptr<IUnknown> temp;
      winrt::copy_to_abi(window.as<winrt::Windows::Foundation::IUnknown>(), *temp.put_void());
      holographicWindowAttachment.coreWindow = temp.detach();

      winrt::copy_to_abi(holographicSpace.as<winrt::Windows::Foundation::IUnknown>(), *temp.put_void());
      holographicWindowAttachment.holographicSpace = temp.detach();
    }

    sessionCreateInfo.next = &holographicWindowAttachment;
  }
#endif

  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateSession(m_pInstance, &sessionCreateInfo, &m_pSession), DeinitSession);

  XrReferenceSpaceCreateInfo spaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
  if (m_Extensions.m_bUnboundedReferenceSpace)
  {
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT;
  }
  else
#endif
  {
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
  }

  spaceCreateInfo.poseInReferenceSpace = ConvertTransform(plTransform::MakeIdentity());
  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateReferenceSpace(m_pSession, &spaceCreateInfo, &m_pSceneSpace), DeinitSession);

  spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateReferenceSpace(m_pSession, &spaceCreateInfo, &m_pLocalSpace), DeinitSession);

  XR_SUCCEED_OR_CLEANUP_LOG(m_pInput->CreateActions(m_pSession, m_pSceneSpace), DeinitSession);
  XR_SUCCEED_OR_CLEANUP_LOG(m_pInput->AttachSessionActionSets(m_pSession), DeinitSession);

  m_GALdeviceEventsId = plGALDevice::s_Events.AddEventHandler(plMakeDelegate(&plOpenXR::GALDeviceEventHandler, this));

  SetStageSpace(plXRStageSpace::Standing);
  if (m_Extensions.m_bSpatialAnchor)
  {
    m_pAnchors = PL_DEFAULT_NEW(plOpenXRSpatialAnchors, this);
  }
  if (m_Extensions.m_bHandTracking && plOpenXRHandTracking::IsHandTrackingSupported(this))
  {
    m_pHandTracking = PL_DEFAULT_NEW(plOpenXRHandTracking, this);
  }
  return XrResult::XR_SUCCESS;
}

void plOpenXR::DeinitSession()
{
  m_pCompanion = nullptr;
  m_bSessionRunning = false;
  m_bExitRenderLoop = false;
  m_bRequestRestart = false;
  m_bRenderInProgress = false;
  m_SessionState = XR_SESSION_STATE_UNKNOWN;

  m_pHandTracking = nullptr;
  m_pAnchors = nullptr;
  if (m_GALdeviceEventsId != 0)
  {
    plGALDevice::s_Events.RemoveEventHandler(m_GALdeviceEventsId);
  }

  if (m_pSceneSpace)
  {
    xrDestroySpace(m_pSceneSpace);
    m_pSceneSpace = XR_NULL_HANDLE;
  }

  if (m_pLocalSpace)
  {
    xrDestroySpace(m_pLocalSpace);
    m_pLocalSpace = XR_NULL_HANDLE;
  }

  m_pInput->DestroyActions();

  if (m_pSession)
  {
    // #TODO_XR flush command queue
    xrDestroySession(m_pSession);
    m_pSession = XR_NULL_HANDLE;
  }

  DeinitGraphicsPlugin();
  DeinitInitDebugMessenger();
}

XrResult plOpenXR::InitGraphicsPlugin()
{
  PL_ASSERT_DEV(m_XrGraphicsBindingD3D11.device == nullptr, "");
  // Hard-coded to d3d
  XrGraphicsRequirementsD3D11KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
  XR_SUCCEED_OR_CLEANUP_LOG(m_Extensions.pfn_xrGetD3D11GraphicsRequirementsKHR(m_pInstance, m_SystemId, &graphicsRequirements), DeinitGraphicsPlugin);
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plGALDeviceDX11* pD3dDevice = static_cast<plGALDeviceDX11*>(pDevice);

  m_XrGraphicsBindingD3D11.device = pD3dDevice->GetDXDevice();

  return XrResult::XR_SUCCESS;
}

void plOpenXR::DeinitGraphicsPlugin()
{
  m_XrGraphicsBindingD3D11.device = nullptr;
}

XrResult plOpenXR::InitDebugMessenger()
{
  XrDebugUtilsMessengerCreateInfoEXT create_info{XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
  create_info.messageSeverities = XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                  XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                  XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.userCallback = xrDebugCallback;

  XR_SUCCEED_OR_CLEANUP_LOG(m_Extensions.pfn_xrCreateDebugUtilsMessengerEXT(m_pInstance, &create_info, &m_pDebugMessenger), DeinitInitDebugMessenger);

  return XrResult::XR_SUCCESS;
}

void plOpenXR::DeinitInitDebugMessenger()
{
  if (m_pDebugMessenger != XR_NULL_HANDLE)
  {
    XR_LOG_ERROR(m_Extensions.pfn_xrDestroyDebugUtilsMessengerEXT(m_pDebugMessenger));
    m_pDebugMessenger = XR_NULL_HANDLE;
  }
}

void plOpenXR::BeforeUpdatePlugins()
{
  PL_PROFILE_SCOPE("BeforeUpdatePlugins");
  // Make sure the main camera component is set to stereo mode.
  if (plWorld* pWorld = GetWorld())
  {
    PL_LOCK(pWorld->GetWriteMarker());
    auto* pCCM = pWorld->GetComponentManager<plCameraComponentManager>();
    if (pCCM)
    {
      if (plCameraComponent* pCameraComponent = pCCM->GetCameraByUsageHint(plCameraUsageHint::MainView))
      {
        pCameraComponent->SetCameraMode(plCameraMode::Stereo);
      }
    }
  }

  XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER, nullptr};

  while (xrPollEvent(m_pInstance, &event) == XR_SUCCESS)
  {
#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
    m_pRemoting->HandleEvent(event);
#endif
    switch (event.type)
    {
      case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
      {
        m_pInput->UpdateCurrentInteractionProfile();
      }
      break;
      case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
      {
        const XrEventDataSessionStateChanged& session_state_changed_event = *reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
        m_SessionState = session_state_changed_event.state;
        switch (m_SessionState)
        {
          case XR_SESSION_STATE_READY:
          {
            XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO};
            sessionBeginInfo.primaryViewConfigurationType = m_PrimaryViewConfigurationType;
            if (xrBeginSession(m_pSession, &sessionBeginInfo) == XR_SUCCESS)
            {
              m_bSessionRunning = true;
            }
            break;
          }
          case XR_SESSION_STATE_STOPPING:
          {
            m_bSessionRunning = false;
            if (xrEndSession(m_pSession) != XR_SUCCESS)
            {
              // TODO log
            }
            break;
          }
          case XR_SESSION_STATE_EXITING:
          {
            // Do not attempt to restart because user closed this session.
            m_bExitRenderLoop = true;
            m_bRequestRestart = false;
            break;
          }
          case XR_SESSION_STATE_LOSS_PENDING:
          {
            // Poll for a new systemId
            m_bExitRenderLoop = true;
            m_bRequestRestart = true;
            break;
          }
          default:
            break;
        }
      }
      break;
      case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
      {
        const XrEventDataInstanceLossPending& instance_loss_pending_event = *reinterpret_cast<XrEventDataInstanceLossPending*>(&event);
        m_bExitRenderLoop = true;
        m_bRequestRestart = false;
      }
      break;
      default:
        break;
    }
    event = {XR_TYPE_EVENT_DATA_BUFFER, nullptr};
  }

  if (m_bExitRenderLoop)
  {
    DeinitSession();
    DeinitSystem();
    if (m_bRequestRestart)
    {
      // Try to re-init session
      XrResult res = InitSystem();
      if (res != XR_SUCCESS)
      {
        return;
      }
      res = InitSession();
      if (res != XR_SUCCESS)
      {
        DeinitSystem();
        return;
      }
    }
  }
  // #TODO exit render loop and restart logic not fully implemented.
}

void plOpenXR::UpdatePoses()
{
  PL_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");

  PL_PROFILE_SCOPE("UpdatePoses");
  m_ViewState = XrViewState{XR_TYPE_VIEW_STATE};
  plUInt32 viewCapacityInput = 2;
  plUInt32 viewCountOutput;

  XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
  viewLocateInfo.viewConfigurationType = m_PrimaryViewConfigurationType;
  viewLocateInfo.displayTime = m_FrameState.predictedDisplayTime;
  viewLocateInfo.space = GetBaseSpace();
  m_Views[0].type = XR_TYPE_VIEW;
  m_Views[1].type = XR_TYPE_VIEW;
  XrFovf previousFov[2];
  previousFov[0] = m_Views[0].fov;
  previousFov[1] = m_Views[1].fov;

  XrResult res = xrLocateViews(m_pSession, &viewLocateInfo, &m_ViewState, viewCapacityInput, &viewCountOutput, m_Views);

  if (res == XR_SUCCESS)
  {
    m_pInput->m_DeviceState[0].m_bGripPoseIsValid = ((m_ViewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) && (m_ViewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT));
    m_pInput->m_DeviceState[0].m_bAimPoseIsValid = m_pInput->m_DeviceState[0].m_bGripPoseIsValid;
  }
  else
  {
    m_pInput->m_DeviceState[0].m_bGripPoseIsValid = false;
    m_pInput->m_DeviceState[0].m_bAimPoseIsValid = false;
  }

  // Needed as workaround for broken XR runtimes.
  auto FovIsNull = [](const XrFovf& fov) {
    return fov.angleLeft == 0.0f && fov.angleRight == 0.0f && fov.angleDown == 0.0f && fov.angleUp == 0.0f;
  };

  auto IdentityFov = [](XrFovf& fov) {
    fov.angleLeft = -plAngle::MakeFromDegree(45.0f).GetRadian();
    fov.angleRight = plAngle::MakeFromDegree(45.0f).GetRadian();
    fov.angleUp = plAngle::MakeFromDegree(45.0f).GetRadian();
    fov.angleDown = -plAngle::MakeFromDegree(45.0f).GetRadian();
  };

  if (FovIsNull(m_Views[0].fov) || FovIsNull(m_Views[1].fov))
  {
    IdentityFov(m_Views[0].fov);
    IdentityFov(m_Views[1].fov);
  }

  m_bProjectionChanged = plMemoryUtils::Compare(&previousFov[0], &m_Views[0].fov, 1) != 0 || plMemoryUtils::Compare(&previousFov[1], &m_Views[1].fov, 1) != 0;

  for (plUInt32 uiEyeIndex : {0, 1})
  {
    plQuat rot = ConvertOrientation(m_Views[uiEyeIndex].pose.orientation);
    if (!rot.IsValid())
    {
      m_Views[uiEyeIndex].pose.orientation = XrQuaternionf{0, 0, 0, 1};
    }
  }

  UpdateCamera();
  m_pInput->UpdateActions();
  if (m_pHandTracking)
  {
    m_pHandTracking->UpdateJointTransforms();
  }
}

void plOpenXR::UpdateCamera()
{
  if (!m_pCameraToSynchronize)
  {
    return;
  }
  // Update camera projection
  if (m_uiSettingsModificationCounter != m_pCameraToSynchronize->GetSettingsModificationCounter() || m_bProjectionChanged)
  {
    m_bProjectionChanged = false;
    const float fAspectRatio = (float)m_Info.m_vEyeRenderTargetSize.width / (float)m_Info.m_vEyeRenderTargetSize.height;
    auto CreateProjection = [](const XrView& view, plCamera* cam) {
      return plGraphicsUtils::CreatePerspectiveProjectionMatrix(plMath::Tan(plAngle::MakeFromRadian(view.fov.angleLeft)) * cam->GetNearPlane(), plMath::Tan(plAngle::MakeFromRadian(view.fov.angleRight)) * cam->GetNearPlane(), plMath::Tan(plAngle::MakeFromRadian(view.fov.angleDown)) * cam->GetNearPlane(),
        plMath::Tan(plAngle::MakeFromRadian(view.fov.angleUp)) * cam->GetNearPlane(), cam->GetNearPlane(), cam->GetFarPlane());
    };

    // Update projection with newest near/ far values. If not sync camera is set, just use the last value from XR
    // camera.
    const plMat4 projLeft = CreateProjection(m_Views[0], m_pCameraToSynchronize);
    const plMat4 projRight = CreateProjection(m_Views[1], m_pCameraToSynchronize);
    m_pCameraToSynchronize->SetStereoProjection(projLeft, projRight, fAspectRatio);
    m_uiSettingsModificationCounter = m_pCameraToSynchronize->GetSettingsModificationCounter();
  }

  // Update camera view
  {
    plTransform add;
    add.SetIdentity();
    plView* pView = nullptr;
    if (plRenderWorld::TryGetView(m_hView, pView))
    {
      if (const plWorld* pWorld = pView->GetWorld())
      {
        PL_LOCK(pWorld->GetReadMarker());
        if (const plStageSpaceComponentManager* pStageMan = pWorld->GetComponentManager<plStageSpaceComponentManager>())
        {
          if (const plStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
          {
            plEnum<plXRStageSpace> stageSpace = pStage->GetStageSpace();
            if (m_StageSpace != stageSpace)
              SetStageSpace(pStage->GetStageSpace());
            add = pStage->GetOwner()->GetGlobalTransform();
          }
        }
      }
    }

    if (m_pInput->m_DeviceState[0].m_bGripPoseIsValid)
    {
      // Update device state (average of both eyes).
      const plQuat rot = plQuat::MakeSlerp(ConvertOrientation(m_Views[0].pose.orientation), ConvertOrientation(m_Views[1].pose.orientation), 0.5f);
      const plVec3 pos = plMath::Lerp(ConvertPosition(m_Views[0].pose.position), ConvertPosition(m_Views[1].pose.position), 0.5f);

      m_pInput->m_DeviceState[0].m_vGripPosition = pos;
      m_pInput->m_DeviceState[0].m_qGripRotation = rot;
      m_pInput->m_DeviceState[0].m_vAimPosition = pos;
      m_pInput->m_DeviceState[0].m_qAimRotation = rot;
      m_pInput->m_DeviceState[0].m_Type = plXRDeviceType::HMD;
      m_pInput->m_DeviceState[0].m_bGripPoseIsValid = true;
      m_pInput->m_DeviceState[0].m_bAimPoseIsValid = true;
      m_pInput->m_DeviceState[0].m_bDeviceIsConnected = true;
    }

    // Set view matrix
    if (m_pInput->m_DeviceState[0].m_bGripPoseIsValid)
    {
      const plMat4 mStageTransform = add.GetAsMat4();
      const plMat4 poseLeft = mStageTransform * ConvertPoseToMatrix(m_Views[0].pose);
      const plMat4 poseRight = mStageTransform * ConvertPoseToMatrix(m_Views[1].pose);

      // PL Forward is +X, need to add this to align the forward projection
      const plMat4 viewMatrix = plGraphicsUtils::CreateLookAtViewMatrix(plVec3::MakeZero(), plVec3(1, 0, 0), plVec3(0, 0, 1));
      const plMat4 mViewTransformLeft = viewMatrix * poseLeft.GetInverse();
      const plMat4 mViewTransformRight = viewMatrix * poseRight.GetInverse();

      m_pCameraToSynchronize->SetViewMatrix(mViewTransformLeft, plCameraEye::Left);
      m_pCameraToSynchronize->SetViewMatrix(mViewTransformRight, plCameraEye::Right);
    }
  }
}

void plOpenXR::BeginFrame()
{
#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
  winrt::Windows::UI::Core::CoreWindow window = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread();
  window.Dispatcher().ProcessEvents(winrt::Windows::UI::Core::CoreProcessEventsOption::ProcessAllIfPresent);
#endif

  if (m_hView.IsInvalidated() || !m_bSessionRunning)
    return;

  PL_PROFILE_SCOPE("OpenXrBeginFrame");
  {
    PL_PROFILE_SCOPE("xrWaitFrame");
    m_FrameWaitInfo = XrFrameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
    m_FrameState = XrFrameState{XR_TYPE_FRAME_STATE};
    XrResult result = xrWaitFrame(m_pSession, &m_FrameWaitInfo, &m_FrameState);
    if (result != XR_SUCCESS)
    {
      m_bRenderInProgress = false;
      return;
    }
  }
  {
    PL_PROFILE_SCOPE("xrBeginFrame");
    m_FrameBeginInfo = XrFrameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    XrResult result = xrBeginFrame(m_pSession, &m_FrameBeginInfo);
    if (result != XR_SUCCESS)
    {
      m_bRenderInProgress = false;
      return;
    }
  }

  // #TODO_XR Swap chain acquire here?

  UpdatePoses();

  // This will update the extracted view from last frame with the new data we got
  // this frame just before starting to render.
  plView* pView = nullptr;
  if (plRenderWorld::TryGetView(m_hView, pView))
  {
    pView->UpdateViewData(plRenderWorld::GetDataIndexForRendering());
  }
  m_bRenderInProgress = true;
}

void plOpenXR::DelayPresent()
{
  PL_ASSERT_DEBUG(!m_bPresentDelayed, "Last present was not flushed");
  m_bPresentDelayed = true;
}

void plOpenXR::Present()
{
  if (!m_bPresentDelayed)
    return;

  m_bPresentDelayed = false;

  const plGALOpenXRSwapChain* pSwapChain = static_cast<const plGALOpenXRSwapChain*>(plGALDevice::GetDefaultDevice()->GetSwapChain(m_hSwapChain));
  if (!m_bRenderInProgress || !pSwapChain)
    return;

  if (m_pCompanion)
  {
    m_pCompanion->RenderCompanionView();
    pSwapChain->PresentRenderTarget();
  }
}

plGALTextureHandle plOpenXR::GetCurrentTexture()
{
  const plGALOpenXRSwapChain* pSwapChain = static_cast<const plGALOpenXRSwapChain*>(plGALDevice::GetDefaultDevice()->GetSwapChain(m_hSwapChain));
  if (!pSwapChain)
    return plGALTextureHandle();

  return pSwapChain->m_hColorRT;
}

void plOpenXR::EndFrame()
{
  const plGALOpenXRSwapChain* pSwapChain = static_cast<const plGALOpenXRSwapChain*>(plGALDevice::GetDefaultDevice()->GetSwapChain(m_hSwapChain));

  if (!m_bRenderInProgress || !pSwapChain)
    return;

  PL_PROFILE_SCOPE("OpenXrEndFrame");
  for (uint32_t i = 0; i < 2; i++)
  {
    m_ProjectionLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
    m_ProjectionLayerViews[i].pose = m_Views[i].pose;
    m_ProjectionLayerViews[i].fov = m_Views[i].fov;
    m_ProjectionLayerViews[i].subImage.swapchain = pSwapChain->GetColorSwapchain();
    m_ProjectionLayerViews[i].subImage.imageRect.offset = {0, 0};
    m_ProjectionLayerViews[i].subImage.imageRect.extent = {(plInt32)m_Info.m_vEyeRenderTargetSize.width, (plInt32)m_Info.m_vEyeRenderTargetSize.height};
    m_ProjectionLayerViews[i].subImage.imageArrayIndex = i;

    if (m_Extensions.m_bDepthComposition && m_pCameraToSynchronize)
    {
      m_DepthLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR};
      m_DepthLayerViews[i].minDepth = 0;
      m_DepthLayerViews[i].maxDepth = 1;
      m_DepthLayerViews[i].nearZ = m_pCameraToSynchronize->GetNearPlane();
      m_DepthLayerViews[i].farZ = m_pCameraToSynchronize->GetFarPlane();
      m_DepthLayerViews[i].subImage.swapchain = pSwapChain->GetDepthSwapchain();
      m_DepthLayerViews[i].subImage.imageRect.offset = {0, 0};
      m_DepthLayerViews[i].subImage.imageRect.extent = {(plInt32)m_Info.m_vEyeRenderTargetSize.width, (plInt32)m_Info.m_vEyeRenderTargetSize.height};
      m_DepthLayerViews[i].subImage.imageArrayIndex = i;

      m_ProjectionLayerViews[i].next = &m_DepthLayerViews[i];
    }
  }


  m_Layer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
  m_Layer.space = GetBaseSpace();
  m_Layer.viewCount = 2;
  m_Layer.views = m_ProjectionLayerViews;

  plHybridArray<XrCompositionLayerBaseHeader*, 1> layers;
  layers.PushBack(reinterpret_cast<XrCompositionLayerBaseHeader*>(&m_Layer));

  // Submit the composition layers for the predicted display time.
  XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
  frameEndInfo.displayTime = m_FrameState.predictedDisplayTime;
  frameEndInfo.environmentBlendMode = m_BlendMode;
  frameEndInfo.layerCount = layers.GetCapacity();
  frameEndInfo.layers = layers.GetData();

  PL_PROFILE_SCOPE("xrEndFrame");
  XR_LOG_ERROR(xrEndFrame(m_pSession, &frameEndInfo));
}

void plOpenXR::GALDeviceEventHandler(const plGALDeviceEvent& e)
{
  // Begin frame and end frame need to be encompassing all workload, XR and otherwise as xrWaitFrame will use this time interval to decide when to wake up the application.
  if (e.m_Type == plGALDeviceEvent::Type::BeforeBeginFrame)
  {
    BeginFrame();
  }
  else if (e.m_Type == plGALDeviceEvent::Type::BeforeEndFrame)
  {
    Present();
    EndFrame();
  }
}

void plOpenXR::GameApplicationEventHandler(const plGameApplicationExecutionEvent& e)
{
  PL_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");

  if (e.m_Type == plGameApplicationExecutionEvent::Type::BeforeUpdatePlugins)
  {
    BeforeUpdatePlugins();
  }
}

void plOpenXR::SetStageSpace(plXRStageSpace::Enum space)
{
  m_StageSpace = space;
}

void plOpenXR::SetHMDCamera(plCamera* pCamera)
{
  PL_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");

  if (m_pCameraToSynchronize == pCamera)
    return;

  m_pCameraToSynchronize = pCamera;
  if (m_pCameraToSynchronize)
  {
    m_uiSettingsModificationCounter = m_pCameraToSynchronize->GetSettingsModificationCounter() + 1;
    m_pCameraToSynchronize->SetCameraMode(plCameraMode::Stereo, m_pCameraToSynchronize->GetFovOrDim(), m_pCameraToSynchronize->GetNearPlane(), m_pCameraToSynchronize->GetFarPlane());
  }
}

plWorld* plOpenXR::GetWorld()
{
  plView* pView = nullptr;
  if (plRenderWorld::TryGetView(m_hView, pView))
  {
    return pView->GetWorld();
  }
  return nullptr;
}

XrPosef plOpenXR::ConvertTransform(const plTransform& tr)
{
  XrPosef pose;
  pose.orientation = ConvertOrientation(tr.m_qRotation);
  pose.position = ConvertPosition(tr.m_vPosition);
  return pose;
}

XrQuaternionf plOpenXR::ConvertOrientation(const plQuat& q)
{
  return {q.y, q.z, -q.x, -q.w};
}

XrVector3f plOpenXR::ConvertPosition(const plVec3& vPos)
{
  return {vPos.y, vPos.z, -vPos.x};
}

plQuat plOpenXR::ConvertOrientation(const XrQuaternionf& q)
{
  return {-q.z, q.x, q.y, -q.w};
}

plVec3 plOpenXR::ConvertPosition(const XrVector3f& pos)
{
  return {-pos.z, pos.x, pos.y};
}

plMat4 plOpenXR::ConvertPoseToMatrix(const XrPosef& pose)
{
  plMat4 m;
  plMat3 rot = ConvertOrientation(pose.orientation).GetAsMat3();
  plVec3 pos = ConvertPosition(pose.position);
  m.SetTransformationMatrix(rot, pos);
  return m;
}

plGALResourceFormat::Enum plOpenXR::ConvertTextureFormat(int64_t format)
{
  switch (format)
  {
    case DXGI_FORMAT_D32_FLOAT:
      return plGALResourceFormat::DFloat;
    case DXGI_FORMAT_D16_UNORM:
      return plGALResourceFormat::D16;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
      return plGALResourceFormat::D24S8;
    default:
      plImageFormat::Enum imageFormat = plImageFormatMappings::FromDxgiFormat(static_cast<plUInt32>(format));
      plGALResourceFormat::Enum galFormat = plTextureUtils::ImageFormatToGalFormat(imageFormat, false);
      return galFormat;
  }
}

PL_STATICLINK_FILE(OpenXRPlugin, OpenXRPlugin_OpenXRSingleton);
