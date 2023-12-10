#include <Core/CorePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>

namespace
{
  static plVariantArray GetDefaultTags()
  {
    plVariantArray value(plStaticAllocatorWrapper::GetAllocator());
    value.PushBack("CastShadow");
    return value;
  }
} // namespace

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plGameObject, plNoBase, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Active", GetActiveFlag, SetActiveFlag)->AddAttributes(new plDefaultValueAttribute(true), new plGroupNextAttribute()),
    PLASMA_ACCESSOR_PROPERTY("Name", GetNameInternal, SetNameInternal),
    PLASMA_ACCESSOR_PROPERTY("GlobalKey", GetGlobalKeyInternal, SetGlobalKeyInternal)->AddAttributes(new plGroupNextAttribute()),
    PLASMA_ENUM_ACCESSOR_PROPERTY("Mode", plObjectMode, Reflection_GetMode, Reflection_SetMode),
    PLASMA_ACCESSOR_PROPERTY("LocalPosition", GetLocalPosition, SetLocalPosition)->AddAttributes(new plSuffixAttribute(" m")),
    PLASMA_ACCESSOR_PROPERTY("LocalRotation", GetLocalRotation, SetLocalRotation),
    PLASMA_ACCESSOR_PROPERTY("LocalScaling", GetLocalScaling, SetLocalScaling)->AddAttributes(new plDefaultValueAttribute(plVec3(1.0f, 1.0f, 1.0f))),
    PLASMA_ACCESSOR_PROPERTY("LocalUniformScaling", GetLocalUniformScaling, SetLocalUniformScaling)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_SET_MEMBER_PROPERTY("Tags", m_Tags)->AddAttributes(new plTagSetWidgetAttribute("Default"), new plDefaultValueAttribute(GetDefaultTags())),
    PLASMA_SET_ACCESSOR_PROPERTY("Children", Reflection_GetChildren, Reflection_AddChild, Reflection_DetachChild)->AddFlags(plPropertyFlags::PointerOwner | plPropertyFlags::Hidden),
    PLASMA_SET_ACCESSOR_PROPERTY("Components", Reflection_GetComponents, Reflection_AddComponent, Reflection_RemoveComponent)->AddFlags(plPropertyFlags::PointerOwner),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(IsActive),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(SetCreatedByPrefab),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(WasCreatedByPrefab),

    PLASMA_SCRIPT_FUNCTION_PROPERTY(HasName, In, "Name"),

    PLASMA_SCRIPT_FUNCTION_PROPERTY(Reflection_GetParent),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(FindChildByName, In, "Name", In, "Recursive")->AddFlags(plPropertyFlags::Const),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(FindChildByPath, In, "Path")->AddFlags(plPropertyFlags::Const),

    PLASMA_SCRIPT_FUNCTION_PROPERTY(Reflection_SetGlobalPosition, In, "Position"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetGlobalPosition),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Reflection_SetGlobalRotation, In, "Rotation"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetGlobalRotation),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Reflection_SetGlobalScaling, In, "Scaling"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetGlobalScaling),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Reflection_SetGlobalTransform, In, "Transform"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetGlobalTransform),

    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetGlobalDirForwards),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetGlobalDirRight),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetGlobalDirUp),

#if PLASMA_ENABLED(PLASMA_GAMEOBJECT_VELOCITY)
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetLinearVelocity),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetAngularVelocity),
#endif

    PLASMA_SCRIPT_FUNCTION_PROPERTY(SetTeamID, In, "Id"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetTeamID),    
  }
  PLASMA_END_FUNCTIONS;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgDeleteGameObject, OnMsgDeleteGameObject),
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

void plGameObject::Reflection_AddChild(plGameObject* pChild)
{
  if (IsDynamic())
  {
    pChild->MakeDynamic();
  }

  AddChild(pChild->GetHandle(), TransformPreservation::PreserveLocal);

  // Check whether the child object was only dynamic because of its old parent
  // If that's the case make it static now.
  pChild->ConditionalMakeStatic();
}

void plGameObject::Reflection_DetachChild(plGameObject* pChild)
{
  DetachChild(pChild->GetHandle(), TransformPreservation::PreserveLocal);

  // The child object is now a top level object, check whether it should be static now.
  pChild->ConditionalMakeStatic();
}

plHybridArray<plGameObject*, 8> plGameObject::Reflection_GetChildren() const
{
  ConstChildIterator it = GetChildren();

  plHybridArray<plGameObject*, 8> all;
  all.Reserve(GetChildCount());

  while (it.IsValid())
  {
    all.PushBack(it.m_pObject);
    ++it;
  }

  return all;
}

void plGameObject::Reflection_AddComponent(plComponent* pComponent)
{
  if (pComponent == nullptr)
    return;

  if (pComponent->IsDynamic())
  {
    MakeDynamic();
  }

  AddComponent(pComponent);
}

