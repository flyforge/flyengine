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

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <RendererDX11/Device/DeviceDX11.h>
#endif

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#  include <winrt/Windows.Graphics.Holographic.h>
#  include <winrt/Windows.UI.Core.h>
#  include <winrt/base.h>
#endif

#include <vector>

PLASMA_CHECK_AT_COMPILETIME(plGALMSAASampleCount::None == 1);
PLASMA_CHECK_AT_COMPILETIME(plGALMSAASampleCount::TwoSamples == 2);
PLASMA_CHECK_AT_COMPILETIME(plGALMSAASampleCount::FourSamples == 4);
PLASMA_CHECK_AT_COMPILETIME(plGALMSAASampleCount::EightSamples == 8);

PLASMA_IMPLEMENT_SINGLETON(plOpenXR);

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
  m_remoting = PLASMA_DEFAULT_NEW(plOpenXRRemoting, this);
#endif
}

plOpenXR::~plOpenXR() {}

bool plOpenXR::GetDepthComposition() const
{
  return m_extensions.m_bDepthComposition;
}

bool plOpenXR::IsHmdPresent() const
{
  XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
  systemInfo.formFactor = XrFormFactor::XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
  uint64_t systemId = XR_NULL_SYSTEM_ID;
  XrResult res = xrGetSystem(m_instance, &systemInfo, &systemId);

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
  auto AddExtIfSupported = [&](const char* extensionName, bool& enableFlag) -> XrResult
  {
    auto it = std::find_if(begin(extensionProperties), end(extensionProperties), [&](const XrExtensionProperties& prop)
      { return plStringUtils::IsEqual(prop.extensionName, extensionName); });
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
  XR_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(XR_KHR_D3D11_ENABLE_EXTENSION_NAME, m_extensions.m_bD3D11));

  AddExtIfSupported(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME, m_extensions.m_bDepthComposition);
  AddExtIfSupported(XR_MSFT_UNBOUNDED_REFERENCE_SPACE_EXTENSION_NAME, m_extensions.m_bUnboundedReferenceSpace);
  AddExtIfSupported(XR_MSFT_SPATIAL_ANCHOR_EXTENSION_NAME, m_extensions.m_bSpatialAnchor);
  AddExtIfSupported(XR_EXT_HAND_TRACKING_EXTENSION_NAME, m_extensions.m_bHandTracking);
  AddExtIfSupported(XR_MSFT_HAND_INTERACTION_EXTENSION_NAME, m_extensions.m_bHandInteraction);
  AddExtIfSupported(XR_MSFT_HAND_TRACKING_MESH_EXTENSION_NAME, m_extensions.m_bHandTrackingMesh);

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  AddExtIfSupported(XR_EXT_DEBUG_UTILS_EXTENSION_NAME, m_extensions.m_bDebugUtils);
#endif

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
  AddExtIfSupported(XR_MSFT_HOLOGRAPHIC_WINDOW_ATTACHMENT_EXTENSION_NAME, m_extensions.m_bHolographicWindowAttachment);
#endif

#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
  AddExtIfSupported(XR_MSFT_HOLOGRAPHIC_REMOTING_EXTENSION_NAME, m_extensions.m_bRemoting);
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
  auto AddExtIfSupported = [&](const char* layerName, bool& enableFlag) -> XrResult
  {
    auto it = std::find_if(begin(layerProperties), end(layerProperties), [&](const XrApiLayerProperties& prop)
      { return plStringUtils::IsEqual(prop.layerName, layerName); });
    if (it != end(layerProperties))
    {
      layers.PushBack(layerName);
      enableFlag = true;
      return XR_SUCCESS;
    }
    enableFlag = false;
    return XR_ERROR_EXTENSION_NOT_PRESENT;
  };

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  AddExtIfSupported("XR_APILAYER_LUNARG_core_validation", m_extensions.m_bValidation);
#endif

  return XR_SUCCESS;
}

