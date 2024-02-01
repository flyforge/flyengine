#pragma once

#include <Core/Graphics/Camera.h>
#include <Core/World/World.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/Declarations.h>

class plView;
struct plResourceEvent;

class PL_RENDERERCORE_DLL plCameraComponentManager : public plComponentManager<class plCameraComponent, plBlockStorageType::Compact>
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

/// \brief Adds a camera to the scene.
///
/// Cameras have different use cases which are selected through the plCameraUsageHint property.
/// A game needs (exactly) one camera with the usage hint "MainView", since that is what the renderer uses to render the output.
/// Other cameras are optional or for specialized use cases.
///
/// The camera component defines the field-of-view, near and far clipping plane distances,
/// which render pipeline to use, which objects to include and exclude in the rendered image and various other options.
///
/// A camera object may be created and controlled through a player prefab, for example in a first person or third person game.
/// It may also be created by an plGameState and controlled by its game logic, for example in top-down games that don't
/// really have a player object.
///
/// Ultimately camera components don't have functionality, they mostly exist and store some data.
/// It is the game state's decision how the game camera works. By default, the game state iterates over all camera components
/// and picks the best one (usually the "MainView") to place the renderer camera.
class PL_RENDERERCORE_DLL plCameraComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plCameraComponent, plComponent, plCameraComponentManager);

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

  /// \brief Sets what the camera should be used for.
  void SetUsageHint(plEnum<plCameraUsageHint> val);                      // [ property ]
  plEnum<plCameraUsageHint> GetUsageHint() const { return m_UsageHint; } // [ property ]

  /// \brief Sets the asset name (or path) to a render target resource, in case this camera should render to texture.
  void SetRenderTargetFile(const char* szFile); // [ property ]
  const char* GetRenderTargetFile() const;      // [ property ]

  /// \brief An offset to render only to a part of a texture.
  void SetRenderTargetRectOffset(plVec2 value);                                  // [ property ]
  plVec2 GetRenderTargetRectOffset() const { return m_vRenderTargetRectOffset; } // [ property ]

  /// \brief A size to render only to a part of a texture.
  void SetRenderTargetRectSize(plVec2 value);                                // [ property ]
  plVec2 GetRenderTargetRectSize() const { return m_vRenderTargetRectSize; } // [ property ]

  /// \brief Specifies whether the camera should be perspective or orthogonal and how to use the aspect ratio.
  void SetCameraMode(plEnum<plCameraMode> val);                 // [ property ]
  plEnum<plCameraMode> GetCameraMode() const { return m_Mode; } // [ property ]

  /// \brief Configures the distance of the near plane. Objects in front of the near plane get culled and clipped.
  void SetNearPlane(float fVal);                      // [ property ]
  float GetNearPlane() const { return m_fNearPlane; } // [ property ]

  /// \brief Configures the distance of the far plane. Objects behin the far plane get culled and clipped.
  void SetFarPlane(float fVal);                     // [ property ]
  float GetFarPlane() const { return m_fFarPlane; } // [ property ]

  /// \brief Sets the opening angle of the perspective view frustum. Whether this means the horizontal or vertical angle is determined by the camera mode.
  void SetFieldOfView(float fVal);                                   // [ property ]
  float GetFieldOfView() const { return m_fPerspectiveFieldOfView; } // [ property ]

  /// \brief Sets the size of the orthogonal view frustum. Whether this means the horizontal or vertical size is determined by the camera mode.
  void SetOrthoDimension(float fVal);                           // [ property ]
  float GetOrthoDimension() const { return m_fOrthoDimension; } // [ property ]

  /// \brief Returns the handle to the render pipeline that is in use.
  plRenderPipelineResourceHandle GetRenderPipeline() const;

  /// \brief Returns a handle to the view that the camera renders to.
  plViewHandle GetRenderTargetView() const;

  /// \brief Sets the name of the render pipeline to use.
  void SetRenderPipelineEnum(const char* szFile); // [ property ]
  const char* GetRenderPipelineEnum() const;      // [ property ]

  void SetAperture(float fAperture);                // [ property ]
  float GetAperture() const { return m_fAperture; } // [ property ]

  void SetShutterTime(plTime shutterTime);                // [ property ]
  plTime GetShutterTime() const { return m_ShutterTime; } // [ property ]

  void SetISO(float fISO);                // [ property ]
  float GetISO() const { return m_fISO; } // [ property ]

  void SetExposureCompensation(float fEC);                                  // [ property ]
  float GetExposureCompensation() const { return m_fExposureCompensation; } // [ property ]

  float GetEV100() const;    // [ property ]
  float GetExposure() const; // [ property ]

  /// \brief If non-empty, only objects with these tags will be included in this camera's output.
  plTagSet m_IncludeTags; // [ property ]

  /// \brief If non-empty, objects with these tags will be excluded from this camera's output.
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
  plTime m_ShutterTime = plTime::MakeFromSeconds(1.0f);
  float m_fISO = 100.0f;
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
