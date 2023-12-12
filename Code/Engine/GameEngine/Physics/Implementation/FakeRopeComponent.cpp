#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Interfaces/WindWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/FakeRopeComponent.h>
#include <RendererCore/AnimationSystem/Declarations.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plFakeRopeComponent, 3, plComponentMode::Static)
  {
    PLASMA_BEGIN_PROPERTIES
    {
      PLASMA_ACCESSOR_PROPERTY("Anchor1", DummyGetter, SetAnchor1Reference)->AddAttributes(new plGameObjectReferenceAttribute()),
      PLASMA_ACCESSOR_PROPERTY("Anchor2", DummyGetter, SetAnchor2Reference)->AddAttributes(new plGameObjectReferenceAttribute()),
      PLASMA_ACCESSOR_PROPERTY("AttachToAnchor1", GetAttachToAnchor1, SetAttachToAnchor1)->AddAttributes(new plDefaultValueAttribute(true)),
      PLASMA_ACCESSOR_PROPERTY("AttachToAnchor2", GetAttachToAnchor2, SetAttachToAnchor2)->AddAttributes(new plDefaultValueAttribute(true)),
      PLASMA_MEMBER_PROPERTY("Pieces", m_uiPieces)->AddAttributes(new plDefaultValueAttribute(32), new plClampValueAttribute(2, 200)),
      PLASMA_ACCESSOR_PROPERTY("Slack", GetSlack, SetSlack)->AddAttributes(new plDefaultValueAttribute(0.2f)),
      PLASMA_MEMBER_PROPERTY("Damping", m_fDamping)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, 1.0f)),
      PLASMA_MEMBER_PROPERTY("WindInfluence", m_fWindInfluence)->AddAttributes(new plDefaultValueAttribute(0.2f), new plClampValueAttribute(0.0f, 10.0f)),
    }
    PLASMA_END_PROPERTIES;
    PLASMA_BEGIN_ATTRIBUTES
    {
      new plCategoryAttribute("Effects/Ropes"),
      new plColorAttribute(plColorScheme::Effects),
    }
    PLASMA_END_ATTRIBUTES;
  }
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plFakeRopeComponent::plFakeRopeComponent() = default;
plFakeRopeComponent::~plFakeRopeComponent() = default;

void plFakeRopeComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_uiPieces;
  s << m_fSlack;
  s << m_fDamping;
  s << m_RopeSim.m_bFirstNodeIsFixed;
  s << m_RopeSim.m_bLastNodeIsFixed;

  inout_stream.WriteGameObjectHandle(m_hAnchor1);
  inout_stream.WriteGameObjectHandle(m_hAnchor2);

  s << m_fWindInfluence;
}

void plFakeRopeComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_uiPieces;
  s >> m_fSlack;
  s >> m_fDamping;
  s >> m_RopeSim.m_bFirstNodeIsFixed;
  s >> m_RopeSim.m_bLastNodeIsFixed;

  if (uiVersion >= 3)
  {
    m_hAnchor1 = inout_stream.ReadGameObjectHandle();
  }

  m_hAnchor2 = inout_stream.ReadGameObjectHandle();

  if (uiVersion >= 2)
  {
    s >> m_fWindInfluence;
  }
}

void plFakeRopeComponent::OnActivated()
{
  m_uiPreviewHash = 0;
  m_RopeSim.m_Nodes.Clear();
  m_RopeSim.m_fSegmentLength = -1.0f;

  m_uiCheckEquilibriumCounter = GetOwner()->GetStableRandomSeed() & 63;

  SendPreviewPose();
}

void plFakeRopeComponent::OnDeactivated()
{
  // tell the render components, that the rope is gone
  m_RopeSim.m_Nodes.Clear();
  SendCurrentPose();

  SUPER::OnDeactivated();
}

