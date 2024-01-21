#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/AI/SensorComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgSensorDetectedObjectsChanged);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgSensorDetectedObjectsChanged, 1, plRTTIDefaultAllocator<plMsgSensorDetectedObjectsChanged>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY_READ_ONLY("DetectedObjects", m_DetectedObjects),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_ABSTRACT_COMPONENT_TYPE(plSensorComponent, 1)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_MEMBER_PROPERTY("UpdateRate", plUpdateRate, m_UpdateRate),
    PLASMA_ACCESSOR_PROPERTY("SpatialCategory", GetSpatialCategory, SetSpatialCategory)->AddAttributes(new plDynamicStringEnumAttribute("SpatialDataCategoryEnum")),
    PLASMA_MEMBER_PROPERTY("TestVisibility", m_bTestVisibility)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),
    PLASMA_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    PLASMA_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new plDefaultValueAttribute(plColorScheme::LightUI(plColorScheme::Orange))),
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("AI/Sensors"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

plSensorComponent::plSensorComponent() = default;
plSensorComponent::~plSensorComponent() = default;

void plSensorComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_sSpatialCategory;
  s << m_bTestVisibility;
  s << m_uiCollisionLayer;
  s << m_UpdateRate;
  s << m_bShowDebugInfo;
  s << m_Color;
}

void plSensorComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_sSpatialCategory;
  s >> m_bTestVisibility;
  s >> m_uiCollisionLayer;
  s >> m_UpdateRate;
  s >> m_bShowDebugInfo;
  s >> m_Color;
}

void plSensorComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateSpatialCategory();
  UpdateScheduling();
  UpdateDebugInfo();
}

void plSensorComponent::OnDeactivated()
{
  auto pModule = GetWorld()->GetOrCreateModule<plSensorWorldModule>();
  pModule->RemoveComponentToSchedule(this);

  UpdateDebugInfo();
}

void plSensorComponent::SetSpatialCategory(const char* szCategory)
{
  m_sSpatialCategory.Assign(szCategory);

  if (IsActiveAndInitialized())
  {
    UpdateSpatialCategory();
  }
}

const char* plSensorComponent::GetSpatialCategory() const
{
  return m_sSpatialCategory;
}

void plSensorComponent::SetUpdateRate(const plEnum<plUpdateRate>& updateRate)
{
  if (m_UpdateRate == updateRate)
    return;

  m_UpdateRate = updateRate;

  if (IsActiveAndInitialized())
  {
    UpdateScheduling();
  }
}

const plEnum<plUpdateRate>& plSensorComponent::GetUpdateRate() const
{
  return m_UpdateRate;
}

void plSensorComponent::SetShowDebugInfo(bool bShow)
{
  if (m_bShowDebugInfo == bShow)
    return;

  m_bShowDebugInfo = bShow;

  if (IsActiveAndInitialized())
  {
    UpdateDebugInfo();
  }
}

bool plSensorComponent::GetShowDebugInfo() const
{
  return m_bShowDebugInfo;
}

void plSensorComponent::SetColor(plColorGammaUB color)
{
  m_Color = color;
}

plColorGammaUB plSensorComponent::GetColor() const
{
  return m_Color;
}

