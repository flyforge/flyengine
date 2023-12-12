#pragma once

#include <Core/World/SettingsComponent.h>
#include <Core/World/SettingsComponentManager.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Textures/TextureCubeResource.h>

struct plMsgUpdateLocalBounds;
struct plMsgExtractRenderData;
struct plMsgTransformChanged;

typedef plSettingsComponentManager<class plSkyLightComponent> plSkyLightComponentManager;

class PLASMA_RENDERERCORE_DLL plSkyLightComponent : public plSettingsComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plSkyLightComponent, plSettingsComponent, plSkyLightComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // plSkyLightComponent

public:
  plSkyLightComponent();
  ~plSkyLightComponent();

  void SetReflectionProbeMode(plEnum<plReflectionProbeMode> mode); // [ property ]
  plEnum<plReflectionProbeMode> GetReflectionProbeMode() const;    // [ property ]

  void SetIntensity(float fIntensity); // [ property ]
  float GetIntensity() const;          // [ property ]

  void SetSaturation(float fSaturation); // [ property ]
  float GetSaturation() const;           // [ property ]

  const plTagSet& GetIncludeTags() const;   // [ property ]
  void InsertIncludeTag(const char* szTag); // [ property ]
  void RemoveIncludeTag(const char* szTag); // [ property ]

  const plTagSet& GetExcludeTags() const;   // [ property ]
  void InsertExcludeTag(const char* szTag); // [ property ]
  void RemoveExcludeTag(const char* szTag); // [ property ]

  void SetShowDebugInfo(bool bShowDebugInfo); // [ property ]
  bool GetShowDebugInfo() const;              // [ property ]

  void SetShowMipMaps(bool bShowMipMaps); // [ property ]
  bool GetShowMipMaps() const;            // [ property ]

  void SetCubeMapFile(const char* szFile); // [ property ]
  const char* GetCubeMapFile() const;      // [ property ]
  plTextureCubeResourceHandle GetCubeMap() const
  {
    return m_hCubeMap;
  }

  float GetNearPlane() const { return m_Desc.m_fNearPlane; } // [ property ]
  void SetNearPlane(float fNearPlane);                       // [ property ]

  float GetFarPlane() const { return m_Desc.m_fFarPlane; } // [ property ]
  void SetFarPlane(float fFarPlane);                       // [ property ]

protected:
  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg);
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;
  void OnTransformChanged(plMsgTransformChanged& msg);

  plReflectionProbeDesc m_Desc;
  plTextureCubeResourceHandle m_hCubeMap;

  plReflectionProbeId m_Id;

  mutable bool m_bStatesDirty = true;
};
