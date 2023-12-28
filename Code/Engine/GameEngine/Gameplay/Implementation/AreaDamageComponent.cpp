#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Gameplay/AreaDamageComponent.h>
#include <GameEngine/Messages/DamageMessage.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plAreaDamageComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("OnCreation", m_bTriggerOnCreation)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new plDefaultValueAttribute(5.0f), new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),
    PLASMA_MEMBER_PROPERTY("Damage", m_fDamage)->AddAttributes(new plDefaultValueAttribute(10.0f)),
    PLASMA_MEMBER_PROPERTY("Impulse", m_fImpulse)->AddAttributes(new plDefaultValueAttribute(100.0f)),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(ApplyAreaDamage),
  }
  PLASMA_END_FUNCTIONS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Gameplay"),
    new plSphereVisualizerAttribute("Radius", plColor::OrangeRed),
    new plSphereManipulatorAttribute("Radius"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static plPhysicsOverlapResultArray g_OverlapResults;

plAreaDamageComponent::plAreaDamageComponent() = default;
plAreaDamageComponent::~plAreaDamageComponent() = default;

void plAreaDamageComponent::ApplyAreaDamage()
{
  if (!IsActiveAndSimulating())
    return;

  PLASMA_PROFILE_SCOPE("ApplyAreaDamage");

  plPhysicsWorldModuleInterface* pPhysicsInterface = GetWorld()->GetOrCreateModule<plPhysicsWorldModuleInterface>();

  if (pPhysicsInterface == nullptr)
    return;

  const plVec3 vOwnPosition = GetOwner()->GetGlobalPosition();

  plPhysicsQueryParameters query(m_uiCollisionLayer);
  query.m_ShapeTypes.Remove(plPhysicsShapeType::Static | plPhysicsShapeType::Trigger);

  pPhysicsInterface->QueryShapesInSphere(g_OverlapResults, m_fRadius, vOwnPosition, query);

  const float fInvRadius = 1.0f / m_fRadius;

  for (const auto& hit : g_OverlapResults.m_Results)
  {
    if (!hit.m_hActorObject.IsInvalidated())
    {
      plGameObject* pObject = nullptr;
      if (GetWorld()->TryGetObject(hit.m_hActorObject, pObject))
      {
        const plVec3 vTargetPos = hit.m_vCenterPosition;
        const plVec3 vDistToTarget = vTargetPos - vOwnPosition;
        plVec3 vDirToTarget = vDistToTarget;
        const float fDistance = vDirToTarget.GetLength();

        if (fDistance >= 0.01f)
        {
          // if the direction is valid (non-zero), just normalize it
          vDirToTarget /= fDistance;
        }
        else
        {
          // otherwise, if we are so close, that the distance is zero, pick a random direction away from it
          vDirToTarget.CreateRandomDirection(GetWorld()->GetRandomNumberGenerator());
        }

        // linearly scale damage and impulse down by distance
        const float fScale = 1.0f - plMath::Min(fDistance * fInvRadius, 1.0f);

        // apply a physical impulse
        if (m_fImpulse != 0.0f)
        {
          plMsgPhysicsAddImpulse msg;
          msg.m_vGlobalPosition = vTargetPos;
          msg.m_vImpulse = vDirToTarget * m_fImpulse * fScale;
          msg.m_uiObjectFilterID = hit.m_uiObjectFilterID;
          msg.m_pInternalPhysicsShape = hit.m_pInternalPhysicsShape;
          msg.m_pInternalPhysicsActor = hit.m_pInternalPhysicsActor;


          pObject->SendMessage(msg);
        }

        // apply damage
        if (m_fDamage != 0.0f)
        {
          plMsgDamage msg;
          msg.m_fDamage = static_cast<double>(m_fDamage) * static_cast<double>(fScale);
          msg.m_vImpactDirection = vDirToTarget;
          msg.m_vGlobalPosition = vOwnPosition + vDistToTarget * 0.9f; // rough guess for a position where to apply the damage

          plGameObject* pShape = nullptr;
          if (GetWorld()->TryGetObject(hit.m_hShapeObject, pShape))
          {
            msg.m_sHitObjectName = pShape->GetName();
          }
          else
          {
            msg.m_sHitObjectName = pObject->GetName();
          }

          // delay the damage a little bit for nicer chain reactions
          pObject->PostEventMessage(msg, this, plTime::Milliseconds(200));
        }
      }
    }
  }
}

void plAreaDamageComponent::OnSimulationStarted()
{
  if (m_bTriggerOnCreation)
  {
    ApplyAreaDamage();
  }
}

void plAreaDamageComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_bTriggerOnCreation;
  s << m_fRadius;
  s << m_uiCollisionLayer;
  s << m_fDamage;
  s << m_fImpulse;
}

void plAreaDamageComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_bTriggerOnCreation;
  s >> m_fRadius;
  s >> m_uiCollisionLayer;
  s >> m_fDamage;
  s >> m_fImpulse;
}

//////////////////////////////////////////////////////////////////////////

plAreaDamageComponentManager::plAreaDamageComponentManager(plWorld* pWorld)
  : SUPER(pWorld)

{
}

void plAreaDamageComponentManager::Initialize()
{
  SUPER::Initialize();

  m_pPhysicsInterface = GetWorld()->GetOrCreateModule<plPhysicsWorldModuleInterface>();
}
