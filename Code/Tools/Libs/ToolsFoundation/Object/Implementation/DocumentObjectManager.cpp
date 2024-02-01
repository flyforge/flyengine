#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

////////////////////////////////////////////////////////////////////////
// plDocumentObjectManager
////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDocumentRoot, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ARRAY_MEMBER_PROPERTY("Children", m_RootObjects)->AddFlags(plPropertyFlags::PointerOwner),
    PL_ARRAY_MEMBER_PROPERTY("TempObjects", m_TempObjects)->AddFlags(plPropertyFlags::PointerOwner)->AddAttributes(new plTemporaryAttribute()),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plDocumentRootObject::InsertSubObject(plDocumentObject* pObject, plStringView sProperty, const plVariant& index)
{
  if (sProperty.IsEmpty())
    sProperty = "Children";
  return plDocumentObject::InsertSubObject(pObject, sProperty, index);
}

void plDocumentRootObject::RemoveSubObject(plDocumentObject* pObject)
{
  return plDocumentObject::RemoveSubObject(pObject);
}

plVariant plDocumentObjectPropertyEvent::getInsertIndex() const
{
  if (m_EventType == Type::PropertyMoved)
  {
    const plIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
    const plRTTI* pType = accessor.GetType();
    auto* pProp = pType->FindPropertyByName(m_sProperty);
    if (pProp->GetCategory() == plPropertyCategory::Array || pProp->GetCategory() == plPropertyCategory::Set)
    {
      plInt32 iCurrentIndex = m_OldIndex.ConvertTo<plInt32>();
      plInt32 iNewIndex = m_NewIndex.ConvertTo<plInt32>();
      // Move after oneself?
      if (iNewIndex > iCurrentIndex)
      {
        iNewIndex -= 1;
        return plVariant(iNewIndex);
      }
    }
  }
  return m_NewIndex;
}

plDocumentObjectManager::Storage::Storage(const plRTTI* pRootType)
  : m_RootObject(pRootType)
{
}

plDocumentObjectManager::plDocumentObjectManager(const plRTTI* pRootType)
{
  auto pStorage = PL_DEFAULT_NEW(Storage, pRootType);
  pStorage->m_RootObject.m_pDocumentObjectManager = this;
  SwapStorage(pStorage);
}

plDocumentObjectManager::~plDocumentObjectManager()
{
  if (m_pObjectStorage->GetRefCount() == 1)
  {
    PL_ASSERT_DEV(m_pObjectStorage->m_GuidToObject.IsEmpty(), "Not all objects have been destroyed!");
  }
}

////////////////////////////////////////////////////////////////////////
// plDocumentObjectManager Object Construction / Destruction
////////////////////////////////////////////////////////////////////////

plDocumentObject* plDocumentObjectManager::CreateObject(const plRTTI* pRtti, plUuid guid)
{
  PL_ASSERT_DEV(pRtti != nullptr, "Unknown RTTI type");

  plDocumentObject* pObject = InternalCreateObject(pRtti);
  // In case the storage is swapped, objects should still be created in their original document manager.
  pObject->m_pDocumentObjectManager = m_pObjectStorage->m_RootObject.GetDocumentObjectManager();

  if (guid.IsValid())
    pObject->m_Guid = guid;
  else
    pObject->m_Guid = plUuid::MakeUuid();

  PatchEmbeddedClassObjectsInternal(pObject, pRtti, false);

  plDocumentObjectEvent e;
  e.m_pObject = pObject;
  e.m_EventType = plDocumentObjectEvent::Type::AfterObjectCreated;
  m_pObjectStorage->m_ObjectEvents.Broadcast(e);

  return pObject;
}

void plDocumentObjectManager::DestroyObject(plDocumentObject* pObject)
{
  for (plDocumentObject* pChild : pObject->m_Children)
  {
    DestroyObject(pChild);
  }

  plDocumentObjectEvent e;
  e.m_pObject = pObject;
  e.m_EventType = plDocumentObjectEvent::Type::BeforeObjectDestroyed;
  m_pObjectStorage->m_ObjectEvents.Broadcast(e);

  InternalDestroyObject(pObject);
}