bool plSensorComponent::RunSensorCheck(plPhysicsWorldModuleInterface* pPhysicsWorldModule, plDynamicArray<plGameObject*>& out_objectsInSensorVolume, plDynamicArray<plGameObjectHandle>& ref_detectedObjects, bool bPostChangeMsg) const
{
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  m_LastOccludedObjectPositions.Clear();
#endif

  out_objectsInSensorVolume.Clear();

  GetObjectsInSensorVolume(out_objectsInSensorVolume);
  const plGameObject* pSensorOwner = GetOwner();

  ref_detectedObjects.Clear();

  if (m_bTestVisibility && pPhysicsWorldModule)
  {
    const plVec3 rayStart = pSensorOwner->GetGlobalPosition();
    for (auto pObject : out_objectsInSensorVolume)
    {
      const plVec3 rayEnd = pObject->GetGlobalPosition();
      plVec3 rayDir = rayEnd - rayStart;
      const float fDistance = rayDir.GetLengthAndNormalize();

      plPhysicsCastResult hitResult;
      plPhysicsQueryParameters params(m_uiCollisionLayer);
      params.m_bIgnoreInitialOverlap = true;
      params.m_ShapeTypes = plPhysicsShapeType::Default;

      // TODO: probably best to expose the plPhysicsShapeType bitflags on the component
      params.m_ShapeTypes.Remove(plPhysicsShapeType::Rope);
      params.m_ShapeTypes.Remove(plPhysicsShapeType::Ragdoll);
      params.m_ShapeTypes.Remove(plPhysicsShapeType::Trigger);
      params.m_ShapeTypes.Remove(plPhysicsShapeType::Query);
      params.m_ShapeTypes.Remove(plPhysicsShapeType::Character);

      if (pPhysicsWorldModule->Raycast(hitResult, rayStart, rayDir, fDistance, params))
      {
        // hit something in between -> not visible
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
        m_LastOccludedObjectPositions.PushBack(rayEnd);
#endif

        continue;
      }

      ref_detectedObjects.PushBack(pObject->GetHandle());
    }
  }
  else
  {
    for (auto pObject : out_objectsInSensorVolume)
    {
      ref_detectedObjects.PushBack(pObject->GetHandle());
    }
  }

  ref_detectedObjects.Sort();
  if (ref_detectedObjects == m_LastDetectedObjects)
    return false;

  ref_detectedObjects.Swap(m_LastDetectedObjects);

  if (bPostChangeMsg)
  {
    plMsgSensorDetectedObjectsChanged msg;
    msg.m_DetectedObjects = m_LastDetectedObjects;
    pSensorOwner->PostEventMessage(msg, this, plTime::Zero(), plObjectMsgQueueType::PostAsync);
  }

  return true;
}

void plSensorComponent::UpdateSpatialCategory()
{
  if (!m_sSpatialCategory.IsEmpty())
  {
    m_SpatialCategory = plSpatialData::RegisterCategory(m_sSpatialCategory, plSpatialData::Flags::None);
  }
  else
  {
    m_SpatialCategory = plInvalidSpatialDataCategory;
  }
}

void plSensorComponent::UpdateScheduling()
{
  auto pModule = GetWorld()->GetOrCreateModule<plSensorWorldModule>();
  pModule->AddComponentToSchedule(this, m_UpdateRate);
}

void plSensorComponent::UpdateDebugInfo()
{
  auto pModule = GetWorld()->GetOrCreateModule<plSensorWorldModule>();
  if (IsActiveAndInitialized() && m_bShowDebugInfo)
  {
    pModule->AddComponentForDebugRendering(this);
  }
  else
  {
    pModule->RemoveComponentForDebugRendering(this);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plSensorSphereComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new plDefaultValueAttribute(10.0f), new plClampValueAttribute(0.0f, plVariant())),
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plSphereManipulatorAttribute("Radius"),
    new plSphereVisualizerAttribute("Radius", plColor::White, "Color"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plSensorSphereComponent::plSensorSphereComponent() = default;
plSensorSphereComponent::~plSensorSphereComponent() = default;

void plSensorSphereComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fRadius;
}

void plSensorSphereComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fRadius;
}

void plSensorSphereComponent::GetObjectsInSensorVolume(plDynamicArray<plGameObject*>& out_Objects) const
{
  const plGameObject* pOwner = GetOwner();

  const float scale = pOwner->GetGlobalTransformSimd().GetMaxScale();
  plBoundingSphere sphere = plBoundingSphere(pOwner->GetGlobalPosition(), m_fRadius * scale);

  plSpatialSystem::QueryParams params;
  params.m_uiCategoryBitmask = m_SpatialCategory.GetBitmask();

  plSimdMat4f toLocalSpace = pOwner->GetGlobalTransformSimd().GetAsMat4().GetInverse();
  plSimdFloat radiusSquared = m_fRadius * m_fRadius;

  GetWorld()->GetSpatialSystem()->FindObjectsInSphere(sphere, params, [&](plGameObject* pObject) {
    plSimdVec4f localSpacePos = toLocalSpace.TransformPosition(pObject->GetGlobalPositionSimd());
    const bool bInRadius = localSpacePos.GetLengthSquared<3>() <= radiusSquared;

    if (bInRadius)
    {
      out_Objects.PushBack(pObject);
    }

    return plVisitorExecution::Continue; });
}