#define PLASMA_GET_INSTANCE_PROC_ADDR(name) (void)xrGetInstanceProcAddr(m_instance, #name, reinterpret_cast<PFN_xrVoidFunction*>(&m_extensions.pfn_##name));

plResult plOpenXR::Initialize()
{
  if (m_instance != XR_NULL_HANDLE)
    return PLASMA_SUCCESS;

  // Build out the extensions to enable. Some extensions are required and some are optional.
  plHybridArray<const char*, 6> enabledExtensions;
  if (SelectExtensions(enabledExtensions) != XR_SUCCESS)
    return PLASMA_FAILURE;

  plHybridArray<const char*, 6> enabledLayers;
  if (SelectLayers(enabledLayers) != XR_SUCCESS)
    return PLASMA_FAILURE;

  // Create the instance with desired extensions.
  XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
  createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.GetCount();
  createInfo.enabledExtensionNames = enabledExtensions.GetData();
  createInfo.enabledApiLayerCount = (uint32_t)enabledLayers.GetCount();
  createInfo.enabledApiLayerNames = enabledLayers.GetData();

  plStringUtils::Copy(createInfo.applicationInfo.applicationName, PLASMA_ARRAY_SIZE(createInfo.applicationInfo.applicationName), plApplication::GetApplicationInstance()->GetApplicationName());
  plStringUtils::Copy(createInfo.applicationInfo.engineName, PLASMA_ARRAY_SIZE(createInfo.applicationInfo.engineName), "plEngine");
  createInfo.applicationInfo.engineVersion = 1;
  createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
  createInfo.applicationInfo.applicationVersion = 1;
  XrResult res = xrCreateInstance(&createInfo, &m_instance);
  if (res != XR_SUCCESS)
  {
    plLog::Error("InitSystem xrCreateInstance failed: {}", res);
    Deinitialize();
    return PLASMA_FAILURE;
  }
  XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
  res = xrGetInstanceProperties(m_instance, &instanceProperties);
  if (res != XR_SUCCESS)
  {
    plLog::Error("InitSystem xrGetInstanceProperties failed: {}", res);
    Deinitialize();
    return PLASMA_FAILURE;
  }

  plStringBuilder sTemp;
  m_Info.m_sDeviceDriver = plConversionUtils::ToString(instanceProperties.runtimeVersion, sTemp);

  PLASMA_GET_INSTANCE_PROC_ADDR(xrGetD3D11GraphicsRequirementsKHR);

  if (m_extensions.m_bSpatialAnchor)
  {
    PLASMA_GET_INSTANCE_PROC_ADDR(xrCreateSpatialAnchorMSFT);
    PLASMA_GET_INSTANCE_PROC_ADDR(xrCreateSpatialAnchorSpaceMSFT);
    PLASMA_GET_INSTANCE_PROC_ADDR(xrDestroySpatialAnchorMSFT);
  }

  if (m_extensions.m_bHandTracking)
  {
    PLASMA_GET_INSTANCE_PROC_ADDR(xrCreateHandTrackerEXT);
    PLASMA_GET_INSTANCE_PROC_ADDR(xrDestroyHandTrackerEXT);
    PLASMA_GET_INSTANCE_PROC_ADDR(xrLocateHandJointsEXT);
  }

  if (m_extensions.m_bHandTrackingMesh)
  {
    PLASMA_GET_INSTANCE_PROC_ADDR(xrCreateHandMeshSpaceMSFT);
    PLASMA_GET_INSTANCE_PROC_ADDR(xrUpdateHandMeshMSFT);
  }

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  if (m_extensions.m_bDebugUtils)
  {
    PLASMA_GET_INSTANCE_PROC_ADDR(xrCreateDebugUtilsMessengerEXT);
    PLASMA_GET_INSTANCE_PROC_ADDR(xrDestroyDebugUtilsMessengerEXT);
  }
#endif

#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
  if (m_extensions.m_bRemoting)
  {
    PLASMA_GET_INSTANCE_PROC_ADDR(xrRemotingSetContextPropertiesMSFT);
    PLASMA_GET_INSTANCE_PROC_ADDR(xrRemotingConnectMSFT);
    PLASMA_GET_INSTANCE_PROC_ADDR(xrRemotingDisconnectMSFT);
    PLASMA_GET_INSTANCE_PROC_ADDR(xrRemotingGetConnectionStateMSFT);
  }
#endif

  m_Input = PLASMA_DEFAULT_NEW(plOpenXRInputDevice, this);

  m_executionEventsId = plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(plMakeDelegate(&plOpenXR::GameApplicationEventHandler, this));

  res = InitSystem();
  if (res != XR_SUCCESS)
  {
    plLog::Error("InitSystem failed: {}", res);
    Deinitialize();
    return PLASMA_FAILURE;
  }

  plLog::Success("OpenXR {0} v{1} initialized successfully.", instanceProperties.runtimeName, instanceProperties.runtimeVersion);
  return PLASMA_SUCCESS;
}