plResult plFakeRopeComponent::ConfigureRopeSimulator()
{
  if (!m_bIsDynamic)
    return PLASMA_SUCCESS;

  if (!IsActiveAndInitialized())
    return PLASMA_FAILURE;

  plGameObjectHandle hAnchor1 = m_hAnchor1;
  plGameObjectHandle hAnchor2 = m_hAnchor2;

  if (hAnchor1.IsInvalidated())
    hAnchor1 = GetOwner()->GetHandle();
  if (hAnchor2.IsInvalidated())
    hAnchor2 = GetOwner()->GetHandle();

  if (hAnchor1 == hAnchor2)
    return PLASMA_FAILURE;

  plSimdVec4f anchor1;
  plSimdVec4f anchor2;

  plGameObject* pAnchor1 = nullptr;
  plGameObject* pAnchor2 = nullptr;

  if (!GetWorld()->TryGetObject(hAnchor1, pAnchor1))
  {
    // never set up so far
    if (m_RopeSim.m_Nodes.IsEmpty())
      return PLASMA_FAILURE;

    if (m_RopeSim.m_bFirstNodeIsFixed)
    {
      anchor1 = m_RopeSim.m_Nodes[0].m_vPosition;
      m_RopeSim.m_bFirstNodeIsFixed = false;
      m_uiSleepCounter = 0;
    }
  }
  else
  {
    anchor1 = plSimdConversion::ToVec3(pAnchor1->GetGlobalPosition());
  }

  if (!GetWorld()->TryGetObject(hAnchor2, pAnchor2))
  {
    // never set up so far
    if (m_RopeSim.m_Nodes.IsEmpty())
      return PLASMA_FAILURE;

    if (m_RopeSim.m_bLastNodeIsFixed)
    {
      anchor2 = m_RopeSim.m_Nodes.PeekBack().m_vPosition;
      m_RopeSim.m_bLastNodeIsFixed = false;
      m_uiSleepCounter = 0;
    }
  }
  else
  {
    anchor2 = plSimdConversion::ToVec3(pAnchor2->GetGlobalPosition());
  }

  // only early out, if we are not in edit mode
  m_bIsDynamic = !IsActiveAndSimulating() || (pAnchor1 != nullptr && pAnchor1->IsDynamic()) || (pAnchor2 != nullptr && pAnchor2->IsDynamic());

  m_RopeSim.m_fDampingFactor = plMath::Lerp(1.0f, 0.97f, m_fDamping);

  if (m_RopeSim.m_fSegmentLength < 0)
  {
    const float len = (anchor1 - anchor2).GetLength<3>();
    m_RopeSim.m_fSegmentLength = (len + len * m_fSlack) / m_uiPieces;
  }

  if (const plPhysicsWorldModuleInterface* pModule = GetWorld()->GetModuleReadOnly<plPhysicsWorldModuleInterface>())
  {
    if (m_RopeSim.m_vAcceleration != pModule->GetGravity())
    {
      m_uiSleepCounter = 0;
      m_RopeSim.m_vAcceleration = pModule->GetGravity();
    }
  }

  if (m_uiPieces < m_RopeSim.m_Nodes.GetCount())
  {
    m_uiSleepCounter = 0;
    m_RopeSim.m_Nodes.SetCount(m_uiPieces);
  }
  else if (m_uiPieces > m_RopeSim.m_Nodes.GetCount())
  {
    m_uiSleepCounter = 0;
    const plUInt32 uiOldNum = m_RopeSim.m_Nodes.GetCount();

    m_RopeSim.m_Nodes.SetCount(m_uiPieces);

    for (plUInt32 i = uiOldNum; i < m_uiPieces; ++i)
    {
      m_RopeSim.m_Nodes[i].m_vPosition = anchor1 + ((anchor2 - anchor1) * (float)i / (m_uiPieces - 1));
      m_RopeSim.m_Nodes[i].m_vPreviousPosition = m_RopeSim.m_Nodes[i].m_vPosition;
    }
  }

  if (!m_RopeSim.m_Nodes.IsEmpty())
  {
    if (m_RopeSim.m_bFirstNodeIsFixed)
    {
      if ((m_RopeSim.m_Nodes[0].m_vPosition != anchor1).AnySet<3>())
      {
        m_uiSleepCounter = 0;
        m_RopeSim.m_Nodes[0].m_vPosition = anchor1;
      }
    }

    if (m_RopeSim.m_bLastNodeIsFixed)
    {
      if ((m_RopeSim.m_Nodes.PeekBack().m_vPosition != anchor2).AnySet<3>())
      {
        m_uiSleepCounter = 0;
        m_RopeSim.m_Nodes.PeekBack().m_vPosition = anchor2;
      }
    }
  }

  return PLASMA_SUCCESS;
}