void plSensorSphereComponent::DebugDrawSensorShape() const
{
  plBoundingSphere sphere = plBoundingSphere(plVec3::ZeroVector(), m_fRadius);
  plDebugRenderer::DrawLineSphere(GetWorld(), sphere, m_Color, GetOwner()->GetGlobalTransform());
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plSensorCylinderComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new plDefaultValueAttribute(10.0f), new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new plDefaultValueAttribute(10.0f), new plClampValueAttribute(0.0f, plVariant())),
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCylinderVisualizerAttribute(plBasisAxis::PositiveZ, "Height", "Radius", plColor::White, "Color"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plSensorCylinderComponent::plSensorCylinderComponent() = default;
plSensorCylinderComponent::~plSensorCylinderComponent() = default;

void plSensorCylinderComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fRadius;
  s << m_fHeight;
}

void plSensorCylinderComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fRadius;
  s >> m_fHeight;
}

void plSensorCylinderComponent::GetObjectsInSensorVolume(plDynamicArray<plGameObject*>& out_Objects) const
{
  const plGameObject* pOwner = GetOwner();

  const plVec3 scale = pOwner->GetGlobalScaling().Abs();
  const float xyScale = plMath::Max(scale.x, scale.y);

  const float sphereRadius = plVec2(m_fRadius * xyScale, m_fHeight * 0.5f * scale.z).GetLength();
  plBoundingSphere sphere = plBoundingSphere(pOwner->GetGlobalPosition(), sphereRadius);

  plSpatialSystem::QueryParams params;
  params.m_uiCategoryBitmask = m_SpatialCategory.GetBitmask();

  plSimdMat4f toLocalSpace = pOwner->GetGlobalTransformSimd().GetAsMat4().GetInverse();
  plSimdFloat radiusSquared = m_fRadius * m_fRadius;
  plSimdFloat halfHeight = m_fHeight * 0.5f;

  GetWorld()->GetSpatialSystem()->FindObjectsInSphere(sphere, params, [&](plGameObject* pObject) {
    plSimdVec4f localSpacePos = toLocalSpace.TransformPosition(pObject->GetGlobalPositionSimd());
    const bool bInRadius = localSpacePos.GetLengthSquared<2>() <= radiusSquared;
    const bool bInHeight = localSpacePos.Abs().z() <= halfHeight;

    if (bInRadius && bInHeight)
    {
      out_Objects.PushBack(pObject);
    }

    return plVisitorExecution::Continue; });
}

void plSensorCylinderComponent::DebugDrawSensorShape() const
{
  plTransform pt = GetOwner()->GetGlobalTransform();

  plQuat r;
  r.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(-90.0f));
  plTransform t = plTransform(plVec3(0, 0, -0.5f * m_fHeight * pt.m_vScale.z), r, plVec3(pt.m_vScale.z, pt.m_vScale.y, pt.m_vScale.x));

  pt.m_vScale.Set(1);
  t = pt * t;

  plColor solidColor = plColor::Black.WithAlpha(0.0f); // lines only
  plDebugRenderer::DrawCylinder(GetWorld(), m_fRadius, m_fRadius, m_fHeight, solidColor, m_Color, t);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plSensorConeComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("NearDistance", m_fNearDistance)->AddAttributes(new plDefaultValueAttribute(0.0f), new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_MEMBER_PROPERTY("FarDistance", m_fFarDistance)->AddAttributes(new plDefaultValueAttribute(10.0f), new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_MEMBER_PROPERTY("Angle", m_Angle)->AddAttributes(new plDefaultValueAttribute(plAngle::Degree(90.0f)), new plClampValueAttribute(0.0f, plAngle::Degree(180.0f))),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plSensorConeComponent::plSensorConeComponent() = default;
plSensorConeComponent::~plSensorConeComponent() = default;

void plSensorConeComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fNearDistance;
  s << m_fFarDistance;
  s << m_Angle;
}

void plSensorConeComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fNearDistance;
  s >> m_fFarDistance;
  s >> m_Angle;
}

