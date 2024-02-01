#pragma once

#include <Core/World/World.h>
#include <GameEngine/Volumes/VolumeSampler.h>
#include <RendererCore/Pipeline/Declarations.h>

class PL_GAMEENGINE_DLL plPostProcessingComponentManager : public plComponentManager<class plPostProcessingComponent, plBlockStorageType::Compact>
{
public:
  plPostProcessingComponentManager(plWorld* pWorld);

  virtual void Initialize() override;

private:
  void UpdateComponents(const UpdateContext& context);
};

struct plPostProcessingValueMapping
{
  plHashedString m_sRenderPassName;
  plHashedString m_sPropertyName;
  plHashedString m_sVolumeValueName;
  plVariant m_DefaultValue;
  plTime m_InterpolationDuration;

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);
};

PL_DECLARE_REFLECTABLE_TYPE(PL_GAMEENGINE_DLL, plPostProcessingValueMapping);

/// \brief A component that sets the configured values on a render pipeline and optionally samples those values from volumes at the corresponding camera position.
///
/// If there is a render target camera component attached to the owner object it will affect the render pipeline of this camera,
/// otherwise the render pipeline of the main camera is affected.
class PL_GAMEENGINE_DLL plPostProcessingComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plPostProcessingComponent, plComponent, plPostProcessingComponentManager);

public:
  plPostProcessingComponent();
  plPostProcessingComponent(plPostProcessingComponent&& other);
  ~plPostProcessingComponent();
  plPostProcessingComponent& operator=(plPostProcessingComponent&& other);

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  void SetVolumeType(const char* szType); // [ property ]
  const char* GetVolumeType() const;      // [ property ]

private:
  plUInt32 Mappings_GetCount() const { return m_Mappings.GetCount(); }                                // [ property ]
  const plPostProcessingValueMapping& Mappings_GetMapping(plUInt32 i) const { return m_Mappings[i]; } // [ property ]
  void Mappings_SetMapping(plUInt32 i, const plPostProcessingValueMapping& mapping);                  // [ property ]
  void Mappings_Insert(plUInt32 uiIndex, const plPostProcessingValueMapping& mapping);                // [ property ]
  void Mappings_Remove(plUInt32 uiIndex);                                                             // [ property ]

  plView* FindView() const;
  void RegisterSamplerValues();
  void ResetViewProperties();
  void SampleAndSetViewProperties();

  plComponentHandle m_hCameraComponent;
  plDynamicArray<plPostProcessingValueMapping> m_Mappings;
  plUniquePtr<plVolumeSampler> m_pSampler;
  plSpatialData::Category m_SpatialCategory = plInvalidSpatialDataCategory;
};