void plOpenXR::Deinitialize()
{
#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
  m_remoting->Disconnect().IgnoreResult();
#endif

  if (m_executionEventsId != 0)
  {
    plGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(m_executionEventsId);
  }

  plGALDevice::GetDefaultDevice()->DestroySwapChain(m_hSwapChain);
  m_hSwapChain.Invalidate();

  DeinitSession();
  DeinitSystem();

  m_Input = nullptr;

  if (m_instance)
  {
    xrDestroyInstance(m_instance);
    m_instance = XR_NULL_HANDLE;
  }
}

bool plOpenXR::IsInitialized() const
{
  return m_instance != XR_NULL_HANDLE;
}

const plHMDInfo& plOpenXR::GetHmdInfo() const
{
  PLASMA_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");
  return m_Info;
}

plXRInputDevice& plOpenXR::GetXRInput() const
{
  return *(m_Input.Borrow());
}

plUniquePtr<plActor> plOpenXR::CreateActor(plView* pView, plGALMSAASampleCount::Enum msaaCount, plUniquePtr<plWindowBase> companionWindow, plUniquePtr<plWindowOutputTargetGAL> companionWindowOutput)
{
  PLASMA_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");

  XrResult res = InitSession();
  if (res != XrResult::XR_SUCCESS)
  {
    plLog::Error("InitSession failed: {}", res);
    return {};
  }

  plGALXRSwapChain::SetFactoryMethod([this, msaaCount](plXRInterface* pXrInterface) -> plGALSwapChainHandle
    { return plGALDevice::GetDefaultDevice()->CreateSwapChain([this, pXrInterface, msaaCount](plAllocatorBase* pAllocator) -> plGALSwapChain*
        { return PLASMA_NEW(pAllocator, plGALOpenXRSwapChain, this, msaaCount); }); });
  PLASMA_SCOPE_EXIT(plGALXRSwapChain::SetFactoryMethod({}););

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
    PLASMA_ASSERT_DEV(pView->GetCamera() != nullptr, "The provided view requires a camera to be set.");
    SetHMDCamera(pView->GetCamera());
  }

  plUniquePtr<plActor> pActor = PLASMA_DEFAULT_NEW(plActor, "OpenXR", this);

  PLASMA_ASSERT_DEV((companionWindow != nullptr) == (companionWindowOutput != nullptr), "Both companionWindow and companionWindowOutput must either be null or valid.");
  PLASMA_ASSERT_DEV(companionWindow == nullptr || SupportsCompanionView(), "If a companionWindow is set, SupportsCompanionView() must be true.");

  plUniquePtr<plActorPluginWindowXR> pActorPlugin = PLASMA_DEFAULT_NEW(plActorPluginWindowXR, this, std::move(companionWindow), std::move(companionWindowOutput));
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
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
  return true;
#else
  // E.g. on UWP OpenXR creates its own main window and other resources that conflict with our window.
  // Thus we must prevent the creation of a companion view or OpenXR crashes.
  return false;
#endif
}

XrSpace plOpenXR::GetBaseSpace() const
{
  return m_StageSpace == plXRStageSpace::Standing ? m_sceneSpace : m_localSpace;
}