void plGameObject::Reflection_RemoveComponent(plComponent* pComponent)
{
  if (pComponent == nullptr)
    return;

  /*Don't call RemoveComponent here, Component is automatically removed when deleted.*/

  if (pComponent->IsDynamic())
  {
    ConditionalMakeStatic(pComponent);
  }
}

plHybridArray<plComponent*, plGameObject::NUM_INPLACE_COMPONENTS> plGameObject::Reflection_GetComponents() const
{
  return plHybridArray<plComponent*, plGameObject::NUM_INPLACE_COMPONENTS>(m_Components);
}

plObjectMode::Enum plGameObject::Reflection_GetMode() const
{
  return m_Flags.IsSet(plObjectFlags::ForceDynamic) ? plObjectMode::ForceDynamic : plObjectMode::Automatic;
}

void plGameObject::Reflection_SetMode(plObjectMode::Enum mode)
{
  if (Reflection_GetMode() == mode)
  {
    return;
  }

  if (mode == plObjectMode::ForceDynamic)
  {
    m_Flags.Add(plObjectFlags::ForceDynamic);
    MakeDynamic();
  }
  else
  {
    m_Flags.Remove(plObjectFlags::ForceDynamic);
    ConditionalMakeStatic();
  }
}

plGameObject* plGameObject::Reflection_GetParent() const
{
  return GetWorld()->GetObjectUnchecked(m_uiParentIndex);
}

void plGameObject::Reflection_SetGlobalPosition(const plVec3& vPosition)
{
  SetGlobalPosition(vPosition);
}

void plGameObject::Reflection_SetGlobalRotation(const plQuat& qRotation)
{
  SetGlobalRotation(qRotation);
}

void plGameObject::Reflection_SetGlobalScaling(const plVec3& vScaling)
{
  SetGlobalScaling(vScaling);
}

void plGameObject::Reflection_SetGlobalTransform(const plTransform& transform)
{
  SetGlobalTransform(transform);
}

bool plGameObject::DetermineDynamicMode(plComponent* pComponentToIgnore /*= nullptr*/) const
{
  if (m_Flags.IsSet(plObjectFlags::ForceDynamic))
  {
    return true;
  }

  const plGameObject* pParent = GetParent();
  if (pParent != nullptr && pParent->IsDynamic())
  {
    return true;
  }

  for (auto pComponent : m_Components)
  {
    if (pComponent != pComponentToIgnore && pComponent->IsDynamic())
    {
      return true;
    }
  }

  return false;
}

void plGameObject::ConditionalMakeStatic(plComponent* pComponentToIgnore /*= nullptr*/)
{
  if (!DetermineDynamicMode(pComponentToIgnore))
  {
    MakeStaticInternal();

    for (auto it = GetChildren(); it.IsValid(); ++it)
    {
      it->ConditionalMakeStatic();
    }
  }
}

void plGameObject::MakeStaticInternal()
{
  if (IsStatic())
  {
    return;
  }

  m_Flags.Remove(plObjectFlags::Dynamic);

  GetWorld()->RecreateHierarchyData(this, true);
}

void plGameObject::UpdateGlobalTransformAndBoundsRecursive()
{
  if (IsStatic() && GetWorld()->ReportErrorWhenStaticObjectMoves())
  {
    plLog::Error("Static object '{0}' was moved during runtime.", GetName());
  }

  plSimdTransform oldGlobalTransform = GetGlobalTransformSimd();

  m_pTransformationData->UpdateGlobalTransformNonRecursive(GetWorld()->GetUpdateCounter());

  if (plSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem())
  {
    m_pTransformationData->UpdateGlobalBoundsAndSpatialData(*pSpatialSystem);
  }
  else
  {
    m_pTransformationData->UpdateGlobalBounds();
  }

  if (IsStatic() && m_Flags.IsSet(plObjectFlags::StaticTransformChangesNotifications) && oldGlobalTransform != GetGlobalTransformSimd())
  {
    plMsgTransformChanged msg;
    msg.m_OldGlobalTransform = plSimdConversion::ToTransform(oldGlobalTransform);
    msg.m_NewGlobalTransform = GetGlobalTransform();

    SendMessage(msg);
  }

  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    it->UpdateGlobalTransformAndBoundsRecursive();
  }
}

void plGameObject::UpdateLastGlobalTransform()
{
  m_pTransformationData->UpdateLastGlobalTransform(GetWorld()->GetUpdateCounter());
}

void plGameObject::ConstChildIterator::Next()
{
  m_pObject = m_pWorld->GetObjectUnchecked(m_pObject->m_uiNextSiblingIndex);
}

plGameObject::~plGameObject()
{
  // Since we are using the small array base class for components we have to cleanup ourself with the correct allocator.
  m_Components.Clear();
  m_Components.Compact(GetWorld()->GetAllocator());
}

