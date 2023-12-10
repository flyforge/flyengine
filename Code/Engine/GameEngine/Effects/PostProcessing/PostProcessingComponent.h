#pragma once

#include <Core/World/World.h>
#include <GameEngine/Volumes/VolumeSampler.h>
#include <RendererCore/Pipeline/Declarations.h>

class PLASMA_GAMEENGINE_DLL plPostProcessingComponentManager : public plComponentManager<class plPostProcessingComponent, plBlockStorageType::Compact>
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

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GAMEENGINE_DLL, plPostProcessingValueMapping);

class PLASMA_GAMEENGINE_DLL plPostProcessingComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plPostProcessingComponent, plComponent, plPostProcessingComponentManager);

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

private:
  plUInt32 Mappings_GetCount() const { return m_Mappings.GetCount(); }                                // [ property ]
  const plPostProcessingValueMapping& Mappings_GetMapping(plUInt32 i) const { return m_Mappings[i]; } // [ property ]
  void Mappings_SetMapping(plUInt32 i, const plPostProcessingValueMapping& mapping);                  // [ property ]
  void Mappings_Insert(plUInt32 uiIndex, const plPostProcessingValueMapping& mapping);                // [ property ]
  void Mappings_Remove(plUInt32 uiIndex);                                                             // [ property ]

  void RegisterSamplerValues();
  void SampleAndSetViewProperties();

  plComponentHandle m_hCameraComponent;
  plDynamicArray<plPostProcessingValueMapping> m_Mappings;
  plUniquePtr<plVolumeSampler> m_pSampler;
};