void plFakeRopeComponent::SendPreviewPose()
{
  if (!IsActiveAndInitialized() || IsActiveAndSimulating())
    return;

  plGameObject* pAnchor1 = nullptr;
  plGameObject* pAnchor2 = nullptr;
  if (!GetWorld()->TryGetObject(m_hAnchor1, pAnchor1))
    pAnchor1 = GetOwner();
  if (!GetWorld()->TryGetObject(m_hAnchor2, pAnchor2))
    pAnchor2 = GetOwner();

  if (pAnchor1 == pAnchor2)
    return;

  plUInt32 uiHash = 0;

  plVec3 pos = GetOwner()->GetGlobalPosition();
  uiHash = plHashingUtils::xxHash32(&pos, sizeof(plVec3), uiHash);

  pos = pAnchor1->GetGlobalPosition();
  uiHash = plHashingUtils::xxHash32(&pos, sizeof(plVec3), uiHash);

  pos = pAnchor2->GetGlobalPosition();
  uiHash = plHashingUtils::xxHash32(&pos, sizeof(plVec3), uiHash);

  uiHash = plHashingUtils::xxHash32(&m_fSlack, sizeof(float), uiHash);
  uiHash = plHashingUtils::xxHash32(&m_fDamping, sizeof(float), uiHash);
  uiHash = plHashingUtils::xxHash32(&m_uiPieces, sizeof(plUInt16), uiHash);

  if (uiHash == m_uiPreviewHash)
    return;

  m_uiPreviewHash = uiHash;
  m_RopeSim.m_fSegmentLength = -1.0f;

  if (ConfigureRopeSimulator().Failed())
    return;

  m_RopeSim.SimulateTillEquilibrium(0.003f, 100);

  SendCurrentPose();
}

void plFakeRopeComponent::RuntimeUpdate()
{
  if (ConfigureRopeSimulator().Failed())
    return;

  plVec3 acc(0);

  if (const plPhysicsWorldModuleInterface* pModule = GetWorld()->GetModuleReadOnly<plPhysicsWorldModuleInterface>())
  {
    acc += pModule->GetGravity();
  }
  else
  {
    acc += plVec3(0, 0, -9.81f);
  }

  if (m_fWindInfluence > 0.0f)
  {
    if (const plWindWorldModuleInterface* pWind = GetWorld()->GetModuleReadOnly<plWindWorldModuleInterface>())
    {
      const plSimdVec4f ropeDir = m_RopeSim.m_Nodes.PeekBack().m_vPosition - m_RopeSim.m_Nodes[0].m_vPosition;

      plVec3 vWind = pWind->GetWindAt(plSimdConversion::ToVec3(m_RopeSim.m_Nodes.PeekBack().m_vPosition));
      vWind += pWind->GetWindAt(plSimdConversion::ToVec3(m_RopeSim.m_Nodes[0].m_vPosition));
      vWind *= 0.5f * m_fWindInfluence;

      acc += vWind;
      acc += pWind->ComputeWindFlutter(vWind, plSimdConversion::ToVec3(ropeDir), 0.5f, GetOwner()->GetStableRandomSeed());
    }
  }

  if (m_RopeSim.m_vAcceleration != acc)
  {
    m_RopeSim.m_vAcceleration = acc;
    m_uiSleepCounter = 0;
  }

  if (m_uiSleepCounter > 10)
    return;

  plVisibilityState visType = GetOwner()->GetVisibilityState();

  if (visType == plVisibilityState::Invisible)
    return;


  m_RopeSim.SimulateRope(GetWorld()->GetClock().GetTimeDiff());

  ++m_uiCheckEquilibriumCounter;
  if (m_uiCheckEquilibriumCounter > 64)
  {
    m_uiCheckEquilibriumCounter = 0;

    if (m_RopeSim.HasEquilibrium(0.01f))
    {
      ++m_uiSleepCounter;
    }
    else
    {
      m_uiSleepCounter = 0;
    }
  }

  SendCurrentPose();
}