void plSensorConeComponent::GetObjectsInSensorVolume(plDynamicArray<plGameObject*>& out_Objects) const
{
  const plGameObject* pOwner = GetOwner();

  const float scale = pOwner->GetGlobalTransformSimd().GetMaxScale();
  plBoundingSphere sphere = plBoundingSphere(pOwner->GetGlobalPosition(), m_fFarDistance * scale);

  plSpatialSystem::QueryParams params;
  params.m_uiCategoryBitmask = m_SpatialCategory.GetBitmask();

  plSimdMat4f toLocalSpace = pOwner->GetGlobalTransformSimd().GetAsMat4().GetInverse();
  const plSimdFloat nearSquared = m_fNearDistance * m_fNearDistance;
  const plSimdFloat farSquared = m_fFarDistance * m_fFarDistance;
  const plSimdFloat cosAngle = plMath::Cos(m_Angle * 0.5f);

  GetWorld()->GetSpatialSystem()->FindObjectsInSphere(sphere, params, [&](plGameObject* pObject) {
    plSimdVec4f localSpacePos = toLocalSpace.TransformPosition(pObject->GetGlobalPositionSimd());
    const plSimdFloat fDistanceSquared = localSpacePos.GetLengthSquared<3>();
    const bool bInDistance = fDistanceSquared >= nearSquared && fDistanceSquared <= farSquared;

    const plSimdVec4f normalizedPos = localSpacePos * fDistanceSquared.GetInvSqrt();
    const bool bInAngle = normalizedPos.x() >= cosAngle;

    if (bInDistance && bInAngle)
    {
      out_Objects.PushBack(pObject);
    }

    return plVisitorExecution::Continue; });
}

