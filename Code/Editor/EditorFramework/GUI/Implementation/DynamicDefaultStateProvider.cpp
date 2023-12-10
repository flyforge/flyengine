#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/GUI/DynamicDefaultStateProvider.h>
#include <EditorFramework/Object/ObjectPropertyPath.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Reflection/PropertyPath.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

plSharedPtr<plDefaultStateProvider> plDynamicDefaultStateProvider::CreateProvider(plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp)
{
  if (pProp)
  {
    auto* pAttrib = pProp->GetAttributeByType<plDynamicDefaultValueAttribute>();
    if (pAttrib && !plStringUtils::IsNullOrEmpty(pAttrib->GetClassProperty()))
    {
      return PLASMA_DEFAULT_NEW(plDynamicDefaultStateProvider, pAccessor, pObject, pObject, pObject, pProp, 0);
    }
  }

  plInt32 iRootDepth = 0;
  if (pProp)
    iRootDepth += 1;

  const plDocumentObject* pCurrentObject = pObject;
  while (pCurrentObject)
  {
    const plAbstractProperty* pParentProp = pCurrentObject->GetParentPropertyType();
    if (!pParentProp)
      return nullptr;

    const auto* pAttrib = pParentProp->GetAttributeByType<plDynamicDefaultValueAttribute>();
    if (pAttrib)
    {
      iRootDepth += 1;
      return PLASMA_DEFAULT_NEW(plDynamicDefaultStateProvider, pAccessor, pObject, pCurrentObject, pCurrentObject->GetParent(), pParentProp, iRootDepth);
    }
    iRootDepth += 2;
    pCurrentObject = pCurrentObject->GetParent();
  }
  return nullptr;
}

plDynamicDefaultStateProvider::plDynamicDefaultStateProvider(plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plDocumentObject* pClassObject, const plDocumentObject* pRootObject, const plAbstractProperty* pRootProp, plInt32 iRootDepth)
  : m_pObject(pObject)
  , m_pClassObject(pClassObject)
  , m_pRootObject(pRootObject)
  , m_pRootProp(pRootProp)
  , m_iRootDepth(iRootDepth)
{
  m_pAttrib = m_pRootProp->GetAttributeByType<plDynamicDefaultValueAttribute>();
  PLASMA_ASSERT_DEBUG(m_pAttrib, "plDynamicDefaultStateProvider was created for a property that does not have the plDynamicDefaultValueAttribute.");

  m_pClassType = plRTTI::FindTypeByName(m_pAttrib->GetClassType());
  PLASMA_ASSERT_DEBUG(m_pClassType, "The dynamic meta data class type '{0}' does not exist", m_pAttrib->GetClassType());

  m_pClassSourceProp = m_pRootObject->GetType()->FindPropertyByName(m_pAttrib->GetClassSource());
  PLASMA_ASSERT_DEBUG(m_pClassSourceProp, "The dynamic meta data class source '{0}' does not exist on type '{1}'", m_pAttrib->GetClassSource(), m_pRootObject->GetType()->GetTypeName());

  const bool bHasProperty = !plStringUtils::IsNullOrEmpty(m_pAttrib->GetClassProperty());
  if (!bHasProperty)
  {
    PLASMA_ASSERT_DEBUG(m_pRootProp->GetCategory() == plPropertyCategory::Member, "plDynamicDefaultValueAttribute must be on a member property if no ClassProperty is given.");
  }
  else
  {
    m_pClassProperty = m_pClassType->FindPropertyByName(m_pAttrib->GetClassProperty());

    PLASMA_ASSERT_DEBUG(m_pClassProperty, "The dynamic meta data class type '{0}' does not have a property named '{1}'", m_pAttrib->GetClassType(), m_pAttrib->GetClassProperty());
  }
}

plInt32 plDynamicDefaultStateProvider::GetRootDepth() const
{
  return m_iRootDepth;
}

plColorGammaUB plDynamicDefaultStateProvider::GetBackgroundColor() const
{
  // Set alpha to 0 -> color will be ignored.
  return plColorGammaUB(0, 0, 0, 0);
}