void plGameObject::operator=(const plGameObject& other)
{
  PLASMA_ASSERT_DEV(m_InternalId.m_WorldIndex == other.m_InternalId.m_WorldIndex, "Cannot copy between worlds.");

  m_InternalId = other.m_InternalId;
  m_Flags = other.m_Flags;
  m_sName = other.m_sName;

  m_uiParentIndex = other.m_uiParentIndex;
  m_uiFirstChildIndex = other.m_uiFirstChildIndex;
  m_uiLastChildIndex = other.m_uiLastChildIndex;

  m_uiNextSiblingIndex = other.m_uiNextSiblingIndex;
  m_uiPrevSiblingIndex = other.m_uiPrevSiblingIndex;
  m_uiChildCount = other.m_uiChildCount;

  m_uiTeamID = other.m_uiTeamID;

  m_uiHierarchyLevel = other.m_uiHierarchyLevel;
  m_pTransformationData = other.m_pTransformationData;
  m_pTransformationData->m_pObject = this;

  if (!m_pTransformationData->m_hSpatialData.IsInvalidated())
  {
    plSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem();
    pSpatialSystem->UpdateSpatialDataObject(m_pTransformationData->m_hSpatialData, this);
  }

  m_Components.CopyFrom(other.m_Components, GetWorld()->GetAllocator());
  for (plComponent* pComponent : m_Components)
  {
    PLASMA_ASSERT_DEV(pComponent->m_pOwner == &other, "");
    pComponent->m_pOwner = this;
  }

  m_Tags = other.m_Tags;
}

void plGameObject::MakeDynamic()
{
  if (IsDynamic())
  {
    return;
  }

  m_Flags.Add(plObjectFlags::Dynamic);

  GetWorld()->RecreateHierarchyData(this, false);

  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    it->MakeDynamic();
  }
}

void plGameObject::MakeStatic()
{
  PLASMA_ASSERT_DEV(!DetermineDynamicMode(), "This object can't be static because it has a dynamic parent or dynamic component(s) attached.");

  MakeStaticInternal();
}

void plGameObject::SetActiveFlag(bool bEnabled)
{
  if (m_Flags.IsSet(plObjectFlags::ActiveFlag) == bEnabled)
    return;

  m_Flags.AddOrRemove(plObjectFlags::ActiveFlag, bEnabled);

  UpdateActiveState(GetParent() == nullptr ? true : GetParent()->IsActive());
}

void plGameObject::UpdateActiveState(bool bParentActive)
{
  const bool bSelfActive = bParentActive && m_Flags.IsSet(plObjectFlags::ActiveFlag);

  if (bSelfActive != m_Flags.IsSet(plObjectFlags::ActiveState))
  {
    m_Flags.AddOrRemove(plObjectFlags::ActiveState, bSelfActive);

    for (plUInt32 i = 0; i < m_Components.GetCount(); ++i)
    {
      m_Components[i]->UpdateActiveState(bSelfActive);
    }

    // recursively update all children
    for (auto it = GetChildren(); it.IsValid(); ++it)
    {
      it->UpdateActiveState(bSelfActive);
    }
  }
}

void plGameObject::SetGlobalKey(const plHashedString& sName)
{
  GetWorld()->SetObjectGlobalKey(this, sName);
}

plStringView plGameObject::GetGlobalKey() const
{
  return GetWorld()->GetObjectGlobalKey(this);
}

const char* plGameObject::GetGlobalKeyInternal() const
{
  return GetWorld()->GetObjectGlobalKey(this).GetStartPointer(); // we know that it's zero terminated
}

void plGameObject::SetParent(const plGameObjectHandle& hParent, plGameObject::TransformPreservation preserve)
{
  plWorld* pWorld = GetWorld();

  plGameObject* pParent = nullptr;
  bool _ = pWorld->TryGetObject(hParent, pParent);
  PLASMA_IGNORE_UNUSED(_);
  pWorld->SetParent(this, pParent, preserve);
}

plGameObject* plGameObject::GetParent()
{
  return GetWorld()->GetObjectUnchecked(m_uiParentIndex);
}

const plGameObject* plGameObject::GetParent() const
{
  return GetWorld()->GetObjectUnchecked(m_uiParentIndex);
}

void plGameObject::AddChild(const plGameObjectHandle& hChild, plGameObject::TransformPreservation preserve)
{
  plWorld* pWorld = GetWorld();

  plGameObject* pChild = nullptr;
  if (pWorld->TryGetObject(hChild, pChild))
  {
    pWorld->SetParent(pChild, this, preserve);
  }
}

void plGameObject::DetachChild(const plGameObjectHandle& hChild, plGameObject::TransformPreservation preserve)
{
  plWorld* pWorld = GetWorld();

  plGameObject* pChild = nullptr;
  if (pWorld->TryGetObject(hChild, pChild))
  {
    if (pChild->GetParent() == this)
    {
      pWorld->SetParent(pChild, nullptr, preserve);
    }
  }
}

plGameObject::ChildIterator plGameObject::GetChildren()
{
  plWorld* pWorld = GetWorld();
  return ChildIterator(pWorld->GetObjectUnchecked(m_uiFirstChildIndex), pWorld);
}

plGameObject::ConstChildIterator plGameObject::GetChildren() const
{
  const plWorld* pWorld = GetWorld();
  return ConstChildIterator(pWorld->GetObjectUnchecked(m_uiFirstChildIndex), pWorld);
}