XrResult plOpenXR::InitSystem()
{
  PLASMA_ASSERT_DEV(m_systemId == XR_NULL_SYSTEM_ID, "OpenXR actor already exists.");
  XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
  systemInfo.formFactor = XrFormFactor::XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
  XR_SUCCEED_OR_CLEANUP_LOG(xrGetSystem(m_instance, &systemInfo, &m_systemId), DeinitSystem);

  XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
  XR_SUCCEED_OR_CLEANUP_LOG(xrGetSystemProperties(m_instance, m_systemId, &systemProperties), DeinitSystem);
  m_Info.m_sDeviceName = systemProperties.systemName;

  return XrResult::XR_SUCCESS;
}

void plOpenXR::DeinitSystem()
{
  m_systemId = XR_NULL_SYSTEM_ID;
}

XrResult plOpenXR::InitSession()
{
  PLASMA_ASSERT_DEV(m_session == XR_NULL_HANDLE, "");

  plUInt32 count;
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateEnvironmentBlendModes(m_instance, m_systemId, m_primaryViewConfigurationType, 0, &count, nullptr), DeinitSystem);

  plHybridArray<XrEnvironmentBlendMode, 4> environmentBlendModes;
  environmentBlendModes.SetCount(count);
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateEnvironmentBlendModes(m_instance, m_systemId, m_primaryViewConfigurationType, count, &count, environmentBlendModes.GetData()), DeinitSession);

  // Select preferred blend mode.
  m_blendMode = environmentBlendModes[0];

  XR_SUCCEED_OR_CLEANUP_LOG(InitGraphicsPlugin(), DeinitSession);
  XR_SUCCEED_OR_CLEANUP_LOG(InitDebugMessenger(), DeinitSession);

  XrSessionCreateInfo sessionCreateInfo{XR_TYPE_SESSION_CREATE_INFO};
  sessionCreateInfo.systemId = m_systemId;
  sessionCreateInfo.next = &m_xrGraphicsBindingD3D11;

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
  if (m_extensions.m_bHolographicWindowAttachment)
  {
    // Creating a HolographicSpace before activating the CoreWindow to make it a holographic window
    winrt::Windows::UI::Core::CoreWindow window = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread();
    winrt::Windows::Graphics::Holographic::HolographicSpace holographicSpace = winrt::Windows::Graphics::Holographic::HolographicSpace::CreateForCoreWindow(window);
    window.Activate();

    XrHolographicWindowAttachmentMSFT holographicWindowAttachment{XR_TYPE_HOLOGRAPHIC_WINDOW_ATTACHMENT_MSFT};
    {
      holographicWindowAttachment.next = &m_xrGraphicsBindingD3D11;
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

  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateSession(m_instance, &sessionCreateInfo, &m_session), DeinitSession);

  XrReferenceSpaceCreateInfo spaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
  if (m_extensions.m_bUnboundedReferenceSpace)
  {
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT;
  }
  else
#endif
  {
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
  }

  spaceCreateInfo.poseInReferenceSpace = ConvertTransform(plTransform::MakeIdentity());
  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateReferenceSpace(m_session, &spaceCreateInfo, &m_sceneSpace), DeinitSession);

  spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateReferenceSpace(m_session, &spaceCreateInfo, &m_localSpace), DeinitSession);

  XR_SUCCEED_OR_CLEANUP_LOG(m_Input->CreateActions(m_session, m_sceneSpace), DeinitSession);
  XR_SUCCEED_OR_CLEANUP_LOG(m_Input->AttachSessionActionSets(m_session), DeinitSession);

  m_GALdeviceEventsId = plGALDevice::GetDefaultDevice()->m_Events.AddEventHandler(plMakeDelegate(&plOpenXR::GALDeviceEventHandler, this));

  SetStageSpace(plXRStageSpace::Standing);
  if (m_extensions.m_bSpatialAnchor)
  {
    m_Anchors = PLASMA_DEFAULT_NEW(plOpenXRSpatialAnchors, this);
  }
  if (m_extensions.m_bHandTracking && plOpenXRHandTracking::IsHandTrackingSupported(this))
  {
    m_HandTracking = PLASMA_DEFAULT_NEW(plOpenXRHandTracking, this);
  }
  return XrResult::XR_SUCCESS;
}

