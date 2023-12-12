#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/XR/SpatialAnchorComponent.h>
#include <GameEngine/XR/XRSpatialAnchorsInterface.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plSpatialAnchorComponent, 2, plComponentMode::Dynamic)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("XR"),
    new plColorAttribute(plColorScheme::XR),
    new plInDevelopmentAttribute(plInDevelopmentAttribute::Phase::Beta),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
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

void plSpatialAnchorComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  plStreamWriter& s = stream.GetStream();
}

void plSpatialAnchorComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = stream.GetStream();
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
    return m_AnchorID.IsInvalidated() ? PLASMA_FAILURE : PLASMA_SUCCESS;
  }

  return PLASMA_SUCCESS;
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

PLASMA_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_SpatialAnchorComponent);
