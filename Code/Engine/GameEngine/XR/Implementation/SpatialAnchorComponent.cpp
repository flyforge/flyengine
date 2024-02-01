#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/XR/SpatialAnchorComponent.h>
#include <GameEngine/XR/XRSpatialAnchorsInterface.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plSpatialAnchorComponent, 2, plComponentMode::Dynamic)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("XR"),
    new plInDevelopmentAttribute(plInDevelopmentAttribute::Phase::Beta),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plSpatialAnchorComponent::plSpatialAnchorComponent() = default;
plSpatialAnchorComponent::~plSpatialAnchorComponent()
{
  if (plXRSpatialAnchorsInterface* pXR = plSingletonRegistry::GetSingletonInstance<plXRSpatialAnchorsInterface>())
  {
    if (!m_AnchorID.IsInvalidated())
    {
      pXR->DestroyAnchor(m_AnchorID).IgnoreResult();
      m_AnchorID = plXRSpatialAnchorID();
    }
  }
}

void plSpatialAnchorComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();
}

void plSpatialAnchorComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();
  if (uiVersion == 1)
  {
    plString sAnchorName;
    s >> sAnchorName;
  }
}

plResult plSpatialAnchorComponent::RecreateAnchorAt(const plTransform& position)
{
  if (plXRSpatialAnchorsInterface* pXR = plSingletonRegistry::GetSingletonInstance<plXRSpatialAnchorsInterface>())
  {
    if (!m_AnchorID.IsInvalidated())
    {
      pXR->DestroyAnchor(m_AnchorID).IgnoreResult();
      m_AnchorID = plXRSpatialAnchorID();
    }

    m_AnchorID = pXR->CreateAnchor(position);
    return m_AnchorID.IsInvalidated() ? PL_FAILURE : PL_SUCCESS;
  }

  return PL_SUCCESS;
}

void plSpatialAnchorComponent::Update()
{
  if (IsActiveAndSimulating())
  {
    if (plXRSpatialAnchorsInterface* pXR = plSingletonRegistry::GetSingletonInstance<plXRSpatialAnchorsInterface>())
    {
      if (!m_AnchorID.IsInvalidated())
      {
        plTransform globalTransform;
        if (pXR->TryGetAnchorTransform(m_AnchorID, globalTransform).Succeeded())
        {
          globalTransform.m_vScale = GetOwner()->GetGlobalScaling();
          GetOwner()->SetGlobalTransform(globalTransform);
        }
      }
      else
      {
        m_AnchorID = pXR->CreateAnchor(GetOwner()->GetGlobalTransform());
      }
    }
  }
}

void plSpatialAnchorComponent::OnSimulationStarted()
{
  if (plXRSpatialAnchorsInterface* pXR = plSingletonRegistry::GetSingletonInstance<plXRSpatialAnchorsInterface>())
  {
    m_AnchorID = pXR->CreateAnchor(GetOwner()->GetGlobalTransform());
  }
}

PL_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_SpatialAnchorComponent);
