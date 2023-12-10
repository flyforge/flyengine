#pragma once

#include <Core/World/World.h>
#include <RendererCore/RendererCoreDLL.h>

struct plMsgUpdateLocalBounds;

using plBakedProbesVolumeComponentManager = plComponentManager<class plBakedProbesVolumeComponent, plBlockStorageType::Compact>;

class PLASMA_RENDERERCORE_DLL plBakedProbesVolumeComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plBakedProbesVolumeComponent, plComponent, plBakedProbesVolumeComponentManager);

public:
  plBakedProbesVolumeComponent();
  ~plBakedProbesVolumeComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  const plVec3& GetExtents() const { return m_vExtents; }
  void SetExtents(const plVec3& vExtents);

  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& ref_msg) const;

private:
  plVec3 m_vExtents = plVec3(10.0f);
};