void plFakeRopeComponent::SendCurrentPose()
{
  plMsgRopePoseUpdated poseMsg;

  plDynamicArray<plTransform> pieces(plFrameAllocator::GetCurrentAllocator());

  if (m_RopeSim.m_Nodes.GetCount() >= 2)
  {
    const plTransform tRoot = GetOwner()->GetGlobalTransform();

    pieces.SetCountUninitialized(m_RopeSim.m_Nodes.GetCount());

    plTransform tGlobal;
    tGlobal.m_vScale.Set(1);

    for (plUInt32 i = 0; i < pieces.GetCount() - 1; ++i)
    {
      const plSimdVec4f p0 = m_RopeSim.m_Nodes[i].m_vPosition;
      const plSimdVec4f p1 = m_RopeSim.m_Nodes[i + 1].m_vPosition;
      plSimdVec4f dir = p1 - p0;

      dir.NormalizeIfNotZero<3>();

      tGlobal.m_vPosition = plSimdConversion::ToVec3(p0);
      tGlobal.m_qRotation.SetShortestRotation(plVec3::UnitXAxis(), plSimdConversion::ToVec3(dir));

      pieces[i].SetLocalTransform(tRoot, tGlobal);
    }

    {
      tGlobal.m_vPosition = plSimdConversion::ToVec3(m_RopeSim.m_Nodes.PeekBack().m_vPosition);
      // tGlobal.m_qRotation is the same as from the previous bone

      pieces.PeekBack().SetLocalTransform(tRoot, tGlobal);
    }


    poseMsg.m_LinkTransforms = pieces;
  }

  GetOwner()->PostMessage(poseMsg, plTime::Zero(), plObjectMsgQueueType::AfterInitialized);
}

void plFakeRopeComponent::SetAnchor1Reference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetAnchor1(resolver(szReference, GetHandle(), "Anchor1"));
}

void plFakeRopeComponent::SetAnchor2Reference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetAnchor2(resolver(szReference, GetHandle(), "Anchor2"));
}

void plFakeRopeComponent::SetAnchor1(plGameObjectHandle hActor)
{
  m_hAnchor1 = hActor;
  m_bIsDynamic = true;
  m_uiSleepCounter = 0;
}

void plFakeRopeComponent::SetAnchor2(plGameObjectHandle hActor)
{
  m_hAnchor2 = hActor;
  m_bIsDynamic = true;
  m_uiSleepCounter = 0;
}

void plFakeRopeComponent::SetSlack(float val)
{
  m_fSlack = val;
  m_RopeSim.m_fSegmentLength = -1.0f;
  m_bIsDynamic = true;
  m_uiSleepCounter = 0;
}

void plFakeRopeComponent::SetAttachToAnchor1(bool bVal)
{
  m_RopeSim.m_bFirstNodeIsFixed = bVal;
  m_bIsDynamic = true;
  m_uiSleepCounter = 0;
}

void plFakeRopeComponent::SetAttachToAnchor2(bool bVal)
{
  m_RopeSim.m_bLastNodeIsFixed = bVal;
  m_bIsDynamic = true;
  m_uiSleepCounter = 0;
}

bool plFakeRopeComponent::GetAttachToAnchor1() const
{
  return m_RopeSim.m_bFirstNodeIsFixed;
}

bool plFakeRopeComponent::GetAttachToAnchor2() const
{
  return m_RopeSim.m_bLastNodeIsFixed;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plFakeRopeComponentManager::plFakeRopeComponentManager(plWorld* pWorld)
  : plComponentManager(pWorld)
{
}

plFakeRopeComponentManager::~plFakeRopeComponentManager() = default;

void plFakeRopeComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plFakeRopeComponentManager::Update, this);
    desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::Async;
    desc.m_bOnlyUpdateWhenSimulating = false;

    this->RegisterUpdateFunction(desc);
  }
}

void plFakeRopeComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  if (!GetWorld()->GetWorldSimulationEnabled())
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActiveAndInitialized())
      {
        it->SendPreviewPose();
      }
    }

    return;
  }

  if (GetWorld()->GetWorldSimulationEnabled())
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      if (it->IsActiveAndInitialized())
      {
        it->RuntimeUpdate();
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class plFakeRopeComponentPatch_2_3 : public plGraphPatch
{
public:
  plFakeRopeComponentPatch_2_3()
    : plGraphPatch("plFakeRopeComponent", 3)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Anchor", "Anchor2");
    pNode->RenameProperty("AttachToOrigin", "AttachToAnchor1");
    pNode->RenameProperty("AttachToAnchor", "AttachToAnchor2");
  }
};

plFakeRopeComponentPatch_2_3 g_plFakeRopeComponentPatch_2_3;
