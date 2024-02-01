#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GameEngine/XR/Declarations.h>
#include <GameEngine/XR/XRInputDevice.h>
#include <GameEngine/XR/XRInterface.h>
#include <RendererCore/Pipeline/Declarations.h>

struct plGALDeviceEvent;
struct plGameApplicationExecutionEvent;
class plWindowOutputTargetXR;

class PL_GAMEENGINE_DLL plDummyXRInput : public plXRInputDevice
{

public:
  void GetDeviceList(plHybridArray<plXRDeviceID, 64>& out_devices) const override;
  plXRDeviceID GetDeviceIDByType(plXRDeviceType::Enum type) const override;
  const plXRDeviceState& GetDeviceState(plXRDeviceID deviceID) const override;
  plString GetDeviceName(plXRDeviceID deviceID) const override;
  plBitflags<plXRDeviceFeatures> GetDeviceFeatures(plXRDeviceID deviceID) const override;

protected:
  void InitializeDevice() override;
  void UpdateInputSlotValues() override;
  void RegisterInputSlots() override;

protected:
  friend class plDummyXR;

  plXRDeviceState m_DeviceState[1];
};

class PL_GAMEENGINE_DLL plDummyXR : public plXRInterface
{
  PL_DECLARE_SINGLETON_OF_INTERFACE(plDummyXR, plXRInterface);

public:
  plDummyXR();
  ~plDummyXR() = default;

  bool IsHmdPresent() const override;
  plResult Initialize() override;
  void Deinitialize() override;
  bool IsInitialized() const override;
  const plHMDInfo& GetHmdInfo() const override;
  plXRInputDevice& GetXRInput() const override;
  bool SupportsCompanionView() override;
  plUniquePtr<plActor> CreateActor(plView* pView, plGALMSAASampleCount::Enum msaaCount = plGALMSAASampleCount::None, plUniquePtr<plWindowBase> pCompanionWindow = nullptr, plUniquePtr<plWindowOutputTargetGAL> pCompanionWindowOutput = nullptr) override;
  plGALTextureHandle GetCurrentTexture() override;
  void OnActorDestroyed() override;
  void GALDeviceEventHandler(const plGALDeviceEvent& e);
  void GameApplicationEventHandler(const plGameApplicationExecutionEvent& e);

protected:
  float m_fHeadHeight = 1.7f;
  float m_fEyeOffset = 0.05f;


  plHMDInfo m_Info;
  mutable plDummyXRInput m_Input;
  bool m_bInitialized = false;

  plEventSubscriptionID m_GALdeviceEventsId = 0;
  plEventSubscriptionID m_ExecutionEventsId = 0;

  plWorld* m_pWorld = nullptr;
  plCamera* m_pCameraToSynchronize = nullptr;
  plEnum<plXRStageSpace> m_StageSpace = plXRStageSpace::Seated;

  plViewHandle m_hView;
  plGALTextureHandle m_hColorRT;
  plGALTextureHandle m_hDepthRT;

  plWindowOutputTargetXR* m_pCompanion = nullptr;
};