plGameObject* plGameObject::FindChildByName(const plTempHashedString& sName, bool bRecursive /*= true*/)
{
  /// \test Needs a unit test

  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName == sName)
    {
      return &(*it);
    }
  }

  if (bRecursive)
  {
    for (auto it = GetChildren(); it.IsValid(); ++it)
    {
      plGameObject* pChild = it->FindChildByName(sName, bRecursive);

      if (pChild != nullptr)
        return pChild;
    }
  }

  return nullptr;
}

plGameObject* plGameObject::FindChildByPath(plStringView sPath)
{
  /// \test Needs a unit test

  if (sPath.IsEmpty())
    return this;

  const char* szSep = sPath.FindSubString("/");
  plUInt64 uiNameHash = 0;

  if (szSep == nullptr)
    uiNameHash = plHashingUtils::StringHash(sPath);
  else
    uiNameHash = plHashingUtils::StringHash(plStringView(sPath.GetStartPointer(), szSep));

  plGameObject* pNextChild = FindChildByName(plTempHashedString(uiNameHash), false);

  if (szSep == nullptr || pNextChild == nullptr)
    return pNextChild;

  return pNextChild->FindChildByPath(plStringView(szSep + 1, sPath.GetEndPointer()));
}


plGameObject* plGameObject::SearchForChildByNameSequence(plStringView sObjectSequence, const plRTTI* pExpectedComponent /*= nullptr*/)
{
  /// \test Needs a unit test

  if (sObjectSequence.IsEmpty())
  {
    // in case we are searching for a specific component type, verify that it exists on this object
    if (pExpectedComponent != nullptr)
    {
      plComponent* pComp = nullptr;
      if (!TryGetComponentOfBaseType(pExpectedComponent, pComp))
        return nullptr;
    }

    return this;
  }

  const char* szSep = sObjectSequence.FindSubString("/");
  plStringView sNextSequence;
  plUInt64 uiNameHash = 0;

  if (szSep == nullptr)
  {
    uiNameHash = plHashingUtils::StringHash(sObjectSequence);
  }
  else
  {
    uiNameHash = plHashingUtils::StringHash(plStringView(sObjectSequence.GetStartPointer(), szSep));
    sNextSequence = plStringView(szSep + 1, sObjectSequence.GetEndPointer());
  }

  const plTempHashedString name(uiNameHash);

  // first go through all direct children an see if any of them actually matches the current name
  // if so, continue the recursion from there and give them the remaining search path to continue
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName == name)
    {
      plGameObject* res = it->SearchForChildByNameSequence(sNextSequence, pExpectedComponent);
      if (res != nullptr)
        return res;
    }
  }

  // if no direct child fulfilled the requirements, just recurse with the full name sequence
  // however, we can skip any child that already fulfilled the next sequence name,
  // because that's definitely a lost cause
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName != name)
    {
      plGameObject* res = it->SearchForChildByNameSequence(sObjectSequence, pExpectedComponent);
      if (res != nullptr)
        return res;
    }
  }

  return nullptr;
}


void plGameObject::SearchForChildrenByNameSequence(plStringView sObjectSequence, const plRTTI* pExpectedComponent, plHybridArray<plGameObject*, 8>& out_objects)
{
  /// \test Needs a unit test

  if (sObjectSequence.IsEmpty())
  {
    // in case we are searching for a specific component type, verify that it exists on this object
    if (pExpectedComponent != nullptr)
    {
      plComponent* pComp = nullptr;
      if (!TryGetComponentOfBaseType(pExpectedComponent, pComp))
        return;
    }

    out_objects.PushBack(this);
    return;
  }

  const char* szSep = sObjectSequence.FindSubString("/");
  plStringView sNextSequence;
  plUInt64 uiNameHash = 0;

  if (szSep == nullptr)
  {
    uiNameHash = plHashingUtils::StringHash(sObjectSequence);
  }
  else
  {
    uiNameHash = plHashingUtils::StringHash(plStringView(sObjectSequence.GetStartPointer(), szSep));
    sNextSequence = plStringView(szSep + 1, sObjectSequence.GetEndPointer());
  }

  const plTempHashedString name(uiNameHash);

  // first go through all direct children an see if any of them actually matches the current name
  // if so, continue the recursion from there and give them the remaining search path to continue
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName == name)
    {
      it->SearchForChildrenByNameSequence(sNextSequence, pExpectedComponent, out_objects);
    }
  }

  // if no direct child fulfilled the requirements, just recurse with the full name sequence
  // however, we can skip any child that already fulfilled the next sequence name,
  // because that's definitely a lost cause
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName != name) // TODO: in this function it is actually debatable whether to skip these or not
    {
      it->SearchForChildrenByNameSequence(sObjectSequence, pExpectedComponent, out_objects);
    }
  }
}

plWorld* plGameObject::GetWorld()
{
  return plWorld::GetWorld(m_InternalId.m_WorldIndex);
}

