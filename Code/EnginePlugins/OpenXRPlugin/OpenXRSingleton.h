#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/XR/XRInputDevice.h>
#include <GameEngine/XR/XRInterface.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class plOpenXRInputDevice;
class plOpenXRSpatialAnchors;
class plOpenXRHandTracking;
class plWindowOutputTargetXR;
struct plGameApplicationExecutionEvent;

PLASMA_DEFINE_AS_POD_TYPE(XrViewConfigurationView);
PLASMA_DEFINE_AS_POD_TYPE(XrEnvironmentBlendMode);

class PLASMA_OPENXRPLUGIN_DLL plOpenXR : public plXRInterface
{
  PLASMA_DECLARE_SINGLETON_OF_INTERFACE(plOpenXR, plXRInterface);

public:
  plOpenXR();
  ~plOpenXR();

  XrInstance GetInstance() const { return m_instance; }
  uint64_t GetSystemId() const { return m_systemId; }
  XrSession GetSession() const { return m_session; }
  XrViewConfigurationType GetViewType() const { return m_primaryViewConfigurationType; }
  bool GetDepthComposition() const;

  virtual bool IsHmdPresent() const override;

  virtual plResult Initialize() override;
  virtual void Deinitialize() override;
  virtual bool IsInitialized() const override;

  virtual const plHMDInfo& GetHmdInfo() const override;
  virtual plXRInputDevice& GetXRInput() const override;

  virtual plGALTextureHandle GetCurrentTexture() override;

  void DelayPresent();
  void Present();
  void EndFrame();

  virtual plUniquePtr<plActor> CreateActor(plView* pView, plGALMSAASampleCount::Enum msaaCount = plGALMSAASampleCount::None,
    plUniquePtr<plWindowBase> companionWindow = nullptr, plUniquePtr<plWindowOutputTargetGAL> companionWindowOutput = nullptr) override;
  virtual void OnActorDestroyed() override;
  virtual bool SupportsCompanionView() override;

  XrSpace GetBaseSpace() const;

private:
  XrResult SelectExtensions(plHybridArray<const char*, 6>& extensions);
  XrResult SelectLayers(plHybridArray<const char*, 6>& layers);
  XrResult InitSystem();
  void DeinitSystem();
  XrResult InitSession();
  void DeinitSession();
  XrResult InitGraphicsPlugin();
  void DeinitGraphicsPlugin();
  XrResult InitDebugMessenger();
  void DeinitInitDebugMessenger();

  void GameApplicationEventHandler(const plGameApplicationExecutionEvent& e);
  void GALDeviceEventHandler(const plGALDeviceEvent& e);

  void BeforeUpdatePlugins();
  void UpdatePoses();
  void UpdateCamera();
  void BeginFrame();

  void SetStageSpace(plXRStageSpace::Enum space);
  void SetHMDCamera(plCamera* pCamera);

  plWorld* GetWorld();

public:
  static XrPosef ConvertTransform(const plTransform& tr);
  static XrQuaternionf ConvertOrientation(const plQuat& q);
  static XrVector3f ConvertPosition(const plVec3& pos);
  static plQuat ConvertOrientation(const XrQuaternionf& q);
  static plVec3 ConvertPosition(const XrVector3f& pos);
  static plMat4 ConvertPoseToMatrix(const XrPosef& pose);
  static plGALResourceFormat::Enum ConvertTextureFormat(int64_t format);

private:
  friend class plOpenXRInputDevice;
  friend class plOpenXRSpatialAnchors;
  friend class plOpenXRHandTracking;
  friend class plOpenXRRemoting;
  friend class plGALOpenXRSwapChain;

  struct Extensions
  {
    bool m_bValidation = false;
    bool m_bDebugUtils = false;
    PFN_xrCreateDebugUtilsMessengerEXT pfn_xrCreateDebugUtilsMessengerEXT;
    PFN_xrDestroyDebugUtilsMessengerEXT pfn_xrDestroyDebugUtilsMessengerEXT;

    bool m_bD3D11 = false;
    PFN_xrGetD3D11GraphicsRequirementsKHR pfn_xrGetD3D11GraphicsRequirementsKHR;

    bool m_bDepthComposition = false;