plVariant plDynamicDefaultStateProvider::GetDefaultValue(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index)
{
  const bool bIsValueType = plReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags);

  if (const plReflectedClass* pMeta = GetMetaInfo(pAccessor))
  {
    plPropertyPath propertyPath;
    if (CreatePath(pAccessor, pMeta, propertyPath, pObject, pProp, index).Failed())
    {
      return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
    }

    plVariant defaultValue;
    plResult res = propertyPath.ReadProperty(const_cast<plReflectedClass*>(pMeta), *pMeta->GetDynamicRTTI(), [&](void* pLeaf, const plRTTI& type, const plAbstractProperty* pNativeProp, const plVariant& index) {
      PLASMA_ASSERT_DEBUG(pProp->GetCategory() == pNativeProp->GetCategory(), "While properties don't need to match exactly, they need to be of the same category and type.");

      switch (pNativeProp->GetCategory())
      {
        case plPropertyCategory::Member:
          defaultValue = plReflectionUtils::GetMemberPropertyValue(static_cast<const plAbstractMemberProperty*>(pNativeProp), pLeaf);
          break;
        case plPropertyCategory::Array:
        {
          plVariant currentValue;
          pAccessor->GetValue(pObject, pProp, currentValue).LogFailure();
          const plVariantArray& currentArray = currentValue.Get<plVariantArray>();

          auto* pArrayProp = static_cast<const plAbstractArrayProperty*>(pNativeProp);
          if (!index.IsValid())
          {
            plVariantArray varArray;
            varArray.SetCount(pArrayProp->GetCount(pLeaf));
            for (plUInt32 i = 0; i < pArrayProp->GetCount(pLeaf); i++)
            {
              if (bIsValueType)
              {
                varArray[i] = plReflectionUtils::GetArrayPropertyValue(pArrayProp, pLeaf, i);
              }
              else
              {
                // We don't have any guid on the native object. Thus we just match the count basically and fill everything we can't find on our current object with 'nullptr', i.e. an invalid guid.
                if (i < currentArray.GetCount())
                {
                  varArray[i] = currentArray[i];
                }
                else
                {
                  varArray[i] = plUuid();
                }
              }
            }
            defaultValue = std::move(varArray);
          }
          else
          {
            if (bIsValueType)
            {
              defaultValue = plReflectionUtils::GetArrayPropertyValue(pArrayProp, pLeaf, index.ConvertTo<plInt32>());
            }
            else
            {
              plUInt32 iIndex = index.ConvertTo<plUInt32>();
              if (iIndex < currentArray.GetCount())
              {
                defaultValue = currentArray[iIndex];
              }
              else
              {
                defaultValue = plUuid();
              }
            }
          }
        }
        break;
        case plPropertyCategory::Map:
        {
          auto* pMapProp = static_cast<const plAbstractMapProperty*>(pNativeProp);

          plVariant currentValue;
          pAccessor->GetValue(pObject, pProp, currentValue).LogFailure();
          const plVariantDictionary& currentDict = currentValue.Get<plVariantDictionary>();

          if (!index.IsValid())
          {
            plHybridArray<plString, 16> keys;
            pMapProp->GetKeys(pLeaf, keys);

            plVariantDictionary varDict;
            for (auto& key : keys)
            {
              if (bIsValueType)
              {
                varDict.Insert(key, plReflectionUtils::GetMapPropertyValue(pMapProp, pLeaf, key));
              }
              else
              {
                if (auto* pValue = currentDict.GetValue(key))
                {
                  varDict.Insert(key, *pValue);
                }
                else
                {
                  varDict.Insert(key, plUuid());
                }
              }
            }
            defaultValue = std::move(varDict);
          }
          else
          {
            if (bIsValueType)
            {
              defaultValue = plReflectionUtils::GetMapPropertyValue(pMapProp, pLeaf, index.Get<plString>());
            }
            else
            {
              if (auto* pValue = currentDict.GetValue(index.Get<plString>()))
              {
                defaultValue = *pValue;
              }
              else
              {
                defaultValue = plUuid();
              }
            }
          }
        }
        break;
        default:
          PLASMA_ASSERT_NOT_IMPLEMENTED;
          break;
      }
    });

    if (res.Succeeded())
    {
      if (!DoesVariantMatchProperty(defaultValue, pProp, index))
      {
        plLog::Error("Default value '{}' does not match property '{}' at index '{}'", defaultValue, pProp->GetPropertyName(), index);
      }
      else
      {
        return defaultValue;
      }
    }
  }
  return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
}

const plReflectedClass* plDynamicDefaultStateProvider::GetMetaInfo(plObjectAccessorBase* pAccessor) const
{
  plVariant value;
  if (pAccessor->GetValue(m_pRootObject, m_pClassSourceProp, value).Succeeded())
  {
    if (value.IsA<plString>())
    {
      const auto& sValue = value.Get<plString>();
      if (const auto asset = plAssetCurator::GetSingleton()->FindSubAsset(sValue.GetData()))
      {
        return asset->m_pAssetInfo->m_Info->GetMetaInfo(m_pClassType);
      }
    }
  }

  return nullptr;
}

