#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <ToolsFoundation/Object/DocumentObjectVisitor.h>

plStatus plObjectPropertyPath::CreatePath(const plObjectPropertyPathContext& context, const plPropertyReference& prop,
  plStringBuilder& ref_sObjectSearchSequence, plStringBuilder& ref_sComponentType, plStringBuilder& ref_sPropertyPath)
{
  PLASMA_ASSERT_DEV(context.m_pAccessor && context.m_pContextObject && !context.m_sRootProperty.IsEmpty(), "All context fields must be valid.");
  const plRTTI* pObjType = plGetStaticRTTI<plGameObject>();

  const plAbstractProperty* pName = pObjType->FindPropertyByName("Name");
  const plDocumentObject* pObject = context.m_pAccessor->GetObjectManager()->GetObject(prop.m_Object);
  if (!pObject || !prop.m_pProperty)
    return plStatus(PLASMA_FAILURE);

  {
    // Build property part of the path from the next parent node / component.
    pObject = FindParentNodeComponent(pObject);
    if (!pObject)
      return plStatus("No parent node or component found.");
    plObjectPropertyPathContext context2 = context;
    context2.m_pContextObject = pObject;
    plStatus res = CreatePropertyPath(context2, prop, ref_sPropertyPath);
    if (res.Failed())
      return res;
  }

  {
    // Component part
    ref_sComponentType.Clear();
    if (pObject->GetType()->IsDerivedFrom(plGetStaticRTTI<plComponent>()))
    {
      ref_sComponentType = pObject->GetType()->GetTypeName();
      pObject = pObject->GetParent();
    }
  }

  // Node path
  while (pObject != context.m_pContextObject)
  {
    if (pObject == nullptr)
    {
      ref_sObjectSearchSequence.Clear();
      ref_sComponentType.Clear();
      ref_sPropertyPath.Clear();
      return plStatus("Property is not under the given context object, no path exists.");
    }

    if (pObject->GetType() == plGetStaticRTTI<plGameObject>())
    {
      plString sName = context.m_pAccessor->Get<plString>(pObject, pName);
      if (!sName.IsEmpty())
      {
        if (!ref_sObjectSearchSequence.IsEmpty())
          ref_sObjectSearchSequence.Prepend("/");
        ref_sObjectSearchSequence.Prepend(sName);
      }
    }
    else
    {
      ref_sObjectSearchSequence.Clear();
      ref_sComponentType.Clear();
      ref_sPropertyPath.Clear();
      return plStatus(plFmt("Only plGameObject objects should be found in the hierarchy, found '{0}' instead.", pObject->GetType()->GetTypeName()));
    }

    pObject = pObject->GetParent();
  }
  return plStatus(PLASMA_SUCCESS);
}

plStatus plObjectPropertyPath::CreatePropertyPath(
  const plObjectPropertyPathContext& context, const plPropertyReference& prop, plStringBuilder& out_sPropertyPath)
{
  PLASMA_ASSERT_DEV(context.m_pAccessor && context.m_pContextObject && !context.m_sRootProperty.IsEmpty(), "All context fields must be valid.");
  const plDocumentObject* pObject = context.m_pAccessor->GetObjectManager()->GetObject(prop.m_Object);
  if (!pObject || !prop.m_pProperty)
    return plStatus(PLASMA_FAILURE);

  out_sPropertyPath.Clear();
  plStatus res = PrependProperty(pObject, prop.m_pProperty, prop.m_Index, out_sPropertyPath);
  if (res.Failed())
    return res;

  while (pObject != context.m_pContextObject)
  {
    plStatus result = PrependProperty(pObject->GetParent(), pObject->GetParentPropertyType(), pObject->GetPropertyIndex(), out_sPropertyPath);
    if (result.Failed())
      return result;

    pObject = pObject->GetParent();
  }
  return plStatus(PLASMA_SUCCESS);
}