    bool m_bUnboundedReferenceSpace = false;

    bool m_bSpatialAnchor = false;
    PFN_xrCreateSpatialAnchorMSFT pfn_xrCreateSpatialAnchorMSFT;
    PFN_xrCreateSpatialAnchorSpaceMSFT pfn_xrCreateSpatialAnchorSpaceMSFT;
    PFN_xrDestroySpatialAnchorMSFT pfn_xrDestroySpatialAnchorMSFT;

    bool m_bHandInteraction = false;

    bool m_bHandTracking = false;
    PFN_xrCreateHandTrackerEXT pfn_xrCreateHandTrackerEXT;
    PFN_xrDestroyHandTrackerEXT pfn_xrDestroyHandTrackerEXT;
    PFN_xrLocateHandJointsEXT pfn_xrLocateHandJointsEXT;

    bool m_bHandTrackingMesh = false;
    PFN_xrCreateHandMeshSpaceMSFT pfn_xrCreateHandMeshSpaceMSFT;
    PFN_xrUpdateHandMeshMSFT pfn_xrUpdateHandMeshMSFT;

    bool m_bHolographicWindowAttachment = false;

    bool m_bRemoting = false;
#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
    PFN_xrRemotingSetContextPropertiesMSFT pfn_xrRemotingSetContextPropertiesMSFT;
    PFN_xrRemotingConnectMSFT pfn_xrRemotingConnectMSFT;
    PFN_xrRemotingDisconnectMSFT pfn_xrRemotingDisconnectMSFT;
    PFN_xrRemotingGetConnectionStateMSFT pfn_xrRemotingGetConnectionStateMSFT;
#endif
  };

  // Instance
  XrInstance m_instance = XR_NULL_HANDLE;
  Extensions m_extensions;
#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
  plUniquePtr<class plOpenXRRemoting> m_remoting;
#endif

  // System
  uint64_t m_systemId = XR_NULL_SYSTEM_ID;

  // Session
  XrSession m_session = XR_NULL_HANDLE;
  XrSpace m_sceneSpace = XR_NULL_HANDLE;
  XrSpace m_localSpace = XR_NULL_HANDLE;
  plEventSubscriptionID m_executionEventsId = 0;
  plEventSubscriptionID m_beginRenderEventsId = 0;
  plEventSubscriptionID m_GALdeviceEventsId = 0;
  XrDebugUtilsMessengerEXT m_DebugMessenger = XR_NULL_HANDLE;

  // Graphics plugin
  XrEnvironmentBlendMode m_blendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  XrGraphicsBindingD3D11KHR m_xrGraphicsBindingD3D11{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
  XrFormFactor m_formFactor{XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY};
  XrViewConfigurationType m_primaryViewConfigurationType{XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};

  plGALSwapChainHandle m_hSwapChain;

  // Views
  XrViewState m_viewState{XR_TYPE_VIEW_STATE};
  XrView m_views[2];
  bool m_projectionChanged = true;
  XrCompositionLayerProjection m_layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
  XrCompositionLayerProjectionView m_projectionLayerViews[2];
  XrCompositionLayerDepthInfoKHR m_depthLayerViews[2];

  // State
  bool m_sessionRunning = false;
  bool m_exitRenderLoop = false;
  bool m_requestRestart = false;
  bool m_renderInProgress = false;
  XrSessionState m_sessionState{XR_SESSION_STATE_UNKNOWN};

  XrFrameWaitInfo m_frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
  XrFrameState m_frameState{XR_TYPE_FRAME_STATE};
  XrFrameBeginInfo m_frameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};

  // XR interface state
  plHMDInfo m_Info;
  mutable plUniquePtr<plOpenXRInputDevice> m_Input;
  plUniquePtr<plOpenXRSpatialAnchors> m_Anchors;
  plUniquePtr<plOpenXRHandTracking> m_HandTracking;

  plCamera* m_pCameraToSynchronize = nullptr;
  plEnum<plXRStageSpace> m_StageSpace;
  plUInt32 m_uiSettingsModificationCounter = 0;
  plViewHandle m_hView;

  plWindowOutputTargetXR* m_pCompanion = nullptr;
  bool m_bPresentDelayed = false;
};