void plDocumentObjectManager::DestroyAllObjects()
{
  for (auto child : m_pObjectStorage->m_RootObject.m_Children)
  {
    DestroyObject(child);
  }

  m_pObjectStorage->m_RootObject.m_Children.Clear();
  m_pObjectStorage->m_GuidToObject.Clear();
}

void plDocumentObjectManager::PatchEmbeddedClassObjects(const plDocumentObject* pObject) const
{
  // Functional should be callable from anywhere but will of course have side effects.
  const_cast<plDocumentObjectManager*>(this)->PatchEmbeddedClassObjectsInternal(
    const_cast<plDocumentObject*>(pObject), pObject->GetTypeAccessor().GetType(), true);
}

const plDocumentObject* plDocumentObjectManager::GetObject(const plUuid& guid) const
{
  const plDocumentObject* pObject = nullptr;
  if (m_pObjectStorage->m_GuidToObject.TryGetValue(guid, pObject))
  {
    return pObject;
  }
  else if (guid == m_pObjectStorage->m_RootObject.GetGuid())
    return &m_pObjectStorage->m_RootObject;
  return nullptr;
}

plDocumentObject* plDocumentObjectManager::GetObject(const plUuid& guid)
{
  return const_cast<plDocumentObject*>(((const plDocumentObjectManager*)this)->GetObject(guid));
}

////////////////////////////////////////////////////////////////////////
// plDocumentObjectManager Property Change
////////////////////////////////////////////////////////////////////////

plStatus plDocumentObjectManager::SetValue(plDocumentObject* pObject, plStringView sProperty, const plVariant& newValue, plVariant index)
{
  PL_ASSERT_DEBUG(pObject, "Object must not be null.");
  plIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  plVariant oldValue = accessor.GetValue(sProperty, index);

  if (!accessor.SetValue(sProperty, newValue, index))
  {
    return plStatus(plFmt("Set Property: The property '{0}' does not exist or value type does not match", sProperty));
  }

  plDocumentObjectPropertyEvent e;
  e.m_EventType = plDocumentObjectPropertyEvent::Type::PropertySet;
  e.m_pObject = pObject;
  e.m_OldValue = oldValue;
  e.m_NewValue = newValue;
  e.m_sProperty = sProperty;
  e.m_NewIndex = index;

  // Allow a recursion depth of 2 for property setters. This allowed for two levels of side-effects on property setters.
  m_pObjectStorage->m_PropertyEvents.Broadcast(e, 2);
  return plStatus(PL_SUCCESS);
}

plStatus plDocumentObjectManager::InsertValue(plDocumentObject* pObject, plStringView sProperty, const plVariant& newValue, plVariant index)
{
  plIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  if (!accessor.InsertValue(sProperty, index, newValue))
  {
    if (!accessor.GetType()->FindPropertyByName(sProperty))
    {
      return plStatus(plFmt("Insert Property: The property '{0}' does not exist", sProperty));
    }
    return plStatus(plFmt("Insert Property: The property '{0}' already has the key '{1}'", sProperty, index));
  }

  plDocumentObjectPropertyEvent e;
  e.m_EventType = plDocumentObjectPropertyEvent::Type::PropertyInserted;
  e.m_pObject = pObject;
  e.m_NewValue = newValue;
  e.m_NewIndex = index;
  e.m_sProperty = sProperty;

  m_pObjectStorage->m_PropertyEvents.Broadcast(e);

  return plStatus(PL_SUCCESS);
}

plStatus plDocumentObjectManager::RemoveValue(plDocumentObject* pObject, plStringView sProperty, plVariant index)
{
  plIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  plVariant oldValue = accessor.GetValue(sProperty, index);

  if (!accessor.RemoveValue(sProperty, index))
  {
    return plStatus(plFmt("Remove Property: The index '{0}' in property '{1}' does not exist!", index.ConvertTo<plString>(), sProperty));
  }

  plDocumentObjectPropertyEvent e;
  e.m_EventType = plDocumentObjectPropertyEvent::Type::PropertyRemoved;
  e.m_pObject = pObject;
  e.m_OldValue = oldValue;
  e.m_OldIndex = index;
  e.m_sProperty = sProperty;

  m_pObjectStorage->m_PropertyEvents.Broadcast(e);

  return plStatus(PL_SUCCESS);
}