plStatus plObjectPropertyPath::ResolvePath(const plObjectPropertyPathContext& context, plDynamicArray<plPropertyReference>& ref_keys,
  const char* szObjectSearchSequence, const char* szComponentType, const char* szPropertyPath)
{
  PLASMA_ASSERT_DEV(context.m_pAccessor && context.m_pContextObject && !context.m_sRootProperty.IsEmpty(), "All context fields must be valid.");
  ref_keys.Clear();
  const plDocumentObject* pContext = context.m_pContextObject;
  plDocumentObjectVisitor visitor(context.m_pAccessor->GetObjectManager(), "Children", context.m_sRootProperty);
  plHybridArray<const plDocumentObject*, 8> input;
  input.PushBack(pContext);
  plHybridArray<const plDocumentObject*, 8> output;

  // Find objects that match the search path
  plStringBuilder sObjectSearchSequence = szObjectSearchSequence;
  plHybridArray<plStringView, 4> names;
  sObjectSearchSequence.Split(false, names, "/");
  for (const plStringView& sName : names)
  {
    for (const plDocumentObject* pObj : input)
    {
      visitor.Visit(pObj, false, [&output, &sName](const plDocumentObject* pObject) -> bool {
        const auto& sObjectName = pObject->GetTypeAccessor().GetValue("Name").Get<plString>();
        if (sObjectName == sName)
        {
          output.PushBack(pObject);
          return false;
        }
        return true; //
      });
    }
    input.Clear();
    input.Swap(output);
  }

  if (input.IsEmpty())
    return plStatus(plFmt("ObjectSearchSequence: '{}' could not be resolved", szObjectSearchSequence));

  // Test found objects for component
  for (const plDocumentObject* pObject : input)
  {
    // Could also be the root object in which case we found nothing.
    if (pObject->GetType() == plGetStaticRTTI<plGameObject>())
    {
      if (plStringUtils::IsNullOrEmpty(szComponentType))
      {
        // We are animating the game object directly
        output.PushBack(pObject);
      }
      else
      {
        const plInt32 iComponents = pObject->GetTypeAccessor().GetCount("Components");
        for (plInt32 i = 0; i < iComponents; i++)
        {
          plVariant value = pObject->GetTypeAccessor().GetValue("Components", i);
          auto pChild = context.m_pAccessor->GetObjectManager()->GetObject(value.Get<plUuid>());
          if (pChild->GetType()->GetTypeName() == szComponentType)
          {
            output.PushBack(pChild);
            continue; // #TODO: break on found component?
          }
        }
      }
    }
  }
  input.Clear();
  input.Swap(output);

  if (input.IsEmpty())
    return plStatus(plFmt("Component '{}' not found on the search path '{}'", szComponentType, szObjectSearchSequence));

  plStatus lastError = plResult(PLASMA_FAILURE);
  // Test found objects / components for property
  for (const plDocumentObject* pObject : input)
  {
    plObjectPropertyPathContext context2 = context;
    context2.m_pContextObject = pObject;
    plPropertyReference key;
    plStatus res = ResolvePropertyPath(context2, szPropertyPath, key);
    if (res.Succeeded())
    {
      ref_keys.PushBack(key);
    }

    if (lastError.Failed())
    {
      lastError = res;
    }
  }
  return lastError;
}

