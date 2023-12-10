#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

struct plMsgUpdateLocalBounds;

using plBlackboardTemplateResourceHandle = plTypedResourceHandle<class plBlackboardTemplateResource>;

class PLASMA_GAMEENGINE_DLL plVolumeComponent : public plComponent
{
  PLASMA_DECLARE_ABSTRACT_COMPONENT_TYPE(plVolumeComponent, plComponent);

public:
  plVolumeComponent();
  ~plVolumeComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetTemplateFile(const char* szFile); // [ property ]
  const char* GetTemplateFile() const;      // [ property ]

  void SetTemplate(const plBlackboardTemplateResourceHandle& hResource);
  plBlackboardTemplateResourceHandle GetTemplate() const { return m_hTemplateResource; }

  void SetSortOrder(float fOrder);// [ property ]
  float GetSortOrder() const { return m_fSortOrder; }// [ property ]

  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  plBlackboardTemplateResourceHandle m_hTemplateResource;
  float m_fSortOrder = 0.0f;
};

//////////////////////////////////////////////////////////////////////////

using plVolumeSphereComponentManager = plComponentManager<class plVolumeSphereComponent, plBlockStorageType::Compact>;

class PLASMA_GAMEENGINE_DLL plVolumeSphereComponent : public plVolumeComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plVolumeSphereComponent, plVolumeComponent, plVolumeSphereComponentManager);

public:
  plVolumeSphereComponent();
  ~plVolumeSphereComponent();

  float GetRadius() const { return m_fRadius; }
  void SetRadius(float fRadius);

  float GetFalloff() const { return m_fFalloff; }
  void SetFalloff(float fFalloff);

  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& ref_msg) const;

protected:
  float m_fRadius = 5.0f;
  float m_fFalloff = 0.5f;
};

//////////////////////////////////////////////////////////////////////////

using plVolumeBoxComponentManager = plComponentManager<class plVolumeBoxComponent, plBlockStorageType::Compact>;

class PLASMA_GAMEENGINE_DLL plVolumeBoxComponent : public plVolumeComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plVolumeBoxComponent, plVolumeComponent, plVolumeBoxComponentManager);

public:
  plVolumeBoxComponent();
  ~plVolumeBoxComponent();

  const plVec3& GetExtents() const { return m_vExtents; }
  void SetExtents(const plVec3& vExtents);

  const plVec3& GetFalloff() const { return m_vFalloff; }
  void SetFalloff(const plVec3& vFalloff);

  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& ref_msg) const;

protected:
  plVec3 m_vExtents = plVec3(10.0f);
  plVec3 m_vFalloff = plVec3(0.5f);
};