plStatus plDocumentObjectManager::MoveValue(plDocumentObject* pObject, plStringView sProperty, const plVariant& oldIndex, const plVariant& newIndex)
{
  if (!oldIndex.CanConvertTo<plInt32>() || !newIndex.CanConvertTo<plInt32>())
    return plStatus("Move Property: Invalid indices provided.");

  plIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  plInt32 iCount = accessor.GetCount(sProperty);
  if (iCount < 0)
    return plStatus("Move Property: Invalid property.");
  if (oldIndex.ConvertTo<plInt32>() < 0 || oldIndex.ConvertTo<plInt32>() >= iCount)
    return plStatus(plFmt("Move Property: Invalid old index '{0}'.", oldIndex.ConvertTo<plInt32>()));
  if (newIndex.ConvertTo<plInt32>() < 0 || newIndex.ConvertTo<plInt32>() > iCount)
    return plStatus(plFmt("Move Property: Invalid new index '{0}'.", newIndex.ConvertTo<plInt32>()));

  if (!accessor.MoveValue(sProperty, oldIndex, newIndex))
    return plStatus("Move Property: Move value failed.");

  {
    plDocumentObjectPropertyEvent e;
    e.m_EventType = plDocumentObjectPropertyEvent::Type::PropertyMoved;
    e.m_pObject = pObject;
    e.m_OldIndex = oldIndex;
    e.m_NewIndex = newIndex;
    e.m_sProperty = sProperty;
    e.m_NewValue = accessor.GetValue(sProperty, e.getInsertIndex());
    // NewValue can be invalid if an invalid variant in a variant array is moved
    //PL_ASSERT_DEV(e.m_NewValue.IsValid(), "Value at new pos should be valid now, index missmatch?");
    m_pObjectStorage->m_PropertyEvents.Broadcast(e);
  }

  return plStatus(PL_SUCCESS);
}

////////////////////////////////////////////////////////////////////////
// plDocumentObjectManager Structure Change
////////////////////////////////////////////////////////////////////////

void plDocumentObjectManager::AddObject(plDocumentObject* pObject, plDocumentObject* pParent, plStringView sParentProperty, plVariant index)
{
  if (pParent == nullptr)
    pParent = &m_pObjectStorage->m_RootObject;
  if (pParent == &m_pObjectStorage->m_RootObject && sParentProperty.IsEmpty())
    sParentProperty = "Children";

  PL_ASSERT_DEV(pObject->GetGuid().IsValid(), "Object Guid invalid! Object was not created via an plObjectManagerBase!");
  PL_ASSERT_DEV(
    CanAdd(pObject->GetTypeAccessor().GetType(), pParent, sParentProperty, index).m_Result.Succeeded(), "Trying to execute invalid add!");

  InternalAddObject(pObject, pParent, sParentProperty, index);
}

void plDocumentObjectManager::RemoveObject(plDocumentObject* pObject)
{
  PL_ASSERT_DEV(CanRemove(pObject).m_Result.Succeeded(), "Trying to execute invalid remove!");
  InternalRemoveObject(pObject);
}

void plDocumentObjectManager::MoveObject(plDocumentObject* pObject, plDocumentObject* pNewParent, plStringView sParentProperty, plVariant index)
{
  PL_ASSERT_DEV(CanMove(pObject, pNewParent, sParentProperty, index).m_Result.Succeeded(), "Trying to execute invalid move!");

  InternalMoveObject(pNewParent, pObject, sParentProperty, index);
}


////////////////////////////////////////////////////////////////////////
// plDocumentObjectManager Structure Change Test
////////////////////////////////////////////////////////////////////////

