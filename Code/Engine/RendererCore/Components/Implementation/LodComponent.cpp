#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/LodComponent.h>


// clang-format off
PL_BEGIN_COMPONENT_TYPE(plLodComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ARRAY_MEMBER_PROPERTY("LodThresholds", m_LodThresholds)->AddAttributes(new plMaxArraySizeAttribute(4))
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Effects"),
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
    PL_MESSAGE_HANDLER(plMsgComponentInternalTrigger, OnMsgComponentInternalTrigger),
  }
  PL_END_MESSAGEHANDLERS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plLodComponent::plLodComponent() = default;
plLodComponent::~plLodComponent() = default;

void plLodComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s.WriteArray(m_LodThresholds).AssertSuccess();
}

void plLodComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();
  s.ReadArray(m_LodThresholds).AssertSuccess();
}

plResult plLodComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  ref_bounds = m_ChildBounds;
  return PL_SUCCESS;
}

void plLodComponent::OnActivated()
{
  SUPER::OnActivated();

  m_iCurState = -1;

  plMsgComponentInternalTrigger trig;
  trig.m_iPayload = 0;
  OnMsgComponentInternalTrigger(trig);

  TriggerLocalBoundsUpdate();
}

void plLodComponent::OnDeactivated()
{
  plGameObject* pLod[5];
  pLod[0] = GetOwner()->FindChildByName("LOD0");
  pLod[1] = GetOwner()->FindChildByName("LOD1");
  pLod[2] = GetOwner()->FindChildByName("LOD2");
  pLod[3] = GetOwner()->FindChildByName("LOD3");
  pLod[4] = GetOwner()->FindChildByName("LOD4");

  for (plUInt32 i = 0; i < PL_ARRAY_SIZE(pLod); ++i)
  {
    if (pLod[i])
    {
      pLod[i]->SetActiveFlag(true);
    }
  }

  SUPER::OnDeactivated();
}

void plLodComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  if (msg.m_pView->GetCameraUsageHint() != plCameraUsageHint::EditorView &&
      msg.m_pView->GetCameraUsageHint() != plCameraUsageHint::MainView)
  {
    return;
  }

  // TODO: figure out how to get this message
  // currently it is only sent when the LOD object is selected


  // TODO: use projected screen size to determine LOD
  // we can't use the actual LOD bounds, because they can be drastically different, ie if one LOD contains a light source, it may be huge
  // but we can just use a unit sphere as reference, since we are probably only talking about percentages of screen coverage anyway
  // TODO: I have no idea how to calculate that, so distance it is for now !

  const plUInt32 uiNumLods = m_LodThresholds.GetCount();
  const float fDistSqr = (msg.m_pView->GetCamera()->GetCenterPosition() - GetOwner()->GetGlobalPosition()).GetLengthSquared();

  plInt32 iNewState = 0;

  if (uiNumLods < 1 || fDistSqr < plMath::Square(m_LodThresholds[0]))
    iNewState = 0;
  else if (uiNumLods < 2 || fDistSqr < plMath::Square(m_LodThresholds[1]))
    iNewState = 1;
  else if (uiNumLods < 3 || fDistSqr < plMath::Square(m_LodThresholds[2]))
    iNewState = 2;
  else if (uiNumLods < 4 || fDistSqr < plMath::Square(m_LodThresholds[3]))
    iNewState = 3;
  else if (uiNumLods < 5)
    iNewState = 4;

  if (iNewState == m_iCurState)
    return;

  plMsgComponentInternalTrigger trig;
  trig.m_iPayload = iNewState;

  PostMessage(trig);
}

void plLodComponent::OnMsgComponentInternalTrigger(plMsgComponentInternalTrigger& msg)
{
  m_iCurState = msg.m_iPayload;

  plGameObject* pLod[5];
  pLod[0] = GetOwner()->FindChildByName("LOD0");
  pLod[1] = GetOwner()->FindChildByName("LOD1");
  pLod[2] = GetOwner()->FindChildByName("LOD2");
  pLod[3] = GetOwner()->FindChildByName("LOD3");
  pLod[4] = GetOwner()->FindChildByName("LOD4");


  for (plUInt32 i = 0; i < PL_ARRAY_SIZE(pLod); ++i)
  {
    if (pLod[i])
    {
      if (m_iCurState == i)
      {
        m_ChildBounds = pLod[i]->GetLocalBounds();
        pLod[i]->SetActiveFlag(true);
      }
      else
      {
        pLod[i]->SetActiveFlag(false);
      }
    }
  }
}