
PL_ALWAYS_INLINE plGameObject::ConstChildIterator::ConstChildIterator(plGameObject* pObject, const plWorld* pWorld)
  : m_pObject(pObject)
  , m_pWorld(pWorld)
{
}

PL_ALWAYS_INLINE const plGameObject& plGameObject::ConstChildIterator::operator*() const
{
  return *m_pObject;
}

PL_ALWAYS_INLINE const plGameObject* plGameObject::ConstChildIterator::operator->() const
{
  return m_pObject;
}

PL_ALWAYS_INLINE plGameObject::ConstChildIterator::operator const plGameObject*() const
{
  return m_pObject;
}

PL_ALWAYS_INLINE bool plGameObject::ConstChildIterator::IsValid() const
{
  return m_pObject != nullptr;
}

PL_ALWAYS_INLINE void plGameObject::ConstChildIterator::operator++()
{
  Next();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

PL_ALWAYS_INLINE plGameObject::ChildIterator::ChildIterator(plGameObject* pObject, const plWorld* pWorld)
  : ConstChildIterator(pObject, pWorld)
{
}

PL_ALWAYS_INLINE plGameObject& plGameObject::ChildIterator::operator*()
{
  return *m_pObject;
}

PL_ALWAYS_INLINE plGameObject* plGameObject::ChildIterator::operator->()
{
  return m_pObject;
}

PL_ALWAYS_INLINE plGameObject::ChildIterator::operator plGameObject*()
{
  return m_pObject;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline plGameObject::plGameObject() = default;

PL_ALWAYS_INLINE plGameObject::plGameObject(const plGameObject& other)
{
  *this = other;
}

PL_ALWAYS_INLINE plGameObjectHandle plGameObject::GetHandle() const
{
  return plGameObjectHandle(m_InternalId);
}

PL_ALWAYS_INLINE bool plGameObject::IsDynamic() const
{
  return m_Flags.IsSet(plObjectFlags::Dynamic);
}

PL_ALWAYS_INLINE bool plGameObject::IsStatic() const
{
  return !m_Flags.IsSet(plObjectFlags::Dynamic);
}

PL_ALWAYS_INLINE bool plGameObject::GetActiveFlag() const
{
  return m_Flags.IsSet(plObjectFlags::ActiveFlag);
}

PL_ALWAYS_INLINE bool plGameObject::IsActive() const
{
  return m_Flags.IsSet(plObjectFlags::ActiveState);
}

PL_ALWAYS_INLINE void plGameObject::SetName(plStringView sName)
{
  m_sName.Assign(sName);
}

PL_ALWAYS_INLINE void plGameObject::SetName(const plHashedString& sName)
{
  m_sName = sName;
}

PL_ALWAYS_INLINE void plGameObject::SetGlobalKey(plStringView sKey)
{
  plHashedString sGlobalKey;
  sGlobalKey.Assign(sKey);
  SetGlobalKey(sGlobalKey);
}

PL_ALWAYS_INLINE plStringView plGameObject::GetName() const
{
  return m_sName.GetView();
}

PL_ALWAYS_INLINE void plGameObject::SetNameInternal(const char* szName)
{
  m_sName.Assign(szName);
}

PL_ALWAYS_INLINE const char* plGameObject::GetNameInternal() const
{
  return m_sName;
}

PL_ALWAYS_INLINE void plGameObject::SetGlobalKeyInternal(const char* szName)
{
  SetGlobalKey(szName);
}

PL_ALWAYS_INLINE bool plGameObject::HasName(const plTempHashedString& sName) const
{
  return m_sName == sName;
}

PL_ALWAYS_INLINE void plGameObject::EnableChildChangesNotifications()
{
  m_Flags.Add(plObjectFlags::ChildChangesNotifications);
}

PL_ALWAYS_INLINE void plGameObject::DisableChildChangesNotifications()
{
  m_Flags.Remove(plObjectFlags::ChildChangesNotifications);
}

PL_ALWAYS_INLINE void plGameObject::EnableParentChangesNotifications()
{
  m_Flags.Add(plObjectFlags::ParentChangesNotifications);
}

PL_ALWAYS_INLINE void plGameObject::DisableParentChangesNotifications()
{
  m_Flags.Remove(plObjectFlags::ParentChangesNotifications);
}

PL_ALWAYS_INLINE void plGameObject::AddChildren(const plArrayPtr<const plGameObjectHandle>& children, plGameObject::TransformPreservation preserve)
{
  for (plUInt32 i = 0; i < children.GetCount(); ++i)
  {
    AddChild(children[i], preserve);
  }
}

PL_ALWAYS_INLINE void plGameObject::DetachChildren(const plArrayPtr<const plGameObjectHandle>& children, plGameObject::TransformPreservation preserve)
{
  for (plUInt32 i = 0; i < children.GetCount(); ++i)
  {
    DetachChild(children[i], preserve);
  }
}

PL_ALWAYS_INLINE plUInt32 plGameObject::GetChildCount() const
{
  return m_uiChildCount;
}

//PL_ALWAYS_INLINE plTransform plGameObject::GetLastGlobalTransform() const
//{
//#if PL_ENABLED(PL_GAMEOBJECT_VELOCITY)
//  return plSimdConversion::ToTransform(m_pTransformationData->m_lastGlobalTransform);
//#else
//  return plSimdConversion::ToTransform(m_pTransformationData->m_globalTransform);
//#endif
//}

PL_ALWAYS_INLINE void plGameObject::SetLocalPosition(plVec3 vPosition)
{
  SetLocalPosition(plSimdConversion::ToVec3(vPosition));
}

PL_ALWAYS_INLINE plVec3 plGameObject::GetLocalPosition() const
{
  return plSimdConversion::ToVec3(m_pTransformationData->m_localPosition);
}


PL_ALWAYS_INLINE void plGameObject::SetLocalRotation(plQuat qRotation)
{
  SetLocalRotation(plSimdConversion::ToQuat(qRotation));
}

PL_ALWAYS_INLINE plQuat plGameObject::GetLocalRotation() const
{
  return plSimdConversion::ToQuat(m_pTransformationData->m_localRotation);
}


PL_ALWAYS_INLINE void plGameObject::SetLocalScaling(plVec3 vScaling)
{
  SetLocalScaling(plSimdConversion::ToVec3(vScaling));
}

PL_ALWAYS_INLINE plVec3 plGameObject::GetLocalScaling() const
{
  return plSimdConversion::ToVec3(m_pTransformationData->m_localScaling);
}


PL_ALWAYS_INLINE void plGameObject::SetLocalUniformScaling(float fScaling)
{
  SetLocalUniformScaling(plSimdFloat(fScaling));
}

PL_ALWAYS_INLINE float plGameObject::GetLocalUniformScaling() const
{
  return m_pTransformationData->m_localScaling.w();
}

PL_ALWAYS_INLINE plTransform plGameObject::GetLocalTransform() const
{
  return plSimdConversion::ToTransform(GetLocalTransformSimd());
}


PL_ALWAYS_INLINE void plGameObject::SetGlobalPosition(const plVec3& vPosition)
{
  SetGlobalPosition(plSimdConversion::ToVec3(vPosition));
}

PL_ALWAYS_INLINE plVec3 plGameObject::GetGlobalPosition() const
{
  return plSimdConversion::ToVec3(m_pTransformationData->m_globalTransform.m_Position);
}


PL_ALWAYS_INLINE void plGameObject::SetGlobalRotation(const plQuat& qRotation)
{
  SetGlobalRotation(plSimdConversion::ToQuat(qRotation));
}

PL_ALWAYS_INLINE plQuat plGameObject::GetGlobalRotation() const
{
  return plSimdConversion::ToQuat(m_pTransformationData->m_globalTransform.m_Rotation);
}


PL_ALWAYS_INLINE void plGameObject::SetGlobalScaling(const plVec3& vScaling)
{
  SetGlobalScaling(plSimdConversion::ToVec3(vScaling));
}

PL_ALWAYS_INLINE plVec3 plGameObject::GetGlobalScaling() const
{
  return plSimdConversion::ToVec3(m_pTransformationData->m_globalTransform.m_Scale);
}


PL_ALWAYS_INLINE void plGameObject::SetGlobalTransform(const plTransform& transform)
{
#if PL_ENABLED(PL_GAMEOBJECT_VELOCITY)
  m_pTransformationData->m_lastGlobalTransform = m_pTransformationData->m_globalTransform;
#endif

  SetGlobalTransform(plSimdConversion::ToTransform(transform));
}

PL_ALWAYS_INLINE plTransform plGameObject::GetGlobalTransform() const
{
  return plSimdConversion::ToTransform(m_pTransformationData->m_globalTransform);
}

PL_ALWAYS_INLINE plTransform plGameObject::GetLastGlobalTransform() const
{
  return plSimdConversion::ToTransform(GetLastGlobalTransformSimd());
}


PL_ALWAYS_INLINE void plGameObject::SetLocalPosition(const plSimdVec4f& vPosition, UpdateBehaviorIfStatic updateBehavior)
{
  m_pTransformationData->m_localPosition = vPosition;

  if (IsStatic() && updateBehavior == UpdateBehaviorIfStatic::UpdateImmediately)
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

PL_ALWAYS_INLINE const plSimdVec4f& plGameObject::GetLocalPositionSimd() const
{
  return m_pTransformationData->m_localPosition;
}


PL_ALWAYS_INLINE void plGameObject::SetLocalRotation(const plSimdQuat& qRotation, UpdateBehaviorIfStatic updateBehavior)
{
  m_pTransformationData->m_localRotation = qRotation;

  if (IsStatic() && updateBehavior == UpdateBehaviorIfStatic::UpdateImmediately)
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

PL_ALWAYS_INLINE const plSimdQuat& plGameObject::GetLocalRotationSimd() const
{
  return m_pTransformationData->m_localRotation;
}


PL_ALWAYS_INLINE void plGameObject::SetLocalScaling(const plSimdVec4f& vScaling, UpdateBehaviorIfStatic updateBehavior)
{
  plSimdFloat uniformScale = m_pTransformationData->m_localScaling.w();
  m_pTransformationData->m_localScaling = vScaling;
  m_pTransformationData->m_localScaling.SetW(uniformScale);

  if (IsStatic() && updateBehavior == UpdateBehaviorIfStatic::UpdateImmediately)
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

PL_ALWAYS_INLINE const plSimdVec4f& plGameObject::GetLocalScalingSimd() const
{
  return m_pTransformationData->m_localScaling;
}


PL_ALWAYS_INLINE void plGameObject::SetLocalUniformScaling(const plSimdFloat& fScaling, UpdateBehaviorIfStatic updateBehavior)
{
  m_pTransformationData->m_localScaling.SetW(fScaling);

  if (IsStatic() && updateBehavior == UpdateBehaviorIfStatic::UpdateImmediately)
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

PL_ALWAYS_INLINE plSimdFloat plGameObject::GetLocalUniformScalingSimd() const
{
  return m_pTransformationData->m_localScaling.w();
}

PL_ALWAYS_INLINE plSimdTransform plGameObject::GetLocalTransformSimd() const
{
  const plSimdVec4f vScale = m_pTransformationData->m_localScaling * m_pTransformationData->m_localScaling.w();
  return plSimdTransform(m_pTransformationData->m_localPosition, m_pTransformationData->m_localRotation, vScale);
}


PL_ALWAYS_INLINE void plGameObject::SetGlobalPosition(const plSimdVec4f& vPosition)
{
  UpdateLastGlobalTransform();

  m_pTransformationData->m_globalTransform.m_Position = vPosition;

  m_pTransformationData->UpdateLocalTransform();

  if (IsStatic())
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

PL_ALWAYS_INLINE const plSimdVec4f& plGameObject::GetGlobalPositionSimd() const
{
  return m_pTransformationData->m_globalTransform.m_Position;
}


PL_ALWAYS_INLINE void plGameObject::SetGlobalRotation(const plSimdQuat& qRotation)
{
  UpdateLastGlobalTransform();

  m_pTransformationData->m_globalTransform.m_Rotation = qRotation;

  m_pTransformationData->UpdateLocalTransform();

  if (IsStatic())
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

PL_ALWAYS_INLINE const plSimdQuat& plGameObject::GetGlobalRotationSimd() const
{
  return m_pTransformationData->m_globalTransform.m_Rotation;
}


PL_ALWAYS_INLINE void plGameObject::SetGlobalScaling(const plSimdVec4f& vScaling)
{
  UpdateLastGlobalTransform();

  m_pTransformationData->m_globalTransform.m_Scale = vScaling;

  m_pTransformationData->UpdateLocalTransform();

  if (IsStatic())
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

PL_ALWAYS_INLINE const plSimdVec4f& plGameObject::GetGlobalScalingSimd() const
{
  return m_pTransformationData->m_globalTransform.m_Scale;
}


PL_ALWAYS_INLINE void plGameObject::SetGlobalTransform(const plSimdTransform& transform)
{
  UpdateLastGlobalTransform();

  m_pTransformationData->m_globalTransform = transform;

  // plTransformTemplate<Type>::SetLocalTransform will produce NaNs in w components
  // of pos and scale if scale.w is not set to 1 here. This only affects builds that
  // use PL_SIMD_IMPLEMENTATION_FPU, e.g. arm atm.
  m_pTransformationData->m_globalTransform.m_Scale.SetW(1.0f);
  m_pTransformationData->UpdateLocalTransform();

  if (IsStatic())
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

PL_ALWAYS_INLINE const plSimdTransform& plGameObject::GetGlobalTransformSimd() const
{
  return m_pTransformationData->m_globalTransform;
}

PL_ALWAYS_INLINE const plSimdTransform& plGameObject::GetLastGlobalTransformSimd() const
{
#if PL_ENABLED(PL_GAMEOBJECT_VELOCITY)
  return m_pTransformationData->m_lastGlobalTransform;
#else
  return m_pTransformationData->m_globalTransform;
#endif
}

PL_ALWAYS_INLINE void plGameObject::EnableStaticTransformChangesNotifications()
{
  m_Flags.Add(plObjectFlags::StaticTransformChangesNotifications);
}

PL_ALWAYS_INLINE void plGameObject::DisableStaticTransformChangesNotifications()
{
  m_Flags.Remove(plObjectFlags::StaticTransformChangesNotifications);
}

PL_ALWAYS_INLINE plBoundingBoxSphere plGameObject::GetLocalBounds() const
{
  return plSimdConversion::ToBBoxSphere(m_pTransformationData->m_localBounds);
}

PL_ALWAYS_INLINE plBoundingBoxSphere plGameObject::GetGlobalBounds() const
{
  return plSimdConversion::ToBBoxSphere(m_pTransformationData->m_globalBounds);
}

PL_ALWAYS_INLINE const plSimdBBoxSphere& plGameObject::GetLocalBoundsSimd() const
{
  return m_pTransformationData->m_localBounds;
}

PL_ALWAYS_INLINE const plSimdBBoxSphere& plGameObject::GetGlobalBoundsSimd() const
{
  return m_pTransformationData->m_globalBounds;
}

PL_ALWAYS_INLINE plSpatialDataHandle plGameObject::GetSpatialData() const
{
  return m_pTransformationData->m_hSpatialData;
}

PL_ALWAYS_INLINE void plGameObject::EnableComponentChangesNotifications()
{
  m_Flags.Add(plObjectFlags::ComponentChangesNotifications);
}

PL_ALWAYS_INLINE void plGameObject::DisableComponentChangesNotifications()
{
  m_Flags.Remove(plObjectFlags::ComponentChangesNotifications);
}

template <typename T>
PL_ALWAYS_INLINE bool plGameObject::TryGetComponentOfBaseType(T*& out_pComponent)
{
  return TryGetComponentOfBaseType(plGetStaticRTTI<T>(), (plComponent*&)out_pComponent);
}

template <typename T>
PL_ALWAYS_INLINE bool plGameObject::TryGetComponentOfBaseType(const T*& out_pComponent) const
{
  return TryGetComponentOfBaseType(plGetStaticRTTI<T>(), (const plComponent*&)out_pComponent);
}

template <typename T>
void plGameObject::TryGetComponentsOfBaseType(plDynamicArray<T*>& out_components)
{
  out_components.Clear();

  for (plUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    plComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf<T>())
    {
      out_components.PushBack(static_cast<T*>(pComponent));
    }
  }
}

template <typename T>
void plGameObject::TryGetComponentsOfBaseType(plDynamicArray<const T*>& out_components) const
{
  out_components.Clear();

  for (plUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    plComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf<T>())
    {
      out_components.PushBack(static_cast<const T*>(pComponent));
    }
  }
}

PL_ALWAYS_INLINE plArrayPtr<plComponent* const> plGameObject::GetComponents()
{
  return m_Components;
}

PL_ALWAYS_INLINE plArrayPtr<const plComponent* const> plGameObject::GetComponents() const
{
  return plMakeArrayPtr(const_cast<const plComponent* const*>(m_Components.GetData()), m_Components.GetCount());
}

PL_ALWAYS_INLINE plUInt16 plGameObject::GetComponentVersion() const
{
  return m_Components.GetUserData<ComponentUserData>().m_uiVersion;
}

PL_ALWAYS_INLINE bool plGameObject::SendMessage(plMessage& ref_msg)
{
  return SendMessageInternal(ref_msg, false);
}

PL_ALWAYS_INLINE bool plGameObject::SendMessage(plMessage& ref_msg) const
{
  return SendMessageInternal(ref_msg, false);
}

PL_ALWAYS_INLINE bool plGameObject::SendMessageRecursive(plMessage& ref_msg)
{
  return SendMessageRecursiveInternal(ref_msg, false);
}

PL_ALWAYS_INLINE bool plGameObject::SendMessageRecursive(plMessage& ref_msg) const
{
  return SendMessageRecursiveInternal(ref_msg, false);
}

PL_ALWAYS_INLINE const plTagSet& plGameObject::GetTags() const
{
  return m_Tags;
}

PL_ALWAYS_INLINE plUInt32 plGameObject::GetStableRandomSeed() const
{
  return m_pTransformationData->m_uiStableRandomSeed;
}

PL_ALWAYS_INLINE void plGameObject::SetStableRandomSeed(plUInt32 uiSeed)
{
  m_pTransformationData->m_uiStableRandomSeed = uiSeed;
}

//////////////////////////////////////////////////////////////////////////

PL_ALWAYS_INLINE void plGameObject::TransformationData::UpdateGlobalTransformWithoutParent(plUInt32 uiUpdateCounter)
{
  UpdateLastGlobalTransform(uiUpdateCounter);

  m_globalTransform.m_Position = m_localPosition;
  m_globalTransform.m_Rotation = m_localRotation;
  m_globalTransform.m_Scale = m_localScaling * m_localScaling.w();
}

PL_ALWAYS_INLINE void plGameObject::TransformationData::UpdateGlobalTransformWithParent(plUInt32 uiUpdateCounter)
{
  UpdateLastGlobalTransform(uiUpdateCounter);

  const plSimdVec4f vScale = m_localScaling * m_localScaling.w();
  const plSimdTransform localTransform(m_localPosition, m_localRotation, vScale);
  m_globalTransform = plSimdTransform::MakeGlobalTransform(m_pParentData->m_globalTransform, localTransform);
}

PL_FORCE_INLINE void plGameObject::TransformationData::UpdateGlobalBounds()
{
  m_globalBounds = m_localBounds;
  m_globalBounds.Transform(m_globalTransform);
}

PL_ALWAYS_INLINE void plGameObject::TransformationData::UpdateLastGlobalTransform(plUInt32 uiUpdateCounter)
{
#if PL_ENABLED(PL_GAMEOBJECT_VELOCITY)
  if (m_uiLastGlobalTransformUpdateCounter != uiUpdateCounter)
  {
    m_lastGlobalTransform = m_globalTransform;
    m_uiLastGlobalTransformUpdateCounter = uiUpdateCounter;
  }
#endif
}