const plWorld* plGameObject::GetWorld() const
{
  return plWorld::GetWorld(m_InternalId.m_WorldIndex);
}

plVec3 plGameObject::GetGlobalDirForwards() const
{
  plCoordinateSystem coordinateSystem;
  GetWorld()->GetCoordinateSystem(GetGlobalPosition(), coordinateSystem);

  return GetGlobalRotation() * coordinateSystem.m_vForwardDir;
}

plVec3 plGameObject::GetGlobalDirRight() const
{
  plCoordinateSystem coordinateSystem;
  GetWorld()->GetCoordinateSystem(GetGlobalPosition(), coordinateSystem);

  return GetGlobalRotation() * coordinateSystem.m_vRightDir;
}

plVec3 plGameObject::GetGlobalDirUp() const
{
  plCoordinateSystem coordinateSystem;
  GetWorld()->GetCoordinateSystem(GetGlobalPosition(), coordinateSystem);

  return GetGlobalRotation() * coordinateSystem.m_vUpDir;
}

#if PLASMA_ENABLED(PLASMA_GAMEOBJECT_VELOCITY)
void plGameObject::SetLastGlobalTransform(const plSimdTransform& transform)
{
  m_pTransformationData->m_lastGlobalTransform = transform;
  m_pTransformationData->m_uiLastGlobalTransformUpdateCounter = GetWorld()->GetUpdateCounter();
}

plVec3 plGameObject::GetLinearVelocity() const
{
  const plSimdFloat invDeltaSeconds = GetWorld()->GetInvDeltaSeconds();
  const plSimdVec4f linearVelocity = (m_pTransformationData->m_globalTransform.m_Position - m_pTransformationData->m_lastGlobalTransform.m_Position) * invDeltaSeconds;
  return plSimdConversion::ToVec3(linearVelocity);
}

plVec3 plGameObject::GetAngularVelocity() const
{
  const plSimdFloat invDeltaSeconds = GetWorld()->GetInvDeltaSeconds();
  const plSimdQuat q = m_pTransformationData->m_globalTransform.m_Rotation * -m_pTransformationData->m_lastGlobalTransform.m_Rotation;
  plSimdVec4f angularVelocity = plSimdVec4f::MakeZero();

  plSimdVec4f axis;
  plSimdFloat angle;
  if (q.GetRotationAxisAndAngle(axis, angle).Succeeded())
  {
    angularVelocity = axis * (angle * invDeltaSeconds);
  }
  return plSimdConversion::ToVec3(angularVelocity);
}
#endif

void plGameObject::UpdateGlobalTransform()
{
  m_pTransformationData->UpdateGlobalTransformRecursive(GetWorld()->GetUpdateCounter());
}

void plGameObject::UpdateLocalBounds()
{
  plMsgUpdateLocalBounds msg;
  msg.m_ResultingLocalBounds = plBoundingBoxSphere::MakeInvalid();

  SendMessage(msg);

  const bool bIsAlwaysVisible = m_pTransformationData->m_localBounds.m_BoxHalfExtents.w() != plSimdFloat::MakeZero();
  bool bRecreateSpatialData = false;

  if (m_pTransformationData->m_hSpatialData.IsInvalidated() == false)
  {
    // force spatial data re-creation if categories have changed
    bRecreateSpatialData |= m_pTransformationData->m_uiSpatialDataCategoryBitmask != msg.m_uiSpatialDataCategoryBitmask;

    // force spatial data re-creation if always visible flag has changed
    bRecreateSpatialData |= bIsAlwaysVisible != msg.m_bAlwaysVisible;

    // delete old spatial data if bounds are now invalid
    bRecreateSpatialData |= msg.m_bAlwaysVisible == false && msg.m_ResultingLocalBounds.IsValid() == false;
  }

  m_pTransformationData->m_localBounds = plSimdConversion::ToBBoxSphere(msg.m_ResultingLocalBounds);
  m_pTransformationData->m_localBounds.m_BoxHalfExtents.SetW(msg.m_bAlwaysVisible ? 1.0f : 0.0f);
  m_pTransformationData->m_uiSpatialDataCategoryBitmask = msg.m_uiSpatialDataCategoryBitmask;

  plSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem();
  if (pSpatialSystem != nullptr && (bRecreateSpatialData || m_pTransformationData->m_hSpatialData.IsInvalidated()))
  {
    m_pTransformationData->RecreateSpatialData(*pSpatialSystem);
  }

  if (IsStatic())
  {
    m_pTransformationData->UpdateGlobalBounds(pSpatialSystem);
  }
}

void plGameObject::UpdateGlobalTransformAndBounds()
{
  m_pTransformationData->UpdateGlobalTransformRecursive(GetWorld()->GetUpdateCounter());
  m_pTransformationData->UpdateGlobalBounds(GetWorld()->GetSpatialSystem());
}

void plGameObject::UpdateGlobalBounds()
{
  m_pTransformationData->UpdateGlobalBounds(GetWorld()->GetSpatialSystem());
}