plStatus plObjectPropertyPath::ResolvePropertyPath(
  const plObjectPropertyPathContext& context, const char* szPropertyPath, plPropertyReference& out_key)
{
  PLASMA_ASSERT_DEV(context.m_pAccessor && context.m_pContextObject && szPropertyPath != nullptr, "All context fields must be valid.");
  const plDocumentObject* pObject = context.m_pContextObject;
  plStringBuilder sPath = szPropertyPath;
  plHybridArray<plStringView, 3> parts;
  sPath.Split(false, parts, "/");
  for (plUInt32 i = 0; i < parts.GetCount(); i++)
  {
    plStringBuilder sPart = parts[i];
    plHybridArray<plStringBuilder, 2> parts2;
    sPart.Split(false, parts2, "[", "]");
    if (parts2.GetCount() == 0 || parts2.GetCount() > 2)
    {
      return plStatus(plFmt("Malformed property path part: {0}", sPart));
    }
    const plAbstractProperty* pProperty = pObject->GetType()->FindPropertyByName(parts2[0]);
    if (!pProperty)
      return plStatus(plFmt("Property not found: {0}", parts2[0]));
    plVariant index;
    if (parts2.GetCount() == 2)
    {
      plInt32 iIndex = 0;
      if (plConversionUtils::StringToInt(parts2[1], iIndex).Succeeded())
      {
        index = iIndex; // Array index
      }
      else
      {
        index = parts2[1].GetData(); // Map index
      }
    }

    plVariant value;
    plStatus res;
    if (const plExposedParametersAttribute* pAttrib = pProperty->GetAttributeByType<plExposedParametersAttribute>())
    {
      const plAbstractProperty* pParameterSourceProp = pObject->GetType()->FindPropertyByName(pAttrib->GetParametersSource());
      PLASMA_ASSERT_DEV(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", pAttrib->GetParametersSource(),
        pObject->GetType()->GetTypeName());
      plExposedParameterCommandAccessor proxy(context.m_pAccessor, pProperty, pParameterSourceProp);
      res = proxy.GetValue(pObject, pProperty, value, index);
    }
    else
    {
      res = context.m_pAccessor->GetValue(pObject, pProperty, value, index);
    }

    if (res.Failed())
      return res;

    if (i == parts.GetCount() - 1)
    {
      out_key.m_Object = pObject->GetGuid();
      out_key.m_pProperty = pProperty;
      out_key.m_Index = index;
      return plStatus(PLASMA_SUCCESS);
    }
    else
    {
      if (value.IsA<plUuid>())
      {
        plUuid id = value.Get<plUuid>();
        pObject = context.m_pAccessor->GetObjectManager()->GetObject(id);
      }
      else
      {
        return plStatus(plFmt("Property '{0}' of type '{1}' is not an object and can't be traversed further.", pProperty->GetPropertyName(),
          pProperty->GetSpecificType()->GetTypeName()));
      }
    }
  }
  return plStatus(PLASMA_FAILURE);
}

plStatus plObjectPropertyPath::PrependProperty(
  const plDocumentObject* pObject, const plAbstractProperty* pProperty, plVariant index, plStringBuilder& out_sPropertyPath)
{
  switch (pProperty->GetCategory())
  {
    case plPropertyCategory::Enum::Member:
    {
      if (!out_sPropertyPath.IsEmpty())
        out_sPropertyPath.Prepend("/");
      out_sPropertyPath.Prepend(pProperty->GetPropertyName());
      return plStatus(PLASMA_SUCCESS);
    }
    case plPropertyCategory::Enum::Array:
    case plPropertyCategory::Enum::Map:
    {
      if (!out_sPropertyPath.IsEmpty())
        out_sPropertyPath.Prepend("/");
      if (index.IsValid())
        out_sPropertyPath.PrependFormat("{0}[{1}]", pProperty->GetPropertyName(), index);
      else
        out_sPropertyPath.PrependFormat("{0}", pProperty->GetPropertyName());
      return plStatus(PLASMA_SUCCESS);
    }
    default:
      return plStatus(plFmt(
        "The property '{0}' of category '{1}' which is not supported in property paths", pProperty->GetPropertyName(), pProperty->GetCategory()));
  }
}

const plDocumentObject* plObjectPropertyPath::FindParentNodeComponent(const plDocumentObject* pObject)
{
  const plRTTI* pObjType = plGetStaticRTTI<plGameObject>();
  const plRTTI* pCompType = plGetStaticRTTI<plComponent>();
  const plDocumentObject* pObj = pObject;
  while (pObj != nullptr)
  {
    if (pObj->GetType() == pObjType)
    {
      return pObj;
    }
    else if (pObj->GetType()->IsDerivedFrom(pCompType))
    {
      return pObj;
    }
    pObj = pObj->GetParent();
  }
  return nullptr;
}
