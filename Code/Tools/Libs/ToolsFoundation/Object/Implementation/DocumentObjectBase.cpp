#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

plIReflectedTypeAccessor& plDocumentObject::GetTypeAccessor()
{
  const plDocumentObject* pMe = this;
  return const_cast<plIReflectedTypeAccessor&>(pMe->GetTypeAccessor());
}

plUInt32 plDocumentObject::GetChildIndex(const plDocumentObject* pChild) const
{
  return m_Children.IndexOf(const_cast<plDocumentObject*>(pChild));
}

void plDocumentObject::InsertSubObject(plDocumentObject* pObject, plStringView sProperty, const plVariant& index)
{
  PLASMA_ASSERT_DEV(pObject != nullptr, "");
  PLASMA_ASSERT_DEV(!sProperty.IsEmpty(), "Child objects must have a parent property to insert into");
  plIReflectedTypeAccessor& accessor = GetTypeAccessor();

  const plRTTI* pType = accessor.GetType();
  auto* pProp = pType->FindPropertyByName(sProperty);
  PLASMA_ASSERT_DEV(pProp && pProp->GetFlags().IsSet(plPropertyFlags::Class) &&
                  (!pProp->GetFlags().IsSet(plPropertyFlags::Pointer) || pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner)),
    "Only class type or pointer to class type that own the object can be inserted, everything else is handled by value.");

  if (pProp->GetCategory() == plPropertyCategory::Array || pProp->GetCategory() == plPropertyCategory::Set)
  {
    if (!index.IsValid() || (index.CanConvertTo<plInt32>() && index.ConvertTo<plInt32>() == -1))
    {
      plVariant newIndex = accessor.GetCount(sProperty);
      bool bRes = accessor.InsertValue(sProperty, newIndex, pObject->GetGuid());
      PLASMA_ASSERT_DEV(bRes, "");
    }
    else
    {
      bool bRes = accessor.InsertValue(sProperty, index, pObject->GetGuid());
      PLASMA_ASSERT_DEV(bRes, "");
    }
  }
  else if (pProp->GetCategory() == plPropertyCategory::Map)
  {
    PLASMA_ASSERT_DEV(index.IsA<plString>(), "Map key must be a string.");
    bool bRes = accessor.InsertValue(sProperty, index, pObject->GetGuid());
    PLASMA_ASSERT_DEV(bRes, "");
  }
  else if (pProp->GetCategory() == plPropertyCategory::Member)
  {
    bool bRes = accessor.SetValue(sProperty, pObject->GetGuid());
    PLASMA_ASSERT_DEV(bRes, "");
  }

  // Object patching
  pObject->m_sParentProperty = sProperty;
  pObject->m_pParent = this;
  m_Children.PushBack(pObject);
}

void plDocumentObject::RemoveSubObject(plDocumentObject* pObject)
{
  PLASMA_ASSERT_DEV(pObject != nullptr, "");
  PLASMA_ASSERT_DEV(!pObject->m_sParentProperty.IsEmpty(), "");
  PLASMA_ASSERT_DEV(this == pObject->m_pParent, "");
  plIReflectedTypeAccessor& accessor = GetTypeAccessor();

  // Property patching
  const plRTTI* pType = accessor.GetType();
  auto* pProp = pType->FindPropertyByName(pObject->m_sParentProperty);
  if (pProp->GetCategory() == plPropertyCategory::Array || pProp->GetCategory() == plPropertyCategory::Set ||
      pProp->GetCategory() == plPropertyCategory::Map)
  {
    plVariant index = accessor.GetPropertyChildIndex(pObject->m_sParentProperty, pObject->GetGuid());
    bool bRes = accessor.RemoveValue(pObject->m_sParentProperty, index);
    PLASMA_ASSERT_DEV(bRes, "");
  }
  else if (pProp->GetCategory() == plPropertyCategory::Member)
  {
    bool bRes = accessor.SetValue(pObject->m_sParentProperty, plUuid());
    PLASMA_ASSERT_DEV(bRes, "");
  }

  m_Children.RemoveAndCopy(pObject);
  pObject->m_pParent = nullptr;
}

