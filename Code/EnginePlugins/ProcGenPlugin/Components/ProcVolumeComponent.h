#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <ProcGenPlugin/Declarations.h>

struct plMsgTransformChanged;
struct plMsgUpdateLocalBounds;
struct plMsgExtractVolumes;

using plImageDataResourceHandle = plTypedResourceHandle<class plImageDataResource>;

class PLASMA_PROCGENPLUGIN_DLL plProcVolumeComponent : public plComponent
{
  PLASMA_DECLARE_ABSTRACT_COMPONENT_TYPE(plProcVolumeComponent, plComponent);

public:
  plProcVolumeComponent();
  ~plProcVolumeComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetValue(float fValue);
  float GetValue() const { return m_fValue; }

  void SetSortOrder(float fOrder);
  float GetSortOrder() const { return m_fSortOrder; }

  void SetBlendMode(plEnum<plProcGenBlendMode> blendMode);
  plEnum<plProcGenBlendMode> GetBlendMode() const { return m_BlendMode; }

  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  void OnTransformChanged(plMsgTransformChanged& msg);

  static const plEvent<const plProcGenInternal::InvalidatedArea&>& GetAreaInvalidatedEvent() { return s_AreaInvalidatedEvent; }

protected:
  float m_fValue = 1.0f;
  float m_fSortOrder = 0.0f;
  plEnum<plProcGenBlendMode> m_BlendMode;

  void InvalidateArea();
  void InvalidateArea(const plBoundingBox& area);

  static plEvent<const plProcGenInternal::InvalidatedArea&> s_AreaInvalidatedEvent;
};

//////////////////////////////////////////////////////////////////////////

using plProcVolumeSphereComponentManager = plComponentManager<class plProcVolumeSphereComponent, plBlockStorageType::Compact>;

class PLASMA_PROCGENPLUGIN_DLL plProcVolumeSphereComponent : public plProcVolumeComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plProcVolumeSphereComponent, plProcVolumeComponent, plProcVolumeSphereComponentManager);

public:
  plProcVolumeSphereComponent();
  ~plProcVolumeSphereComponent();

  float GetRadius() const { return m_fRadius; }
  void SetRadius(float fRadius);

  float GetFadeOutStart() const { return m_fFadeOutStart; }
  void SetFadeOutStart(float fFadeOutStart);

  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const;
  void OnExtractVolumes(plMsgExtractVolumes& msg) const;

protected:
  float m_fRadius = 5.0f;
  float m_fFadeOutStart = 0.5f;
};

//////////////////////////////////////////////////////////////////////////

using plProcVolumeBoxComponentManager = plComponentManager<class plProcVolumeBoxComponent, plBlockStorageType::Compact>;

class PLASMA_PROCGENPLUGIN_DLL plProcVolumeBoxComponent : public plProcVolumeComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plProcVolumeBoxComponent, plProcVolumeComponent, plProcVolumeBoxComponentManager);

public:
  plProcVolumeBoxComponent();
  ~plProcVolumeBoxComponent();

  const plVec3& GetExtents() const { return m_vExtents; }
  void SetExtents(const plVec3& extents);

  const plVec3& GetFadeOutStart() const { return m_vFadeOutStart; }
  void SetFadeOutStart(const plVec3& fadeOutStart);

  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const;
  void OnExtractVolumes(plMsgExtractVolumes& msg) const;

protected:
  plVec3 m_vExtents = plVec3(10.0f);
  plVec3 m_vFadeOutStart = plVec3(0.5f);
};

//////////////////////////////////////////////////////////////////////////

using plProcVolumeImageComponentManager = plComponentManager<class plProcVolumeImageComponent, plBlockStorageType::Compact>;

class PLASMA_PROCGENPLUGIN_DLL plProcVolumeImageComponent : public plProcVolumeBoxComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plProcVolumeImageComponent, plProcVolumeBoxComponent, plProcVolumeImageComponentManager);

public:
  plProcVolumeImageComponent();
  ~plProcVolumeImageComponent();

  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  void OnExtractVolumes(plMsgExtractVolumes& msg) const;

  void SetImageFile(const char* szFile); // [ property ]
  const char* GetImageFile() const;      // [ property ]

  void SetImage(const plImageDataResourceHandle& hResource);
  plImageDataResourceHandle GetImage() const { return m_hImage; }

protected:
  plImageDataResourceHandle m_hImage;
};