bool plGameObject::TryGetComponentOfBaseType(const plRTTI* pType, plComponent*& out_pComponent)
{
  for (plUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    plComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf(pType))
    {
      out_pComponent = pComponent;
      return true;
    }
  }

  out_pComponent = nullptr;
  return false;
}

bool plGameObject::TryGetComponentOfBaseType(const plRTTI* pType, const plComponent*& out_pComponent) const
{
  for (plUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    plComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf(pType))
    {
      out_pComponent = pComponent;
      return true;
    }
  }

  out_pComponent = nullptr;
  return false;
}


void plGameObject::TryGetComponentsOfBaseType(const plRTTI* pType, plDynamicArray<plComponent*>& out_components)
{
  out_components.Clear();

  for (plUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    plComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf(pType))
    {
      out_components.PushBack(pComponent);
    }
  }
}

void plGameObject::TryGetComponentsOfBaseType(const plRTTI* pType, plDynamicArray<const plComponent*>& out_components) const
{
  out_components.Clear();

  for (plUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    plComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf(pType))
    {
      out_components.PushBack(pComponent);
    }
  }
}

void plGameObject::SetTeamID(plUInt16 uiId)
{
  m_uiTeamID = uiId;

  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    it->SetTeamID(uiId);
  }
}

plVisibilityState plGameObject::GetVisibilityState(plUInt32 uiNumFramesBeforeInvisible) const
{
  if (!m_pTransformationData->m_hSpatialData.IsInvalidated())
  {
    const plSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem();
    return pSpatialSystem->GetVisibilityState(m_pTransformationData->m_hSpatialData, uiNumFramesBeforeInvisible);
  }

  return plVisibilityState::Direct;
}

void plGameObject::OnMsgDeleteGameObject(plMsgDeleteGameObject& msg)
{
  GetWorld()->DeleteObjectNow(GetHandle(), msg.m_bDeleteEmptyParents);
}

void plGameObject::AddComponent(plComponent* pComponent)
{
  PLASMA_ASSERT_DEV(pComponent->m_pOwner == nullptr, "Component must not be added twice.");
  PLASMA_ASSERT_DEV(IsDynamic() || !pComponent->IsDynamic(), "Cannot attach a dynamic component to a static object. Call MakeDynamic() first.");

  pComponent->m_pOwner = this;
  m_Components.PushBack(pComponent, GetWorld()->GetAllocator());
  m_Components.GetUserData<ComponentUserData>().m_uiVersion++;

  pComponent->UpdateActiveState(IsActive());

  if (m_Flags.IsSet(plObjectFlags::ComponentChangesNotifications))
  {
    plMsgComponentsChanged msg;
    msg.m_Type = plMsgComponentsChanged::Type::ComponentAdded;
    msg.m_hOwner = GetHandle();
    msg.m_hComponent = pComponent->GetHandle();

    SendNotificationMessage(msg);
  }
}

void plGameObject::RemoveComponent(plComponent* pComponent)
{
  plUInt32 uiIndex = m_Components.IndexOf(pComponent);
  PLASMA_ASSERT_DEV(uiIndex != plInvalidIndex, "Component not found");

  pComponent->m_pOwner = nullptr;
  m_Components.RemoveAtAndSwap(uiIndex);
  m_Components.GetUserData<ComponentUserData>().m_uiVersion++;

  if (m_Flags.IsSet(plObjectFlags::ComponentChangesNotifications))
  {
    plMsgComponentsChanged msg;
    msg.m_Type = plMsgComponentsChanged::Type::ComponentRemoved;
    msg.m_hOwner = GetHandle();
    msg.m_hComponent = pComponent->GetHandle();

    SendNotificationMessage(msg);
  }
}

bool plGameObject::SendMessageInternal(plMessage& msg, bool bWasPostedMsg)
{
  bool bSentToAny = false;

  const plRTTI* pRtti = plGetStaticRTTI<plGameObject>();
  bSentToAny |= pRtti->DispatchMessage(this, msg);

  for (plUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    plComponent* pComponent = m_Components[i];
    bSentToAny |= pComponent->SendMessageInternal(msg, bWasPostedMsg);
  }

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  if (!bSentToAny && msg.GetDebugMessageRouting())
  {
    plLog::Warning("plGameObject::SendMessage: None of the target object's components had a handler for messages of type {0}.", msg.GetId());
  }
#endif

  return bSentToAny;
}

bool plGameObject::SendMessageInternal(plMessage& msg, bool bWasPostedMsg) const
{
  bool bSentToAny = false;

  const plRTTI* pRtti = plGetStaticRTTI<plGameObject>();
  bSentToAny |= pRtti->DispatchMessage(this, msg);

  for (plUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    // forward only to 'const' message handlers
    const plComponent* pComponent = m_Components[i];
    bSentToAny |= pComponent->SendMessageInternal(msg, bWasPostedMsg);
  }

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  if (!bSentToAny && msg.GetDebugMessageRouting())
  {
    plLog::Warning("plGameObject::SendMessage (const): None of the target object's components had a handler for messages of type {0}.", msg.GetId());
  }
#endif

  return bSentToAny;
}