const plResult plDynamicDefaultStateProvider::CreatePath(plObjectAccessorBase* pAccessor, const plReflectedClass* pMeta, plPropertyPath& propertyPath, const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index)
{
  plObjectPropertyPathContext pathContext = {m_pClassProperty ? m_pRootObject : m_pClassObject, pAccessor, "Children"};

  plPropertyReference ref;
  ref.m_Object = pObject->GetGuid();
  ref.m_pProperty = pProp;
  ref.m_Index = index;

  plStringBuilder sPropPath;
  plObjectPropertyPath::CreatePropertyPath(pathContext, ref, sPropPath).LogFailure();
  if (m_pClassProperty)
  {
    sPropPath.ReplaceFirst(m_pRootProp->GetPropertyName(), m_pAttrib->GetClassProperty());
  }

  return propertyPath.InitializeFromPath(*pMeta->GetDynamicRTTI(), sPropPath);
}

plStatus plDynamicDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plDeque<plAbstractGraphDiffOperation>& out_diff)
{
  if (const plReflectedClass* pMeta = GetMetaInfo(pAccessor))
  {
    plPropertyPath propertyPath;
    if (CreatePath(pAccessor, pMeta, propertyPath, pObject, pProp).Failed())
    {
      return plStatus(plFmt("Failed to find root object in object graph"));
    }

    plAbstractObjectGraph prefabSubGraph;
    plAbstractObjectNode* pPrefabSubRoot = nullptr;
    {
      // Create a graph of the native object, skipping all other properties except for the container in question.
      plRttiConverterContext context;
      plString sRootPropertyName = pProp->GetPropertyName();
      // If we are dealing with an attributed container and pObject is its parent, then the root container property name can differ between the meta info and the target object so we have to rename it later to make the two graphs match.
      if (m_pClassProperty && pObject == m_pRootObject)
      {
        sRootPropertyName = m_pAttrib->GetClassProperty();
      }

      void* pNativeRootObject = nullptr;
      plRttiConverterWriter rttiConverter(&prefabSubGraph, &context, [&](const void* pObject, const plAbstractProperty* pCurrentProp) {
        if (pNativeRootObject == pObject && pCurrentProp->GetPropertyName() != sRootPropertyName)
          return false;
        return true;
      });

      auto WriteObject = [&](void* pLeafObject, const plRTTI& leafType, const plAbstractProperty* pLeafProp, const plVariant& index) {
        pNativeRootObject = pLeafObject;
        context.RegisterObject(pObject->GetGuid(), &leafType, pLeafObject);
        pPrefabSubRoot = rttiConverter.AddObjectToGraph(&leafType, pLeafObject);
        pPrefabSubRoot->RenameProperty(sRootPropertyName, pProp->GetPropertyName());
      };

      plVariant defaultValue;
      plResult res = propertyPath.ReadProperty(const_cast<plReflectedClass*>(pMeta), *pMeta->GetDynamicRTTI(), WriteObject);
      if (res.Failed())
      {
        return plStatus(plFmt("Failed to find root object in object graph"));
      }
    }

    // Create graph from current object with only the container to be reverted present.
    plAbstractObjectGraph instanceSubGraph;
    plAbstractObjectNode* pInstanceSubRoot = nullptr;
    {
      plDocumentObjectConverterWriter writer(&instanceSubGraph, pObject->GetDocumentObjectManager(), [pRootObject = pObject, pRootProp = pProp](const plDocumentObject* pObject, const plAbstractProperty* pProp) {
        if (pObject == pRootObject && pProp != pRootProp)
          return false;
        return true;
      });
      pInstanceSubRoot = writer.AddObjectToGraph(pObject);
    }

    // Make the native graph match the guids of pObject graph.
    pPrefabSubRoot->SetType(pInstanceSubRoot->GetType());
    prefabSubGraph.ReMapNodeGuidsToMatchGraph(pPrefabSubRoot, instanceSubGraph, pInstanceSubRoot);
    prefabSubGraph.CreateDiffWithBaseGraph(instanceSubGraph, out_diff);
    return plStatus(PLASMA_SUCCESS);
  }
  return superPtr[0]->CreateRevertContainerDiff(superPtr.GetSubArray(1), pAccessor, pObject, pProp, out_diff);
}