void plSensorConeComponent::DebugDrawSensorShape() const
{
  constexpr plUInt32 MIN_SEGMENTS = 3;
  constexpr plUInt32 MAX_SEGMENTS = 16;
  constexpr plUInt32 CIRCLE_SEGMENTS = MAX_SEGMENTS * 2;
  constexpr plUInt32 NUM_LINES = MAX_SEGMENTS * 4 + CIRCLE_SEGMENTS * 2 + 4;

  plDebugRenderer::Line lines[NUM_LINES];
  plUInt32 curLine = 0;

  const plUInt32 numSegments = plMath::Clamp(static_cast<plUInt32>(m_Angle / plAngle::Degree(180)) * MAX_SEGMENTS, MIN_SEGMENTS, MAX_SEGMENTS);
  const plAngle stepAngle = m_Angle / static_cast<float>(numSegments);
  const plAngle circleStepAngle = plAngle::Degree(360.0f / CIRCLE_SEGMENTS);

  for (plUInt32 i = 0; i < 2; ++i)
  {
    plAngle curAngle = m_Angle * -0.5f;

    plQuat q;
    float fX = plMath::Cos(curAngle);
    float fCircleRadius = plMath::Sin(curAngle);

    if (i == 0)
    {
      q.SetIdentity();
      fX *= m_fNearDistance;
      fCircleRadius *= m_fNearDistance;
    }
    else
    {
      q.SetFromAxisAndAngle(plVec3::UnitXAxis(), plAngle::Degree(90));
      fX *= m_fFarDistance;
      fCircleRadius *= m_fFarDistance;
    }

    for (plUInt32 s = 0; s < numSegments; ++s)
    {
      const plAngle nextAngle = curAngle + stepAngle;

      const float fCos1 = plMath::Cos(curAngle);
      const float fCos2 = plMath::Cos(nextAngle);

      const float fSin1 = plMath::Sin(curAngle);
      const float fSin2 = plMath::Sin(nextAngle);

      curAngle = nextAngle;

      const plVec3 p1 = q * plVec3(fCos1, fSin1, 0.0f);
      const plVec3 p2 = q * plVec3(fCos2, fSin2, 0.0f);

      lines[curLine].m_start = p1 * m_fNearDistance;
      lines[curLine].m_end = p2 * m_fNearDistance;
      ++curLine;

      lines[curLine].m_start = p1 * m_fFarDistance;
      lines[curLine].m_end = p2 * m_fFarDistance;
      ++curLine;

      if (s == 0)
      {
        lines[curLine].m_start = p1 * m_fNearDistance;
        lines[curLine].m_end = p1 * m_fFarDistance;
        ++curLine;
      }
      else if (s == numSegments - 1)
      {
        lines[curLine].m_start = p2 * m_fNearDistance;
        lines[curLine].m_end = p2 * m_fFarDistance;
        ++curLine;
      }
    }

    curAngle = plAngle::Degree(0.0f);
    for (plUInt32 s = 0; s < CIRCLE_SEGMENTS; ++s)
    {
      const plAngle nextAngle = curAngle + circleStepAngle;

      const float fCos1 = plMath::Cos(curAngle);
      const float fCos2 = plMath::Cos(nextAngle);

      const float fSin1 = plMath::Sin(curAngle);
      const float fSin2 = plMath::Sin(nextAngle);

      curAngle = nextAngle;

      const plVec3 p1 = plVec3(fX, fCos1 * fCircleRadius, fSin1 * fCircleRadius);
      const plVec3 p2 = plVec3(fX, fCos2 * fCircleRadius, fSin2 * fCircleRadius);

      lines[curLine].m_start = p1;
      lines[curLine].m_end = p2;
      ++curLine;
    }
  }

  PLASMA_ASSERT_DEV(curLine <= NUM_LINES, "");
  plDebugRenderer::DrawLines(GetWorld(), plMakeArrayPtr(lines, curLine), m_Color, GetOwner()->GetGlobalTransform());
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_IMPLEMENT_WORLD_MODULE(plSensorWorldModule);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSensorWorldModule, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSensorWorldModule::plSensorWorldModule(plWorld* pWorld)
  : plWorldModule(pWorld)
{
}

void plSensorWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto updateDesc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plSensorWorldModule::UpdateSensors, this);
    updateDesc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::Async;
    updateDesc.m_bOnlyUpdateWhenSimulating = true;

    RegisterUpdateFunction(updateDesc);
  }

  {
    auto updateDesc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plSensorWorldModule::DebugDrawSensors, this);
    updateDesc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PostTransform;

    RegisterUpdateFunction(updateDesc);
  }

  m_pPhysicsWorldModule = GetWorld()->GetOrCreateModule<plPhysicsWorldModuleInterface>();
}

void plSensorWorldModule::AddComponentToSchedule(plSensorComponent* pComponent, plUpdateRate::Enum updateRate)
{
  m_Scheduler.AddOrUpdateWork(pComponent->GetHandle(), plUpdateRate::GetInterval(updateRate));
}

void plSensorWorldModule::RemoveComponentToSchedule(plSensorComponent* pComponent)
{
  m_Scheduler.RemoveWork(pComponent->GetHandle());
}

void plSensorWorldModule::AddComponentForDebugRendering(plSensorComponent* pComponent)
{
  plComponentHandle hComponent = pComponent->GetHandle();
  if (m_DebugComponents.Contains(hComponent) == false)
  {
    m_DebugComponents.PushBack(hComponent);
  }
}

void plSensorWorldModule::RemoveComponentForDebugRendering(plSensorComponent* pComponent)
{
  m_DebugComponents.RemoveAndSwap(pComponent->GetHandle());
}