bool plGameObject::SendMessageRecursiveInternal(plMessage& msg, bool bWasPostedMsg)
{
  bool bSentToAny = false;

  const plRTTI* pRtti = plGetStaticRTTI<plGameObject>();
  bSentToAny |= pRtti->DispatchMessage(this, msg);

  for (plUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    plComponent* pComponent = m_Components[i];
    bSentToAny |= pComponent->SendMessageInternal(msg, bWasPostedMsg);
  }

  for (auto childIt = GetChildren(); childIt.IsValid(); ++childIt)
  {
    bSentToAny |= childIt->SendMessageRecursiveInternal(msg, bWasPostedMsg);
  }

  // should only be evaluated at the top function call
  // #if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  //  if (!bSentToAny && msg.GetDebugMessageRouting())
  //  {
  //    plLog::Warning("plGameObject::SendMessageRecursive: None of the target object's components had a handler for messages of type {0}.",
  //    msg.GetId());
  //  }
  // #endif
  // #
  return bSentToAny;
}

bool plGameObject::SendMessageRecursiveInternal(plMessage& msg, bool bWasPostedMsg) const
{
  bool bSentToAny = false;

  const plRTTI* pRtti = plGetStaticRTTI<plGameObject>();
  bSentToAny |= pRtti->DispatchMessage(this, msg);

  for (plUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    // forward only to 'const' message handlers
    const plComponent* pComponent = m_Components[i];
    bSentToAny |= pComponent->SendMessageInternal(msg, bWasPostedMsg);
  }

  for (auto childIt = GetChildren(); childIt.IsValid(); ++childIt)
  {
    bSentToAny |= childIt->SendMessageRecursiveInternal(msg, bWasPostedMsg);
  }

  // should only be evaluated at the top function call
  // #if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  //  if (!bSentToAny && msg.GetDebugMessageRouting())
  //  {
  //    plLog::Warning("plGameObject::SendMessageRecursive(const): None of the target object's components had a handler for messages of type
  //    {0}.", msg.GetId());
  //  }
  // #endif
  // #
  return bSentToAny;
}

void plGameObject::PostMessage(const plMessage& msg, plTime delay, plObjectMsgQueueType::Enum queueType) const
{
  GetWorld()->PostMessage(GetHandle(), msg, delay, queueType);
}

void plGameObject::PostMessageRecursive(const plMessage& msg, plTime delay, plObjectMsgQueueType::Enum queueType) const
{
  GetWorld()->PostMessageRecursive(GetHandle(), msg, delay, queueType);
}

bool plGameObject::SendEventMessage(plMessage& ref_msg, const plComponent* pSenderComponent)
{
  if (auto pEventMsg = plDynamicCast<plEventMessage*>(&ref_msg))
  {
    pEventMsg->FillFromSenderComponent(pSenderComponent);
  }

  plHybridArray<plComponent*, 4> eventMsgHandlers;
  GetWorld()->FindEventMsgHandlers(ref_msg, this, eventMsgHandlers);

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  if (ref_msg.GetDebugMessageRouting())
  {
    if (eventMsgHandlers.IsEmpty())
    {
      plLog::Warning("plGameObject::SendEventMessage: None of the target object's components had a handler for messages of type {0}.", ref_msg.GetId());
    }
  }
#endif

  bool bResult = false;
  for (auto pEventMsgHandler : eventMsgHandlers)
  {
    bResult |= pEventMsgHandler->SendMessage(ref_msg);
  }
  return bResult;
}

bool plGameObject::SendEventMessage(plMessage& ref_msg, const plComponent* pSenderComponent) const
{
  if (auto pEventMsg = plDynamicCast<plEventMessage*>(&ref_msg))
  {
    pEventMsg->FillFromSenderComponent(pSenderComponent);
  }

  plHybridArray<const plComponent*, 4> eventMsgHandlers;
  GetWorld()->FindEventMsgHandlers(ref_msg, this, eventMsgHandlers);

  bool bResult = false;
  for (auto pEventMsgHandler : eventMsgHandlers)
  {
    bResult |= pEventMsgHandler->SendMessage(ref_msg);
  }
  return bResult;
}

void plGameObject::PostEventMessage(plMessage& ref_msg, const plComponent* pSenderComponent, plTime delay, plObjectMsgQueueType::Enum queueType) const
{
  if (auto pEventMsg = plDynamicCast<plEventMessage*>(&ref_msg))
  {
    pEventMsg->FillFromSenderComponent(pSenderComponent);
  }

  plHybridArray<const plComponent*, 4> eventMsgHandlers;
  GetWorld()->FindEventMsgHandlers(ref_msg, this, eventMsgHandlers);

  for (auto pEventMsgHandler : eventMsgHandlers)
  {
    pEventMsgHandler->PostMessage(ref_msg, delay, queueType);
  }
}