void plOpenXR::DeinitSession()
{
  m_pCompanion = nullptr;
  m_sessionRunning = false;
  m_exitRenderLoop = false;
  m_requestRestart = false;
  m_renderInProgress = false;
  m_sessionState = XR_SESSION_STATE_UNKNOWN;

  m_HandTracking = nullptr;
  m_Anchors = nullptr;
  if (m_GALdeviceEventsId != 0)
  {
    plGALDevice::GetDefaultDevice()->m_Events.RemoveEventHandler(m_GALdeviceEventsId);
  }

  if (m_sceneSpace)
  {
    xrDestroySpace(m_sceneSpace);
    m_sceneSpace = XR_NULL_HANDLE;
  }

  if (m_localSpace)
  {
    xrDestroySpace(m_localSpace);
    m_localSpace = XR_NULL_HANDLE;
  }

  m_Input->DestroyActions();

  if (m_session)
  {
    // #TODO_XR flush command queue
    xrDestroySession(m_session);
    m_session = XR_NULL_HANDLE;
  }

  DeinitGraphicsPlugin();
  DeinitInitDebugMessenger();
}

XrResult plOpenXR::InitGraphicsPlugin()
{
  PLASMA_ASSERT_DEV(m_xrGraphicsBindingD3D11.device == nullptr, "");
  // Hard-coded to d3d
  XrGraphicsRequirementsD3D11KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
  XR_SUCCEED_OR_CLEANUP_LOG(m_extensions.pfn_xrGetD3D11GraphicsRequirementsKHR(m_instance, m_systemId, &graphicsRequirements), DeinitGraphicsPlugin);
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plGALDeviceDX11* pD3dDevice = static_cast<plGALDeviceDX11*>(pDevice);

  m_xrGraphicsBindingD3D11.device = pD3dDevice->GetDXDevice();

  return XrResult::XR_SUCCESS;
}

void plOpenXR::DeinitGraphicsPlugin()
{
  m_xrGraphicsBindingD3D11.device = nullptr;
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

  XR_SUCCEED_OR_CLEANUP_LOG(m_extensions.pfn_xrCreateDebugUtilsMessengerEXT(m_instance, &create_info, &m_DebugMessenger), DeinitInitDebugMessenger);

  return XrResult::XR_SUCCESS;
}

void plOpenXR::DeinitInitDebugMessenger()
{
  if (m_DebugMessenger != XR_NULL_HANDLE)
  {
    XR_LOG_ERROR(m_extensions.pfn_xrDestroyDebugUtilsMessengerEXT(m_DebugMessenger));
    m_DebugMessenger = XR_NULL_HANDLE;
  }
}