plStatus plDocumentObjectManager::CanAdd(
  const plRTTI* pRtti, const plDocumentObject* pParent, plStringView sParentProperty, const plVariant& index) const
{
  // Test whether parent exists in tree.
  if (pParent == GetRootObject())
    pParent = nullptr;

  if (pParent != nullptr)
  {
    const plDocumentObject* pObjectInTree = GetObject(pParent->GetGuid());
    PL_ASSERT_DEV(pObjectInTree == pParent, "Tree Corruption!!!");
    if (pObjectInTree == nullptr)
      return plStatus("Parent is not part of the object manager!");

    const plIReflectedTypeAccessor& accessor = pParent->GetTypeAccessor();
    const plRTTI* pType = accessor.GetType();
    auto* pProp = pType->FindPropertyByName(sParentProperty);
    if (pProp == nullptr)
      return plStatus(plFmt("Property '{0}' could not be found in type '{1}'", sParentProperty, pType->GetTypeName()));

    const bool bIsValueType = plReflectionUtils::IsValueType(pProp);

    if (bIsValueType || pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags))
    {
      return plStatus("Need to use 'InsertValue' action instead.");
    }
    else if (pProp->GetFlags().IsSet(plPropertyFlags::Class))
    {
      if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
      {
        if (!pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
          return plStatus(plFmt("Cannot add object to the pointer property '{0}' as it does not hold ownership.", sParentProperty));

        if (!pRtti->IsDerivedFrom(pProp->GetSpecificType()))
          return plStatus(plFmt("Cannot add object to the pointer property '{0}' as its type '{1}' is not derived from the property type '{2}'!",
            sParentProperty, pRtti->GetTypeName(), pProp->GetSpecificType()->GetTypeName()));
      }
      else
      {
        if (pRtti != pProp->GetSpecificType())
          return plStatus(plFmt("Cannot add object to the property '{0}' as its type '{1}' does not match the property type '{2}'!", sParentProperty,
            pRtti->GetTypeName(), pProp->GetSpecificType()->GetTypeName()));
      }
    }

    if (pProp->GetCategory() == plPropertyCategory::Array || pProp->GetCategory() == plPropertyCategory::Set)
    {
      plInt32 iCount = accessor.GetCount(sParentProperty);
      if (!index.CanConvertTo<plInt32>())
      {
        return plStatus(plFmt("Cannot add object to the property '{0}', the given index is an invalid plVariant (Either use '-1' to append "
                              "or a valid index).",
          sParentProperty));
      }
      plInt32 iNewIndex = index.ConvertTo<plInt32>();
      if (iNewIndex > (plInt32)iCount)
        return plStatus(plFmt(
          "Cannot add object to its new location '{0}' is out of the bounds of the parent's property range '{1}'!", iNewIndex, (plInt32)iCount));
      if (iNewIndex < 0 && iNewIndex != -1)
        return plStatus(plFmt("Cannot add object to the property '{0}', the index '{1}' is not valid (Either use '-1' to append or a valid index).",
          sParentProperty, iNewIndex));
    }
    if (pProp->GetCategory() == plPropertyCategory::Map)
    {
      if (!index.IsA<plString>())
        return plStatus(plFmt("Cannot add object to the map property '{0}' as its index type is not a string.", sParentProperty));
      plVariant value = accessor.GetValue(sParentProperty, index);
      if (value.IsValid() && value.IsA<plUuid>())
      {
        plUuid guid = value.Get<plUuid>();
        if (guid.IsValid())
          return plStatus(
            plFmt("Cannot add object to the map property '{0}' at key '{1}'. Delete old value first.", sParentProperty, index.Get<plString>()));
      }
    }
    else if (pProp->GetCategory() == plPropertyCategory::Member)
    {
      if (!pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
        return plStatus("Embedded classes cannot be changed manually.");

      plVariant value = accessor.GetValue(sParentProperty);
      if (!value.IsA<plUuid>())
        return plStatus("Property is not a pointer and thus can't be added to.");

      if (value.Get<plUuid>().IsValid())
        return plStatus("Can't set pointer if it already has a value, need to delete value first.");
    }
  }

  return InternalCanAdd(pRtti, pParent, sParentProperty, index);
}

plStatus plDocumentObjectManager::CanRemove(const plDocumentObject* pObject) const
{
  const plDocumentObject* pObjectInTree = GetObject(pObject->GetGuid());

  if (pObjectInTree == nullptr)
    return plStatus("Object is not part of the object manager!");

  if (pObject->GetParent())
  {
    const plAbstractProperty* pProp = pObject->GetParentPropertyType();
    PL_ASSERT_DEV(pProp != nullptr, "Parent property should always be valid!");
    if (pProp->GetCategory() == plPropertyCategory::Member && !pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
      return plStatus("Non pointer members can't be deleted!");
  }
  PL_ASSERT_DEV(pObjectInTree == pObject, "Tree Corruption!!!");

  return InternalCanRemove(pObject);
}

plStatus plDocumentObjectManager::CanMove(
  const plDocumentObject* pObject, const plDocumentObject* pNewParent, plStringView sParentProperty, const plVariant& index) const
{
  PL_SUCCEED_OR_RETURN(CanAdd(pObject->GetTypeAccessor().GetType(), pNewParent, sParentProperty, index));

  PL_SUCCEED_OR_RETURN(CanRemove(pObject));

  if (pNewParent == nullptr)
    pNewParent = GetRootObject();

  if (pObject == pNewParent)
    return plStatus("Can't move object onto itself!");

  const plDocumentObject* pObjectInTree = GetObject(pObject->GetGuid());

  if (pObjectInTree == nullptr)
    return plStatus("Object is not part of the object manager!");

  PL_ASSERT_DEV(pObjectInTree == pObject, "Tree Corruption!!!");

  if (pNewParent != GetRootObject())
  {
    const plDocumentObject* pNewParentInTree = GetObject(pNewParent->GetGuid());

    if (pNewParentInTree == nullptr)
      return plStatus("New parent is not part of the object manager!");

    PL_ASSERT_DEV(pNewParentInTree == pNewParent, "Tree Corruption!!!");
  }

  const plDocumentObject* pCurParent = pNewParent->GetParent();

  while (pCurParent)
  {
    if (pCurParent == pObject)
      return plStatus("Can't move object to one of its children!");

    pCurParent = pCurParent->GetParent();
  }

  const plIReflectedTypeAccessor& accessor = pNewParent->GetTypeAccessor();
  const plRTTI* pType = accessor.GetType();

  auto* pProp = pType->FindPropertyByName(sParentProperty);

  if (pProp == nullptr)
    return plStatus(plFmt("Property '{0}' could not be found in type '{1}'", sParentProperty, pType->GetTypeName()));

  if (pProp->GetCategory() == plPropertyCategory::Array || pProp->GetCategory() == plPropertyCategory::Set)
  {
    plInt32 iChildIndex = index.ConvertTo<plInt32>();
    if (iChildIndex == -1)
    {
      iChildIndex = pNewParent->GetTypeAccessor().GetCount(sParentProperty);
    }

    if (pNewParent == pObject->GetParent())
    {
      // Test whether we are moving before or after ourselves, both of which are not allowed and would not change the tree.
      plIReflectedTypeAccessor& oldAccessor = pObject->m_pParent->GetTypeAccessor();
      plInt32 iCurrentIndex = oldAccessor.GetPropertyChildIndex(sParentProperty, pObject->GetGuid()).ConvertTo<plInt32>();
      if (iChildIndex == iCurrentIndex || iChildIndex == iCurrentIndex + 1)
        return plStatus("Can't move object onto itself!");
    }
  }
  if (pProp->GetCategory() == plPropertyCategory::Map)
  {
    if (!index.IsA<plString>())
      return plStatus(plFmt("Cannot add object to the map property '{0}' as its index type is not a string.", sParentProperty));
    plVariant value = accessor.GetValue(sParentProperty, index);
    if (value.IsValid() && value.IsA<plUuid>())
    {
      plUuid guid = value.Get<plUuid>();
      if (guid.IsValid())
        return plStatus(
          plFmt("Cannot add object to the map property '{0}' at key '{1}'. Delete old value first.", sParentProperty, index.Get<plString>()));
    }
  }

  if (pNewParent == GetRootObject())
    pNewParent = nullptr;

  return InternalCanMove(pObject, pNewParent, sParentProperty, index);
}

plStatus plDocumentObjectManager::CanSelect(const plDocumentObject* pObject) const
{
  PL_ASSERT_DEV(pObject != nullptr, "pObject must be valid");

  const plDocumentObject* pOwnObject = GetObject(pObject->GetGuid());
  if (pOwnObject == nullptr)
    return plStatus(
      plFmt("Object of type '{0}' is not part of the document and can't be selected", pObject->GetTypeAccessor().GetType()->GetTypeName()));

  return InternalCanSelect(pObject);
}


bool plDocumentObjectManager::IsUnderRootProperty(plStringView sRootProperty, const plDocumentObject* pObject) const
{
  PL_ASSERT_DEBUG(m_pObjectStorage->m_RootObject.GetDocumentObjectManager() == pObject->GetDocumentObjectManager(), "Passed in object does not belong to this object manager.");
  while (pObject->GetParent() != GetRootObject())
  {
    pObject = pObject->GetParent();
  }
  return sRootProperty == pObject->GetParentProperty();
}


bool plDocumentObjectManager::IsUnderRootProperty(plStringView sRootProperty, const plDocumentObject* pParent, plStringView sParentProperty) const
{
  PL_ASSERT_DEBUG(pParent == nullptr || m_pObjectStorage->m_RootObject.GetDocumentObjectManager() == pParent->GetDocumentObjectManager(), "Passed in object does not belong to this object manager.");
  if (pParent == nullptr || pParent == GetRootObject())
  {
    return sParentProperty == sRootProperty;
  }
  return IsUnderRootProperty(sRootProperty, pParent);
}

bool plDocumentObjectManager::IsTemporary(const plDocumentObject* pObject) const
{
  return IsUnderRootProperty("TempObjects", pObject);
}

bool plDocumentObjectManager::IsTemporary(const plDocumentObject* pParent, plStringView sParentProperty) const
{
  return IsUnderRootProperty("TempObjects", pParent, sParentProperty);
}

plSharedPtr<plDocumentObjectManager::Storage> plDocumentObjectManager::SwapStorage(plSharedPtr<plDocumentObjectManager::Storage> pNewStorage)
{
  PL_ASSERT_ALWAYS(pNewStorage != nullptr, "Need a valid history storage object");

  auto retVal = m_pObjectStorage;

  m_StructureEventsUnsubscriber.Unsubscribe();
  m_PropertyEventsUnsubscriber.Unsubscribe();
  m_ObjectEventsUnsubscriber.Unsubscribe();

  m_pObjectStorage = pNewStorage;

  m_pObjectStorage->m_StructureEvents.AddEventHandler([this](const plDocumentObjectStructureEvent& e) { m_StructureEvents.Broadcast(e); }, m_StructureEventsUnsubscriber);
  m_pObjectStorage->m_PropertyEvents.AddEventHandler([this](const plDocumentObjectPropertyEvent& e) { m_PropertyEvents.Broadcast(e, 2); }, m_PropertyEventsUnsubscriber);
  m_pObjectStorage->m_ObjectEvents.AddEventHandler([this](const plDocumentObjectEvent& e) { m_ObjectEvents.Broadcast(e); }, m_ObjectEventsUnsubscriber);

  return retVal;
}

////////////////////////////////////////////////////////////////////////
// plDocumentObjectManager Private Functions
////////////////////////////////////////////////////////////////////////

void plDocumentObjectManager::InternalAddObject(plDocumentObject* pObject, plDocumentObject* pParent, plStringView sParentProperty, plVariant index)
{
  plDocumentObjectStructureEvent e;
  e.m_pDocument = m_pObjectStorage->m_pDocument;
  e.m_EventType = plDocumentObjectStructureEvent::Type::BeforeObjectAdded;
  e.m_pObject = pObject;
  e.m_pPreviousParent = nullptr;
  e.m_pNewParent = pParent;
  e.m_sParentProperty = sParentProperty;
  e.m_NewPropertyIndex = index;

  if (e.m_NewPropertyIndex.CanConvertTo<plInt32>() && e.m_NewPropertyIndex.ConvertTo<plInt32>() == -1)
  {
    plIReflectedTypeAccessor& accessor = pParent->GetTypeAccessor();
    e.m_NewPropertyIndex = accessor.GetCount(sParentProperty);
  }
  m_pObjectStorage->m_StructureEvents.Broadcast(e);

  pParent->InsertSubObject(pObject, sParentProperty, e.m_NewPropertyIndex);
  RecursiveAddGuids(pObject);

  e.m_EventType = plDocumentObjectStructureEvent::Type::AfterObjectAdded;
  m_pObjectStorage->m_StructureEvents.Broadcast(e);
}

void plDocumentObjectManager::InternalRemoveObject(plDocumentObject* pObject)
{
  plDocumentObjectStructureEvent e;
  e.m_pDocument = m_pObjectStorage->m_pDocument;
  e.m_EventType = plDocumentObjectStructureEvent::Type::BeforeObjectRemoved;
  e.m_pObject = pObject;
  e.m_pPreviousParent = pObject->m_pParent;
  e.m_pNewParent = nullptr;
  e.m_sParentProperty = pObject->m_sParentProperty;
  e.m_OldPropertyIndex = pObject->GetPropertyIndex();
  m_pObjectStorage->m_StructureEvents.Broadcast(e);

  pObject->m_pParent->RemoveSubObject(pObject);
  RecursiveRemoveGuids(pObject);

  e.m_EventType = plDocumentObjectStructureEvent::Type::AfterObjectRemoved;
  m_pObjectStorage->m_StructureEvents.Broadcast(e);
}

void plDocumentObjectManager::InternalMoveObject(
  plDocumentObject* pNewParent, plDocumentObject* pObject, plStringView sParentProperty, plVariant index)
{
  if (pNewParent == nullptr)
    pNewParent = &m_pObjectStorage->m_RootObject;

  plDocumentObjectStructureEvent e;
  e.m_pDocument = m_pObjectStorage->m_pDocument;
  e.m_EventType = plDocumentObjectStructureEvent::Type::BeforeObjectMoved;
  e.m_pObject = pObject;
  e.m_pPreviousParent = pObject->m_pParent;
  e.m_pNewParent = pNewParent;
  e.m_sParentProperty = sParentProperty;
  e.m_OldPropertyIndex = pObject->GetPropertyIndex();
  e.m_NewPropertyIndex = index;
  if (e.m_NewPropertyIndex.CanConvertTo<plInt32>() && e.m_NewPropertyIndex.ConvertTo<plInt32>() == -1)
  {
    plIReflectedTypeAccessor& accessor = pNewParent->GetTypeAccessor();
    e.m_NewPropertyIndex = accessor.GetCount(sParentProperty);
  }

  m_pObjectStorage->m_StructureEvents.Broadcast(e);

  plVariant newIndex = e.getInsertIndex();

  pObject->m_pParent->RemoveSubObject(pObject);
  pNewParent->InsertSubObject(pObject, sParentProperty, newIndex);

  e.m_EventType = plDocumentObjectStructureEvent::Type::AfterObjectMoved;
  m_pObjectStorage->m_StructureEvents.Broadcast(e);

  e.m_EventType = plDocumentObjectStructureEvent::Type::AfterObjectMoved2;
  m_pObjectStorage->m_StructureEvents.Broadcast(e);
}

void plDocumentObjectManager::RecursiveAddGuids(plDocumentObject* pObject)
{
  m_pObjectStorage->m_GuidToObject[pObject->m_Guid] = pObject;

  for (plUInt32 c = 0; c < pObject->GetChildren().GetCount(); ++c)
    RecursiveAddGuids(pObject->GetChildren()[c]);
}

void plDocumentObjectManager::RecursiveRemoveGuids(plDocumentObject* pObject)
{
  m_pObjectStorage->m_GuidToObject.Remove(pObject->m_Guid);

  for (plUInt32 c = 0; c < pObject->GetChildren().GetCount(); ++c)
    RecursiveRemoveGuids(pObject->GetChildren()[c]);
}

void plDocumentObjectManager::PatchEmbeddedClassObjectsInternal(plDocumentObject* pObject, const plRTTI* pType, bool addToDoc)
{
  const plRTTI* pParent = pType->GetParentType();
  if (pParent != nullptr)
    PatchEmbeddedClassObjectsInternal(pObject, pParent, addToDoc);

  plIReflectedTypeAccessor& accessor = pObject->GetTypeAccessor();
  const plUInt32 uiPropertyCount = pType->GetProperties().GetCount();
  for (plUInt32 i = 0; i < uiPropertyCount; ++i)
  {
    const plAbstractProperty* pProperty = pType->GetProperties()[i];
    const plVariantTypeInfo* pInfo = plVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pProperty->GetSpecificType());

    if (pProperty->GetCategory() == plPropertyCategory::Member && pProperty->GetFlags().IsSet(plPropertyFlags::Class) && !pInfo &&
        !pProperty->GetFlags().IsSet(plPropertyFlags::Pointer))
    {
      plUuid value = accessor.GetValue(pProperty->GetPropertyName()).Get<plUuid>();
      PL_ASSERT_DEV(addToDoc || !value.IsValid(), "If addToDoc is false, the current value must be invalid!");
      if (value.IsValid())
      {
        plDocumentObject* pEmbeddedObject = GetObject(value);
        if (pEmbeddedObject)
        {
          if (pEmbeddedObject->GetTypeAccessor().GetType() == pProperty->GetSpecificType())
            continue;
          else
          {
            // Type mismatch, delete old.
            InternalRemoveObject(pEmbeddedObject);
          }
        }
      }

      // Create new
      plStringBuilder sTemp;
      plConversionUtils::ToString(pObject->GetGuid(), sTemp);
      sTemp.Append("/", pProperty->GetPropertyName());
      const plUuid subObjectGuid = plUuid::MakeStableUuidFromString(sTemp);
      plDocumentObject* pEmbeddedObject = CreateObject(pProperty->GetSpecificType(), subObjectGuid);
      if (addToDoc)
      {
        InternalAddObject(pEmbeddedObject, pObject, pProperty->GetPropertyName(), plVariant());
      }
      else
      {
        pObject->InsertSubObject(pEmbeddedObject, pProperty->GetPropertyName(), plVariant());
      }
    }
  }
}


const plAbstractProperty* plDocumentObjectStructureEvent::GetProperty() const
{
  return m_pObject->GetParentPropertyType();
}

plVariant plDocumentObjectStructureEvent::getInsertIndex() const
{
  if ((m_EventType == Type::BeforeObjectMoved || m_EventType == Type::AfterObjectMoved || m_EventType == Type::AfterObjectMoved2) &&
      m_pNewParent == m_pPreviousParent)
  {
    const plIReflectedTypeAccessor& accessor = m_pPreviousParent->GetTypeAccessor();
    const plRTTI* pType = accessor.GetType();
    auto* pProp = pType->FindPropertyByName(m_sParentProperty);
    if (pProp->GetCategory() == plPropertyCategory::Array || pProp->GetCategory() == plPropertyCategory::Set)
    {
      plInt32 iCurrentIndex = m_OldPropertyIndex.ConvertTo<plInt32>();
      plInt32 iNewIndex = m_NewPropertyIndex.ConvertTo<plInt32>();
      // Move after oneself?
      if (iNewIndex > iCurrentIndex)
      {
        iNewIndex -= 1;
        return plVariant(iNewIndex);
      }
    }
  }
  return m_NewPropertyIndex;
}
