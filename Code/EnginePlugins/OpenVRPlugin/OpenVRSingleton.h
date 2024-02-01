#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/Interfaces/VRInterface.h>
#include <OpenVRPlugin/Basics.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

struct plGameApplicationExecutionEvent;
typedef plTypedResourceHandle<class plShaderResource> plShaderResourceHandle;

class PL_OPENVRPLUGIN_DLL plOpenVR : public plVRInterface
{
  PL_DECLARE_SINGLETON_OF_INTERFACE(plOpenVR, plVRInterface);

public:
  plOpenVR();

  virtual bool IsHmdPresent() const override;

  virtual bool Initialize() override;
  virtual void Deinitialize() override;
  virtual bool IsInitialized() const override;

  virtual const plHMDInfo& GetHmdInfo() const override;
  virtual void GetDeviceList(plHybridArray<plVRDeviceID, 64>& out_Devices) const override;
  virtual plVRDeviceID GetDeviceIDByType(plVRDeviceType::Enum type) const override;
  virtual const plVRDeviceState& GetDeviceState(plVRDeviceID uiDeviceID) const override;
  virtual plEvent<const plVRDeviceEvent&>& DeviceEvents() override;

  virtual plViewHandle CreateVRView(
    const plRenderPipelineResourceHandle& hRenderPipeline, plCamera* pCamera, plGALMSAASampleCount::Enum msaaCount) override;
  virtual plViewHandle GetVRView() const override;
  virtual bool DestroyVRView() override;
  virtual bool SupportsCompanionView() override;
  virtual bool SetCompanionViewRenderTarget(plGALTextureHandle hRenderTarget) override;
  virtual plGALTextureHandle GetCompanionViewRenderTarget() const override;

private:
  void GameApplicationEventHandler(const plGameApplicationExecutionEvent& e);
  void GALDeviceEventHandler(const plGALDeviceEvent& e);
  void OnBeginRender(plUInt64);

  void ReadHMDInfo();
  void OnDeviceActivated(plVRDeviceID uiDeviceID);
  void OnDeviceDeactivated(plVRDeviceID uiDeviceID);

  void UpdatePoses();
  void UpdateHands();
  void SetStageSpace(plVRStageSpace::Enum space);
  void SetHMDCamera(plCamera* pCamera);
  void UpdateCamera();

  plMat4 GetHMDProjectionEye(vr::Hmd_Eye nEye, float fNear, float fFar) const;
  plMat4 GetHMDEyePose(vr::Hmd_Eye nEye) const;
  plString GetTrackedDeviceString(
    vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = nullptr) const;

  static plMat4 ConvertSteamVRMatrix(const vr::HmdMatrix34_t& matPose);
  static plVec3 ConvertSteamVRVector(const vr::HmdVector3_t& vector);

private:
  bool m_bInitialized = false;

  vr::IVRSystem* m_pHMD = nullptr;
  vr::IVRRenderModels* m_pRenderModels = nullptr;

  plHMDInfo m_Info;
  plVRDeviceState m_DeviceState[vr::k_unMaxTrackedDeviceCount];
  plInt8 m_iLeftControllerDeviceID = -1;
  plInt8 m_iRightControllerDeviceID = -1;
  plEvent<const plVRDeviceEvent&> m_DeviceEvents;

  plWorld* m_pWorld = nullptr;
  plCamera* m_pCameraToSynchronize = nullptr;
  plEnum<plVRStageSpace> m_StageSpace;

  plCamera m_VRCamera;
  plUInt32 m_uiSettingsModificationCounter = 0;
  plViewHandle m_hView;
  plGALRenderTagetSetup m_RenderTargetSetup;
  plGALTextureCreationDescription m_eyeDesc;
  plGALTextureHandle m_hColorRT;
  plGALTextureHandle m_hDepthRT;

  plGALTextureHandle m_hCompanionRenderTarget;
  plConstantBufferStorageHandle m_hCompanionConstantBuffer;
  plShaderResourceHandle m_hCompanionShader;
};

PL_DYNAMIC_PLUGIN_DECLARATION(PL_OPENVRPLUGIN_DLL, plOpenVRPlugin);
