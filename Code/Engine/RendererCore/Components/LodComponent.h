#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <RendererCore/Components/RenderComponent.h>

using plLodComponentManager = plComponentManager<class plLodComponent, plBlockStorageType::FreeList>;

struct plMsgExtractRenderData;
struct plMsgComponentInternalTrigger;

class PL_RENDERERCORE_DLL plLodComponent : public plRenderComponent
{
  PL_DECLARE_COMPONENT_TYPE(plLodComponent, plRenderComponent, plLodComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

public:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // plLodComponent

public:
  plLodComponent();
  ~plLodComponent();

protected:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;
  void OnMsgComponentInternalTrigger(plMsgComponentInternalTrigger& msg);

  plInt8 m_iCurState = -1;
  plBoundingBoxSphere m_ChildBounds;
  plStaticArray<float, 4> m_LodThresholds;
};