void plOpenXR::BeforeUpdatePlugins()
{
  PLASMA_PROFILE_SCOPE("BeforeUpdatePlugins");
  // Make sure the main camera component is set to stereo mode.
  if (plWorld* pWorld = GetWorld())
  {
    PLASMA_LOCK(pWorld->GetWriteMarker());
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

  while (xrPollEvent(m_instance, &event) == XR_SUCCESS)
  {
#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
    m_remoting->HandleEvent(event);
#endif
    switch (event.type)
    {
      case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
      {
        m_Input->UpdateCurrentInteractionProfile();
      }
      break;
      case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
      {
        const XrEventDataSessionStateChanged& session_state_changed_event = *reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
        m_sessionState = session_state_changed_event.state;
        switch (m_sessionState)
        {
          case XR_SESSION_STATE_READY:
          {
            XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO};
            sessionBeginInfo.primaryViewConfigurationType = m_primaryViewConfigurationType;
            if (xrBeginSession(m_session, &sessionBeginInfo) == XR_SUCCESS)
            {
              m_sessionRunning = true;
            }
            break;
          }
          case XR_SESSION_STATE_STOPPING:
          {
            m_sessionRunning = false;
            if (xrEndSession(m_session) != XR_SUCCESS)
            {
              // TODO log
            }
            break;
          }
          case XR_SESSION_STATE_EXITING:
          {
            // Do not attempt to restart because user closed this session.
            m_exitRenderLoop = true;
            m_requestRestart = false;
            break;
          }
          case XR_SESSION_STATE_LOSS_PENDING:
          {
            // Poll for a new systemId
            m_exitRenderLoop = true;
            m_requestRestart = true;
            break;
          }
        }
      }
      break;
      case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
      {
        const XrEventDataInstanceLossPending& instance_loss_pending_event = *reinterpret_cast<XrEventDataInstanceLossPending*>(&event);
        m_exitRenderLoop = true;
        m_requestRestart = false;
      }
      break;
    }
    event = {XR_TYPE_EVENT_DATA_BUFFER, nullptr};
  }

  if (m_exitRenderLoop)
  {
    DeinitSession();
    DeinitSystem();
    if (m_requestRestart)
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
  PLASMA_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");

  PLASMA_PROFILE_SCOPE("UpdatePoses");
  m_viewState = XrViewState{XR_TYPE_VIEW_STATE};
  plUInt32 viewCapacityInput = 2;
  plUInt32 viewCountOutput;

  XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
  viewLocateInfo.viewConfigurationType = m_primaryViewConfigurationType;
  viewLocateInfo.displayTime = m_frameState.predictedDisplayTime;
  viewLocateInfo.space = GetBaseSpace();
  m_views[0].type = XR_TYPE_VIEW;
  m_views[1].type = XR_TYPE_VIEW;
  XrFovf previousFov[2];
  previousFov[0] = m_views[0].fov;
  previousFov[1] = m_views[1].fov;

  XrResult res = xrLocateViews(m_session, &viewLocateInfo, &m_viewState, viewCapacityInput, &viewCountOutput, m_views);

  if (res == XR_SUCCESS)
  {
    m_Input->m_DeviceState[0].m_bGripPoseIsValid = ((m_viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) && (m_viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT));
    m_Input->m_DeviceState[0].m_bAimPoseIsValid = m_Input->m_DeviceState[0].m_bGripPoseIsValid;
  }
  else
  {
    m_Input->m_DeviceState[0].m_bGripPoseIsValid = false;
    m_Input->m_DeviceState[0].m_bAimPoseIsValid = false;
  }

  // Needed as workaround for broken XR runtimes.
  auto FovIsNull = [](const XrFovf& fov)
  {
    return fov.angleLeft == 0.0f && fov.angleRight == 0.0f && fov.angleDown == 0.0f && fov.angleUp == 0.0f;
  };

  auto IdentityFov = [](XrFovf& fov)
  {
    fov.angleLeft = -plAngle::MakeFromDegree(45.0f).GetRadian();
    fov.angleRight = plAngle::MakeFromDegree(45.0f).GetRadian();
    fov.angleUp = plAngle::MakeFromDegree(45.0f).GetRadian();
    fov.angleDown = -plAngle::MakeFromDegree(45.0f).GetRadian();
  };

  if (FovIsNull(m_views[0].fov) || FovIsNull(m_views[1].fov))
  {
    IdentityFov(m_views[0].fov);
    IdentityFov(m_views[1].fov);
  }

  m_projectionChanged = plMemoryUtils::Compare(&previousFov[0], &m_views[0].fov, 1) != 0 || plMemoryUtils::Compare(&previousFov[1], &m_views[1].fov, 1) != 0;

  for (plUInt32 uiEyeIndex : {0, 1})
  {
    plQuat rot = ConvertOrientation(m_views[uiEyeIndex].pose.orientation);
    if (!rot.IsValid())
    {
      m_views[uiEyeIndex].pose.orientation = XrQuaternionf{0, 0, 0, 1};
    }

  }

  UpdateCamera();
  m_Input->UpdateActions();
  if (m_HandTracking)
  {
    m_HandTracking->UpdateJointTransforms();
  }
}

void plOpenXR::UpdateCamera()
{
  if (!m_pCameraToSynchronize)
  {
    return;
  }
  // Update camera projection
  if (m_uiSettingsModificationCounter != m_pCameraToSynchronize->GetSettingsModificationCounter() || m_projectionChanged)
  {
    m_projectionChanged = false;
    const float fAspectRatio = (float)m_Info.m_vEyeRenderTargetSize.width / (float)m_Info.m_vEyeRenderTargetSize.height;
    auto CreateProjection = [](const XrView& view, plCamera* cam)
    {
      return plGraphicsUtils::CreatePerspectiveProjectionMatrix(plMath::Tan(plAngle::MakeFromRadian(view.fov.angleLeft)) * cam->GetNearPlane(), plMath::Tan(plAngle::MakeFromRadian(view.fov.angleRight)) * cam->GetNearPlane(), plMath::Tan(plAngle::MakeFromRadian(view.fov.angleDown)) * cam->GetNearPlane(),
        plMath::Tan(plAngle::MakeFromRadian(view.fov.angleUp)) * cam->GetNearPlane(), cam->GetNearPlane(), cam->GetFarPlane());
    };

    // Update projection with newest near/ far values. If not sync camera is set, just use the last value from XR
    // camera.
    const plMat4 projLeft = CreateProjection(m_views[0], m_pCameraToSynchronize);
    const plMat4 projRight = CreateProjection(m_views[1], m_pCameraToSynchronize);
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
        PLASMA_LOCK(pWorld->GetReadMarker());
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

    if (m_Input->m_DeviceState[0].m_bGripPoseIsValid)
    {
      // Update device state (average of both eyes).
      plQuat rot;
      rot.SetIdentity();
      rot = plQuat::MakeSlerp(ConvertOrientation(m_views[0].pose.orientation), ConvertOrientation(m_views[1].pose.orientation), 0.5f);
      const plVec3 pos = plMath::Lerp(ConvertPosition(m_views[0].pose.position), ConvertPosition(m_views[1].pose.position), 0.5f);

      m_Input->m_DeviceState[0].m_vGripPosition = pos;
      m_Input->m_DeviceState[0].m_qGripRotation = rot;
      m_Input->m_DeviceState[0].m_vAimPosition = pos;
      m_Input->m_DeviceState[0].m_qAimRotation = rot;
      m_Input->m_DeviceState[0].m_Type = plXRDeviceType::HMD;
      m_Input->m_DeviceState[0].m_bGripPoseIsValid = true;
      m_Input->m_DeviceState[0].m_bAimPoseIsValid = true;
      m_Input->m_DeviceState[0].m_bDeviceIsConnected = true;
    }

    // Set view matrix
    if (m_Input->m_DeviceState[0].m_bGripPoseIsValid)
    {
      const plMat4 mStageTransform = add.GetAsMat4();
      const plMat4 poseLeft = mStageTransform * ConvertPoseToMatrix(m_views[0].pose);
      const plMat4 poseRight = mStageTransform * ConvertPoseToMatrix(m_views[1].pose);

      // PLASMA Forward is +X, need to add this to align the forward projection
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
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
  winrt::Windows::UI::Core::CoreWindow window = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread();
  window.Dispatcher().ProcessEvents(winrt::Windows::UI::Core::CoreProcessEventsOption::ProcessAllIfPresent);
#endif

  if (m_hView.IsInvalidated() || !m_sessionRunning)
    return;

  PLASMA_PROFILE_SCOPE("OpenXrBeginFrame");
  {
    PLASMA_PROFILE_SCOPE("xrWaitFrame");
    m_frameWaitInfo = XrFrameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
    m_frameState = XrFrameState{XR_TYPE_FRAME_STATE};
    XrResult result = xrWaitFrame(m_session, &m_frameWaitInfo, &m_frameState);
    if (result != XR_SUCCESS)
    {
      m_renderInProgress = false;
      return;
    }
  }
  {
    PLASMA_PROFILE_SCOPE("xrBeginFrame");
    m_frameBeginInfo = XrFrameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    XrResult result = xrBeginFrame(m_session, &m_frameBeginInfo);
    if (result != XR_SUCCESS)
    {
      m_renderInProgress = false;
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
  m_renderInProgress = true;
}

void plOpenXR::DelayPresent()
{
  PLASMA_ASSERT_DEBUG(!m_bPresentDelayed, "Last present was not flushed");
  m_bPresentDelayed = true;
}

void plOpenXR::Present()
{
  if (!m_bPresentDelayed)
    return;

  m_bPresentDelayed = false;

  const plGALOpenXRSwapChain* pSwapChain = static_cast<const plGALOpenXRSwapChain*>(plGALDevice::GetDefaultDevice()->GetSwapChain(m_hSwapChain));
  if (!m_renderInProgress || !pSwapChain)
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

  if (!m_renderInProgress || !pSwapChain)
    return;

  PLASMA_PROFILE_SCOPE("OpenXrEndFrame");
  for (uint32_t i = 0; i < 2; i++)
  {
    m_projectionLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
    m_projectionLayerViews[i].pose = m_views[i].pose;
    m_projectionLayerViews[i].fov = m_views[i].fov;
    m_projectionLayerViews[i].subImage.swapchain = pSwapChain->GetColorSwapchain();
    m_projectionLayerViews[i].subImage.imageRect.offset = {0, 0};
    m_projectionLayerViews[i].subImage.imageRect.extent = {(plInt32)m_Info.m_vEyeRenderTargetSize.width, (plInt32)m_Info.m_vEyeRenderTargetSize.height};
    m_projectionLayerViews[i].subImage.imageArrayIndex = i;

    if (m_extensions.m_bDepthComposition && m_pCameraToSynchronize)
    {
      m_depthLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR};
      m_depthLayerViews[i].minDepth = 0;
      m_depthLayerViews[i].maxDepth = 1;
      m_depthLayerViews[i].nearZ = m_pCameraToSynchronize->GetNearPlane();
      m_depthLayerViews[i].farZ = m_pCameraToSynchronize->GetFarPlane();
      m_depthLayerViews[i].subImage.swapchain = pSwapChain->GetDepthSwapchain();
      m_depthLayerViews[i].subImage.imageRect.offset = {0, 0};
      m_depthLayerViews[i].subImage.imageRect.extent = {(plInt32)m_Info.m_vEyeRenderTargetSize.width, (plInt32)m_Info.m_vEyeRenderTargetSize.height};
      m_depthLayerViews[i].subImage.imageArrayIndex = i;

      m_projectionLayerViews[i].next = &m_depthLayerViews[i];
    }
  }


  m_layer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
  m_layer.space = GetBaseSpace();
  m_layer.viewCount = 2;
  m_layer.views = m_projectionLayerViews;

  plHybridArray<XrCompositionLayerBaseHeader*, 1> layers;
  layers.PushBack(reinterpret_cast<XrCompositionLayerBaseHeader*>(&m_layer));

  // Submit the composition layers for the predicted display time.
  XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
  frameEndInfo.displayTime = m_frameState.predictedDisplayTime;
  frameEndInfo.environmentBlendMode = m_blendMode;
  frameEndInfo.layerCount = layers.GetCapacity();
  frameEndInfo.layers = layers.GetData();

  PLASMA_PROFILE_SCOPE("xrEndFrame");
  XR_LOG_ERROR(xrEndFrame(m_session, &frameEndInfo));
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
  PLASMA_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");

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
  PLASMA_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");

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

XrVector3f plOpenXR::ConvertPosition(const plVec3& pos)
{
  return {pos.y, pos.z, -pos.x};
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

PLASMA_STATICLINK_FILE(OpenXRPlugin, OpenXRPlugin_OpenXRSingleton);
