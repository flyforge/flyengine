#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/RendererCoreDLL.h>

struct plMsgTransformChanged;
struct plMsgUpdateLocalBounds;
struct plMsgExtractOccluderData;

class PLASMA_RENDERERCORE_DLL plOccluderComponentManager final : public plComponentManager<class plOccluderComponent, plBlockStorageType::FreeList>
{
public:
  plOccluderComponentManager(plWorld* pWorld);
};

class PLASMA_RENDERERCORE_DLL plOccluderComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plOccluderComponent, plComponent, plOccluderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plBoxReflectionProbeComponent

public:
  plOccluderComponent();
  ~plOccluderComponent();

  const plVec3& GetExtents() const
  {
    return m_vExtents;
  }

  void SetExtents(const plVec3& vExtents);

private:
  plVec3 m_vExtents = plVec3(5.0f);

  mutable plSharedPtr<const plRasterizerObject> m_pOccluderObject;

  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg);
  void OnMsgExtractOccluderData(plMsgExtractOccluderData& msg) const;
};
