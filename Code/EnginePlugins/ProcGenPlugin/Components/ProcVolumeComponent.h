#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <ProcGenPlugin/Declarations.h>

struct plMsgTransformChanged;
struct plMsgUpdateLocalBounds;
struct plMsgExtractVolumes;

using plImageDataResourceHandle = plTypedResourceHandle<class plImageDataResource>;

class PL_PROCGENPLUGIN_DLL plProcVolumeComponent : public plComponent
{
  PL_DECLARE_ABSTRACT_COMPONENT_TYPE(plProcVolumeComponent, plComponent);

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

  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  void OnTransformChanged(plMsgTransformChanged& ref_msg);

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

class PL_PROCGENPLUGIN_DLL plProcVolumeSphereComponent : public plProcVolumeComponent
{
  PL_DECLARE_COMPONENT_TYPE(plProcVolumeSphereComponent, plProcVolumeComponent, plProcVolumeSphereComponentManager);

public:
  plProcVolumeSphereComponent();
  ~plProcVolumeSphereComponent();

  float GetRadius() const { return m_fRadius; }
  void SetRadius(float fRadius);

  float GetFalloff() const { return m_fFalloff; }
  void SetFalloff(float fFalloff);

  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& ref_msg) const;
  void OnExtractVolumes(plMsgExtractVolumes& ref_msg) const;

protected:
  float m_fRadius = 5.0f;
  float m_fFalloff = 0.5f;
};

//////////////////////////////////////////////////////////////////////////

using plProcVolumeBoxComponentManager = plComponentManager<class plProcVolumeBoxComponent, plBlockStorageType::Compact>;

class PL_PROCGENPLUGIN_DLL plProcVolumeBoxComponent : public plProcVolumeComponent
{
  PL_DECLARE_COMPONENT_TYPE(plProcVolumeBoxComponent, plProcVolumeComponent, plProcVolumeBoxComponentManager);

public:
  plProcVolumeBoxComponent();
  ~plProcVolumeBoxComponent();

  const plVec3& GetExtents() const { return m_vExtents; }
  void SetExtents(const plVec3& vExtents);

  const plVec3& GetFalloff() const { return m_vFalloff; }
  void SetFalloff(const plVec3& vFalloff);

  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& ref_msg) const;
  void OnExtractVolumes(plMsgExtractVolumes& ref_msg) const;

protected:
  plVec3 m_vExtents = plVec3(10.0f);
  plVec3 m_vFalloff = plVec3(0.5f);
};

//////////////////////////////////////////////////////////////////////////

using plProcVolumeImageComponentManager = plComponentManager<class plProcVolumeImageComponent, plBlockStorageType::Compact>;

class PL_PROCGENPLUGIN_DLL plProcVolumeImageComponent : public plProcVolumeBoxComponent
{
  PL_DECLARE_COMPONENT_TYPE(plProcVolumeImageComponent, plProcVolumeBoxComponent, plProcVolumeImageComponentManager);

public:
  plProcVolumeImageComponent();
  ~plProcVolumeImageComponent();

  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  void OnExtractVolumes(plMsgExtractVolumes& ref_msg) const;

  void SetImageFile(const char* szFile); // [ property ]
  const char* GetImageFile() const;      // [ property ]

  void SetImage(const plImageDataResourceHandle& hResource);
  plImageDataResourceHandle GetImage() const { return m_hImage; }

protected:
  plImageDataResourceHandle m_hImage;
};