void plGameObject::SetTags(const plTagSet& tags)
{
  if (plSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem())
  {
    if (m_Tags != tags)
    {
      m_Tags = tags;
      m_pTransformationData->RecreateSpatialData(*pSpatialSystem);
    }
  }
  else
  {
    m_Tags = tags;
  }
}

void plGameObject::SetTag(const plTag& tag)
{
  if (plSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem())
  {
    if (m_Tags.IsSet(tag) == false)
    {
      m_Tags.Set(tag);
      m_pTransformationData->RecreateSpatialData(*pSpatialSystem);
    }
  }
  else
  {
    m_Tags.Set(tag);
  }
}

void plGameObject::RemoveTag(const plTag& tag)
{
  if (plSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem())
  {
    if (m_Tags.IsSet(tag))
    {
      m_Tags.Remove(tag);
      m_pTransformationData->RecreateSpatialData(*pSpatialSystem);
    }
  }
  else
  {
    m_Tags.Remove(tag);
  }
}

void plGameObject::FixComponentPointer(plComponent* pOldPtr, plComponent* pNewPtr)
{
  plUInt32 uiIndex = m_Components.IndexOf(pOldPtr);
  PLASMA_ASSERT_DEV(uiIndex != plInvalidIndex, "Memory corruption?");
  m_Components[uiIndex] = pNewPtr;
}

void plGameObject::SendNotificationMessage(plMessage& msg)
{
  plGameObject* pObject = this;
  while (pObject != nullptr)
  {
    pObject->SendMessage(msg);

    pObject = pObject->GetParent();
  }
}

//////////////////////////////////////////////////////////////////////////

void plGameObject::TransformationData::UpdateLocalTransform()
{
  plSimdTransform tLocal;

  if (m_pParentData != nullptr)
  {
    tLocal= plSimdTransform::MakeLocalTransform(m_pParentData->m_globalTransform, m_globalTransform);
  }
  else
  {
    tLocal = m_globalTransform;
  }

  m_localPosition = tLocal.m_Position;
  m_localRotation = tLocal.m_Rotation;
  m_localScaling = tLocal.m_Scale;
  m_localScaling.SetW(1.0f);
}

void plGameObject::TransformationData::UpdateGlobalTransformNonRecursive(plUInt32 uiUpdateCounter)
{
  if (m_pParentData != nullptr)
  {
    UpdateGlobalTransformWithParent(uiUpdateCounter);
  }
  else
  {
    UpdateGlobalTransformWithoutParent(uiUpdateCounter);
  }
}

void plGameObject::TransformationData::UpdateGlobalTransformRecursive(plUInt32 uiUpdateCounter)
{
  if (m_pParentData != nullptr)
  {
    m_pParentData->UpdateGlobalTransformRecursive(uiUpdateCounter);
    UpdateGlobalTransformWithParent(uiUpdateCounter);
  }
  else
  {
    UpdateGlobalTransformWithoutParent(uiUpdateCounter);
  }
}

void plGameObject::TransformationData::UpdateGlobalBounds(plSpatialSystem* pSpatialSystem)
{
  if (pSpatialSystem == nullptr)
  {
    UpdateGlobalBounds();
  }
  else
  {
    UpdateGlobalBoundsAndSpatialData(*pSpatialSystem);
  }
}

void plGameObject::TransformationData::UpdateGlobalBoundsAndSpatialData(plSpatialSystem& ref_spatialSystem)
{
  plSimdBBoxSphere oldGlobalBounds = m_globalBounds;

  UpdateGlobalBounds();

  const bool bIsAlwaysVisible = m_localBounds.m_BoxHalfExtents.w() != plSimdFloat::MakeZero();
  if (m_hSpatialData.IsInvalidated() == false && bIsAlwaysVisible == false && m_globalBounds != oldGlobalBounds)
  {
    ref_spatialSystem.UpdateSpatialDataBounds(m_hSpatialData, m_globalBounds);
  }
}

void plGameObject::TransformationData::RecreateSpatialData(plSpatialSystem& ref_spatialSystem)
{
  if (m_hSpatialData.IsInvalidated() == false)
  {
    ref_spatialSystem.DeleteSpatialData(m_hSpatialData);
    m_hSpatialData.Invalidate();
  }

  const bool bIsAlwaysVisible = m_localBounds.m_BoxHalfExtents.w() != plSimdFloat::MakeZero();
  if (bIsAlwaysVisible)
  {
    m_hSpatialData = ref_spatialSystem.CreateSpatialDataAlwaysVisible(m_pObject, m_uiSpatialDataCategoryBitmask, m_pObject->m_Tags);
  }
  else if (m_localBounds.IsValid())
  {
    UpdateGlobalBounds();
    m_hSpatialData = ref_spatialSystem.CreateSpatialData(m_globalBounds, m_pObject, m_uiSpatialDataCategoryBitmask, m_pObject->m_Tags);
  }
}

PLASMA_STATICLINK_FILE(Core, Core_World_Implementation_GameObject);