void plSensorWorldModule::UpdateSensors(const plWorldModule::UpdateContext& context)
{
  if (m_pPhysicsWorldModule == nullptr)
    return;

  const plTime deltaTime = GetWorld()->GetClock().GetTimeDiff();
  m_Scheduler.Update(deltaTime, [this](const plComponentHandle& hComponent, plTime deltaTime) {
    const plWorld* pWorld = GetWorld();
    const plSensorComponent* pSensorComponent = nullptr;
    PLASMA_VERIFY(pWorld->TryGetComponent(hComponent, pSensorComponent), "Invalid component handle");

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
    pSensorComponent->m_LastOccludedObjectPositions.Clear();
#endif

    m_ObjectsInSensorVolume.Clear();

    pSensorComponent->GetObjectsInSensorVolume(m_ObjectsInSensorVolume);
    const plGameObject* pSensorOwner = pSensorComponent->GetOwner();

    m_DetectedObjects.Clear();

    if (pSensorComponent->m_bTestVisibility)
    {
      const plVec3 rayStart = pSensorOwner->GetGlobalPosition();
      for (auto pObject : m_ObjectsInSensorVolume)
      {
        const plVec3 rayEnd = pObject->GetGlobalPosition();
        plVec3 rayDir = rayEnd - rayStart;
        const float fDistance = rayDir.GetLengthAndNormalize();

        plPhysicsCastResult hitResult;
        plPhysicsQueryParameters params(pSensorComponent->m_uiCollisionLayer);
        params.m_bIgnoreInitialOverlap = true;
        params.m_ShapeTypes = plPhysicsShapeType::Default;

        // TODO: probably best to expose the plPhysicsShapeType bitflags on the component
        params.m_ShapeTypes.Remove(plPhysicsShapeType::Rope);
        params.m_ShapeTypes.Remove(plPhysicsShapeType::Ragdoll);
        params.m_ShapeTypes.Remove(plPhysicsShapeType::Trigger);
        params.m_ShapeTypes.Remove(plPhysicsShapeType::Query);
        params.m_ShapeTypes.Remove(plPhysicsShapeType::Character);

        if (m_pPhysicsWorldModule->Raycast(hitResult, rayStart, rayDir, fDistance, params))
        {
          // hit something in between -> not visible
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
          pSensorComponent->m_LastOccludedObjectPositions.PushBack(rayEnd);
#endif

          continue;
        }

        m_DetectedObjects.PushBack(pObject->GetHandle());
      }
    }
    else
    {
      for (auto pObject : m_ObjectsInSensorVolume)
      {
        m_DetectedObjects.PushBack(pObject->GetHandle());
      }
    }

    m_DetectedObjects.Sort();
    if (m_DetectedObjects != pSensorComponent->m_LastDetectedObjects)
    {
      m_DetectedObjects.Swap(pSensorComponent->m_LastDetectedObjects);

      plMsgSensorDetectedObjectsChanged msg;
      msg.m_DetectedObjects = pSensorComponent->m_LastDetectedObjects;

      pSensorOwner->PostEventMessage(msg, pSensorComponent, plTime::Zero(), plObjectMsgQueueType::PostAsync);
    } });
}

void plSensorWorldModule::DebugDrawSensors(const plWorldModule::UpdateContext& context)
{
  plHybridArray<plDebugRenderer::Line, 256> lines;
  const plWorld* pWorld = GetWorld();

  for (plComponentHandle hComponent : m_DebugComponents)
  {
    lines.Clear();

    const plSensorComponent* pSensorComponent = nullptr;
    PLASMA_VERIFY(pWorld->TryGetComponent(hComponent, pSensorComponent), "Invalid component handle");

    pSensorComponent->DebugDrawSensorShape();

    const plVec3 sensorPos = pSensorComponent->GetOwner()->GetGlobalPosition();
    for (plGameObjectHandle hObject : pSensorComponent->m_LastDetectedObjects)
    {
      const plGameObject* pObject = nullptr;
      if (pWorld->TryGetObject(hObject, pObject) == false)
        continue;

      lines.PushBack({sensorPos, pObject->GetGlobalPosition(), plColor::Lime});
    }

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
    for (const plVec3& occludedPos : pSensorComponent->m_LastOccludedObjectPositions)
    {
      lines.PushBack({sensorPos, occludedPos, plColor::Red});
    }
#endif

    plDebugRenderer::DrawLines(pWorld, lines, plColor::White);
  }
}
