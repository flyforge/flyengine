#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
PLASMA_IMPLEMENT_MESSAGE_TYPE(plJoltMsgDisconnectConstraints);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plJoltMsgDisconnectConstraints, 1, plRTTIDefaultAllocator<plJoltMsgDisconnectConstraints>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

// clang-format off
PLASMA_BEGIN_ABSTRACT_COMPONENT_TYPE(plJoltActorComponent, 2)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetObjectFilterID),
  }
  PLASMA_END_FUNCTIONS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Physics/Jolt/Actors"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

plJoltActorComponent::plJoltActorComponent() = default;
plJoltActorComponent::~plJoltActorComponent() = default;

void plJoltActorComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_uiCollisionLayer;
}

void plJoltActorComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_uiCollisionLayer;
}

void plJoltActorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_uiObjectFilterID == plInvalidIndex)
  {
    // only create a new filter ID, if none has been passed in manually

    plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
    m_uiObjectFilterID = pModule->CreateObjectFilterID();
  }
}

void plJoltActorComponent::OnDeactivated()
{
  plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();

  JPH::BodyID bodyId(m_uiJoltBodyID);

  if (!bodyId.IsInvalid())
  {
    auto* pSystem = pModule->GetJoltSystem();
    auto* pBodies = &pSystem->GetBodyInterface();

    if (pBodies->IsAdded(bodyId))
    {
      pBodies->RemoveBody(bodyId);
    }

    pBodies->DestroyBody(bodyId);
    m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
  }

  pModule->DeallocateUserData(m_uiUserDataIndex);
  pModule->DeleteObjectFilterID(m_uiObjectFilterID);

  SUPER::OnDeactivated();
}

void plJoltActorComponent::GatherShapes(plDynamicArray<plJoltSubShape>& shapes, plGameObject* pObject, const plTransform& rootTransform, float fDensity, const plJoltMaterial* pMaterial)
{
  plHybridArray<plJoltShapeComponent*, 8> shapeComps;
  pObject->TryGetComponentsOfBaseType(shapeComps);

  for (auto pShape : shapeComps)
  {
    if (pShape->IsActive())
    {
      pShape->CreateShapes(shapes, rootTransform, fDensity, pMaterial);
    }
  }

  for (auto itChild = pObject->GetChildren(); itChild.IsValid(); ++itChild)
  {
    // ignore all children that are actors themselves
    const plJoltActorComponent* pActorComponent;
    if (itChild->TryGetComponentOfBaseType<plJoltActorComponent>(pActorComponent))
      continue;

    GatherShapes(shapes, itChild, rootTransform, fDensity, pMaterial);
  }
}

plResult plJoltActorComponent::CreateShape(JPH::BodyCreationSettings* pSettings, float fDensity, const plJoltMaterial* pMaterial)
{
  plHybridArray<plJoltSubShape, 16> shapes;
  plTransform towner = GetOwner()->GetGlobalTransform();
  towner.m_vScale.Set(1.0f); // pretend like there is no scaling at the root, so that each shape applies its scale

  CreateShapes(shapes, towner, fDensity, pMaterial);
  GatherShapes(shapes, GetOwner(), towner, fDensity, pMaterial);

  auto cleanShapes = [&]() {
    for (auto& s : shapes)
    {
      if (s.m_pShape)
      {
        s.m_pShape->Release();
      }
    }
  };

  PLASMA_SCOPE_EXIT(cleanShapes());

  if (shapes.IsEmpty())
    return PLASMA_FAILURE;

  if (shapes.GetCount() > 0)
  {
    JPH::StaticCompoundShapeSettings opt;

    for (const auto& shape : shapes)
    {
      auto pShape = shape.m_pShape;

      if (!shape.m_Transform.m_vScale.IsEqual(plVec3(1.0f), 0.01f))
      {
        auto* pScaledShape = new JPH::ScaledShape(pShape, plJoltConversionUtils::ToVec3(shape.m_Transform.m_vScale));
        pShape = pScaledShape;
      }

      opt.AddShape(plJoltConversionUtils::ToVec3(shape.m_Transform.m_vPosition), plJoltConversionUtils::ToQuat(shape.m_Transform.m_qRotation).Normalized(), pShape);
    }

    auto res = opt.Create();
    if (!res.IsValid())
      return PLASMA_FAILURE;

    pSettings->SetShape(res.Get());
    return PLASMA_SUCCESS;
  }
  else
  {
    JPH::Shape* pShape = shapes[0].m_pShape;

    if (!shapes[0].m_Transform.m_vScale.IsEqual(plVec3(1.0f), 0.01f))
    {
      auto* pScaledShape = new JPH::ScaledShape(pShape, plJoltConversionUtils::ToVec3(shapes[0].m_Transform.m_vScale));
      pShape = pScaledShape;
    }

    if (!shapes[0].m_Transform.m_vPosition.IsZero(0.01f) || shapes[0].m_Transform.m_qRotation != plQuat::IdentityQuaternion())
    {
      JPH::RotatedTranslatedShapeSettings opt(plJoltConversionUtils::ToVec3(shapes[0].m_Transform.m_vPosition), plJoltConversionUtils::ToQuat(shapes[0].m_Transform.m_qRotation), pShape);

      auto res = opt.Create();
      if (!res.IsValid())
        return PLASMA_FAILURE;

      pShape = res.Get();
    }

    pSettings->SetShape(pShape);
    return PLASMA_SUCCESS;
  }
}

void plJoltActorComponent::ExtractSubShapeGeometry(const plGameObject* pObject, plMsgExtractGeometry& msg) const
{
  plHybridArray<const plJoltShapeComponent*, 8> shapes;
  pObject->TryGetComponentsOfBaseType(shapes);

  for (auto pShape : shapes)
  {
    if (pShape->IsActive())
    {
      pShape->ExtractGeometry(msg);
    }
  }

  for (auto itChild = pObject->GetChildren(); itChild.IsValid(); ++itChild)
  {
    // ignore all children that are actors themselves
    const plJoltActorComponent* pActorComponent;
    if (itChild->TryGetComponentOfBaseType<plJoltActorComponent>(pActorComponent))
      continue;

    ExtractSubShapeGeometry(itChild, msg);
  }
}

const plJoltUserData* plJoltActorComponent::GetUserData() const
{
  const plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();

  return &pModule->GetUserData(m_uiUserDataIndex);
}

void plJoltActorComponent::SetInitialObjectFilterID(plUInt32 uiObjectFilterID)
{
  PLASMA_ASSERT_DEBUG(!IsActiveAndSimulating(), "The object filter ID can't be changed after simulation has started.");
  m_uiObjectFilterID = uiObjectFilterID;
}


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltActorComponent);

