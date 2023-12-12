#pragma once

#include <Core/World/World.h>
#include <RendererCore/Meshes/MeshComponent.h>

typedef plComponentManager<class plAtmosphericScatteringComponent, plBlockStorageType::Compact> plAtmosphericScatteringComponentManager;

class PLASMA_RENDERERCORE_DLL plAtmosphericScatteringComponent : public plRenderComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plAtmosphericScatteringComponent, plRenderComponent, plAtmosphericScatteringComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;


  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

public:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg) override;

  void SetPlanetRadius(float fPlanetRadius);
  float GetPlanetRadius() const;

  void SetAtmosphereRadius(float fAtmosphereRadius);
  float GetAtmosphereRadius() const;

  void SetRayleighScattering(plColor RayleighScattering);
  plColor GetRayleighScattering() const;

  void SetMieScattering(plColor MieScattering);
  plColor GetMieScattering() const;

  void SetAbsorption(plColor Absorption);
  plColor GetAbsorption() const;

  void SetAmbientScattering(plColor AmbientScattering);
  plColor GetAmbientScattering() const;

  void SetMieScatterDirection(float MieScatterDirection);
  float GetMieScatterDirection() const;

  void SetRayleighHeight(float fRayleighHeight);
  float GetRayleighHeight() const;

  void SetMieHeight(float fMieHeight);
  float GetMieHeight() const;

  void SetAbsorptionHeight(float fAbsorptionHeight);
  float GetAbsorptionHeight() const;

  void SetAbsorptionFalloff(float fAbsorptionFalloff);
  float GetAbsorptionFalloff() const;

  void SetRaySteps(float fRaySteps);
  float GetRaySteps() const;

  void SetLightSteps(float fLightSteps);
  float GetLightSteps() const;

  //////////////////////////////////////////////////////////////////////////
  // plAtmosphericScatteringComponent

public:
  plAtmosphericScatteringComponent();
  ~plAtmosphericScatteringComponent();

private:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;
  void UpdateMaterials() const;

  plMeshResourceHandle m_hMesh;
  plMaterialResourceHandle m_hMaterial;

  float m_fPlanetRadius;
  float m_fAtmosphereRadius;
  plColor m_RayleighScattering;
  plColor m_MieScattering;
  plColor m_Absorption;
  plColor m_AmbientScattering;
  float m_fMieScatterDirection;
  float m_fRayleighHeight;
  float m_fMieHeight;
  float m_fAbsorptionHeight;
  float m_fAbsorptionFalloff;
  float m_fRaySteps;
  float m_fLightSteps;
};