void plDocumentObject::ComputeObjectHash(plUInt64& ref_uiHash) const
{
  const plIReflectedTypeAccessor& acc = GetTypeAccessor();
  auto pType = acc.GetType();

  ref_uiHash = plHashingUtils::xxHash64(&m_Guid, sizeof(plUuid), ref_uiHash);
  HashPropertiesRecursive(acc, ref_uiHash, pType);
}


plDocumentObject* plDocumentObject::GetChild(const plUuid& guid)
{
  for (auto* pChild : m_Children)
  {
    if (pChild->GetGuid() == guid)
      return pChild;
  }
  return nullptr;
}


const plDocumentObject* plDocumentObject::GetChild(const plUuid& guid) const
{
  for (auto* pChild : m_Children)
  {
    if (pChild->GetGuid() == guid)
      return pChild;
  }
  return nullptr;
}

const plAbstractProperty* plDocumentObject::GetParentPropertyType() const
{
  if (!m_pParent)
    return nullptr;
  const plIReflectedTypeAccessor& accessor = m_pParent->GetTypeAccessor();
  const plRTTI* pType = accessor.GetType();
  return pType->FindPropertyByName(m_sParentProperty);
}

plVariant plDocumentObject::GetPropertyIndex() const
{
  if (m_pParent == nullptr)
    return plVariant();
  const plIReflectedTypeAccessor& accessor = m_pParent->GetTypeAccessor();
  return accessor.GetPropertyChildIndex(m_sParentProperty.GetData(), GetGuid());
}

bool plDocumentObject::IsOnHeap() const
{
  /// \todo Christopher: This crashes when the pointer is nullptr, which appears to be possible
  /// It happened for me when duplicating (CTRL+D) 2 objects 2 times then moving them and finally undoing everything
  PLASMA_ASSERT_DEV(m_pParent != nullptr,
    "Object being modified is not part of the document, e.g. may be in the undo stack instead. "
    "This could happen if within an undo / redo op some callback tries to create a new undo scope / update prefabs etc.");

  if (GetParent() == GetDocumentObjectManager()->GetRootObject())
    return true;

  auto* pProp = GetParentPropertyType();
  return pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner);
}


void plDocumentObject::HashPropertiesRecursive(const plIReflectedTypeAccessor& acc, plUInt64& uiHash, const plRTTI* pType) const
{
  // Parse parent class
  const plRTTI* pParentType = pType->GetParentType();
  if (pParentType != nullptr)
    HashPropertiesRecursive(acc, uiHash, pParentType);

  // Parse properties
  plUInt32 uiPropertyCount = pType->GetProperties().GetCount();
  for (plUInt32 i = 0; i < uiPropertyCount; ++i)
  {
    const plAbstractProperty* pProperty = pType->GetProperties()[i];

    if (pProperty->GetFlags().IsSet(plPropertyFlags::ReadOnly))
      continue;
    if (pProperty->GetAttributeByType<plTemporaryAttribute>() != nullptr)
      continue;

    if (pProperty->GetCategory() == plPropertyCategory::Member)
    {
      const plVariant var = acc.GetValue(pProperty->GetPropertyName());
      uiHash = var.ComputeHash(uiHash);
    }
    else if (pProperty->GetCategory() == plPropertyCategory::Array || pProperty->GetCategory() == plPropertyCategory::Set)
    {
      plHybridArray<plVariant, 16> keys;
      acc.GetValues(pProperty->GetPropertyName(), keys);
      for (const plVariant& var : keys)
      {
        uiHash = var.ComputeHash(uiHash);
      }
    }
    else if (pProperty->GetCategory() == plPropertyCategory::Map)
    {
      plHybridArray<plVariant, 16> keys;
      acc.GetKeys(pProperty->GetPropertyName(), keys);
      keys.Sort([](const plVariant& a, const plVariant& b) { return a.Get<plString>().Compare(b.Get<plString>()) < 0; });
      for (const plVariant& key : keys)
      {
        uiHash = key.ComputeHash(uiHash);
        plVariant value = acc.GetValue(pProperty->GetPropertyName(), key);
        uiHash = value.ComputeHash(uiHash);
      }
    }
  }
}
