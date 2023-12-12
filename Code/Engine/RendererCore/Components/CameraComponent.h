#pragma once

#include <Core/Graphics/Camera.h>
#include <Core/World/World.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/Declarations.h>

class plView;
struct plResourceEvent;

class PLASMA_RENDERERCORE_DLL plCameraComponentManager : public plComponentManager<class plCameraComponent, plBlockStorageType::Compact>
{
public:
  plCameraComponentManager(plWorld* pWorld);
  ~plCameraComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  void Update(const plWorldModule::UpdateContext& context);

  void ReinitializeAllRenderTargetCameras();

  const plCameraComponent* GetCameraByUsageHint(plCameraUsageHint::Enum usageHint) const;
  plCameraComponent* GetCameraByUsageHint(plCameraUsageHint::Enum usageHint);

private:
  friend class plCameraComponent;

  void AddRenderTargetCamera(plCameraComponent* pComponent);
  void RemoveRenderTargetCamera(plCameraComponent* pComponent);

  void OnViewCreated(plView* pView);
  void OnCameraConfigsChanged(void* dummy);

  plDynamicArray<plComponentHandle> m_ModifiedCameras;
  plDynamicArray<plComponentHandle> m_RenderTargetCameras;
};


class PLASMA_RENDERERCORE_DLL plCameraComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plCameraComponent, plComponent, plCameraComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plCameraComponent

public:
  plCameraComponent();
  ~plCameraComponent();

  plEnum<plCameraUsageHint> GetUsageHint() const { return m_UsageHint; } // [ property ]
  void SetUsageHint(plEnum<plCameraUsageHint> val);                      // [ property ]

  void SetRenderTargetFile(const char* szFile); // [ property ]
  const char* GetRenderTargetFile() const;      // [ property ]

  void SetRenderTargetRectOffset(plVec2 value);                                 // [ property ]
  plVec2 GetRenderTargetRectOffset() const { return m_vRenderTargetRectOffset; } // [ property ]

  void SetRenderTargetRectSize(plVec2 value);                               // [ property ]
  plVec2 GetRenderTargetRectSize() const { return m_vRenderTargetRectSize; } // [ property ]

  plEnum<plCameraMode> GetCameraMode() const { return m_Mode; } // [ property ]
  void SetCameraMode(plEnum<plCameraMode> val);                 // [ property ]

  float GetNearPlane() const { return m_fNearPlane; } // [ property ]
  void SetNearPlane(float fVal);                      // [ property ]

  float GetFarPlane() const { return m_fFarPlane; } // [ property ]
  void SetFarPlane(float fVal);                     // [ property ]

  float GetFieldOfView() const { return m_fPerspectiveFieldOfView; } // [ property ]
  void SetFieldOfView(float fVal);                                   // [ property ]

  float GetOrthoDimension() const { return m_fOrthoDimension; } // [ property ]
  void SetOrthoDimension(float fVal);                           // [ property ]

  plRenderPipelineResourceHandle GetRenderPipeline() const;
  plViewHandle GetRenderTargetView() const;

  const char* GetRenderPipelineEnum() const;      // [ property ]
  void SetRenderPipelineEnum(const char* szFile); // [ property ]

  float GetAperture() const { return m_fAperture; } // [ property ]
  void SetAperture(float fAperture);                // [ property ]

  plTime GetShutterTime() const { return m_ShutterTime; } // [ property ]
  void SetShutterTime(plTime shutterTime);                // [ property ]

  float GetISO() const { return m_fISO; } // [ property ]
  void SetISO(float fISO);                // [ property ]

  float GetFocusDistance() const { return m_fFocusDistance; } // [ property ]
  void SetFocusDistance(float fFocusDistance);                // [ property ]

  float GetExposureCompensation() const { return m_fExposureCompensation; } // [ property ]
  void SetExposureCompensation(float fEC);                                  // [ property ]

  float GetEV100() const;    // [ property ]
  float GetExposure() const; // [ property ]

  plTagSet m_IncludeTags; // [ property ]
  plTagSet m_ExcludeTags; // [ property ]

  void ApplySettingsToView(plView* pView) const;

private:
  void UpdateRenderTargetCamera();
  void ShowStats(plView* pView);

  void ResourceChangeEventHandler(const plResourceEvent& e);

  plEnum<plCameraUsageHint> m_UsageHint;
  plEnum<plCameraMode> m_Mode;
  plRenderToTexture2DResourceHandle m_hRenderTarget;
  float m_fNearPlane = 0.25f;
  float m_fFarPlane = 1000.0f;
  float m_fPerspectiveFieldOfView = 60.0f;
  float m_fOrthoDimension = 10.0f;
  plRenderPipelineResourceHandle m_hCachedRenderPipeline;

  float m_fAperture = 1.0f;
  plTime m_ShutterTime = plTime::Seconds(1.0f);
  float m_fISO = 100.0f;
  float m_fFocusDistance = 100.0f;
  float m_fExposureCompensation = 0.0f;

  void MarkAsModified();
  void MarkAsModified(plCameraComponentManager* pCameraManager);

  bool m_bIsModified = false;
  bool m_bShowStats = false;
  bool m_bRenderTargetInitialized = false;

  // -1 for none, 0 to 9 for ALT+Number
  plInt8 m_iEditorShortcut = -1; // [ property ]

  void ActivateRenderToTexture();
  void DeactivateRenderToTexture();

  plViewHandle m_hRenderTargetView;
  plVec2 m_vRenderTargetRectOffset = plVec2(0.0f);
  plVec2 m_vRenderTargetRectSize = plVec2(1.0f);
  plCamera m_RenderTargetCamera;
  plHashedString m_sRenderPipeline;
};
