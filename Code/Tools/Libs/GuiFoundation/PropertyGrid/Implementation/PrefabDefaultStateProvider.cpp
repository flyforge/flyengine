#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/PrefabDefaultStateProvider.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

plSharedPtr<plDefaultStateProvider> plPrefabDefaultStateProvider::CreateProvider(plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp)
{
  const auto* pMetaData = pObject->GetDocumentObjectManager()->GetDocument()->m_DocumentObjectMetaData.Borrow();
  plInt32 iRootDepth = 0;
  plUuid rootObjectGuid = plPrefabUtils::GetPrefabRoot(pObject, *pMetaData, &iRootDepth);
  // The root depth is taken x2 because GetPrefabRoot counts the number of parent objects while plDefaultStateProvider expects to count the properties as well.
  iRootDepth *= 2;
  // If we construct this from a property scope, the root is an additional hop away as GetPrefabRoot counts from the parent object.
  if (pProp)
    iRootDepth += 1;

  if (rootObjectGuid.IsValid())
  {
    auto pMeta = pMetaData->BeginReadMetaData(rootObjectGuid);
    PL_SCOPE_EXIT(pMetaData->EndReadMetaData(););
    plUuid objectPrefabGuid = pObject->GetGuid();
    objectPrefabGuid.RevertCombinationWithSeed(pMeta->m_PrefabSeedGuid);
    const plAbstractObjectGraph* pGraph = plPrefabCache::GetSingleton()->GetCachedPrefabGraph(pMeta->m_CreateFromPrefab);
    if (pGraph)
    {
      if (pGraph->GetNode(objectPrefabGuid) != nullptr)
      {
        // The object was found in the prefab, we can thus use its prefab counterpart to provide a default state.
        return PL_DEFAULT_NEW(plPrefabDefaultStateProvider, rootObjectGuid, pMeta->m_CreateFromPrefab, pMeta->m_PrefabSeedGuid, iRootDepth);
      }
    }
  }
  return nullptr;
}

plPrefabDefaultStateProvider::plPrefabDefaultStateProvider(const plUuid& rootObjectGuid, const plUuid& createFromPrefab, const plUuid& prefabSeedGuid, plInt32 iRootDepth)
  : m_RootObjectGuid(rootObjectGuid)
  , m_CreateFromPrefab(createFromPrefab)
  , m_PrefabSeedGuid(prefabSeedGuid)
  , m_iRootDepth(iRootDepth)
{
}

plInt32 plPrefabDefaultStateProvider::GetRootDepth() const
{
  return m_iRootDepth;
}

plColorGammaUB plPrefabDefaultStateProvider::GetBackgroundColor() const
{
  return plColorScheme::DarkUI(plColorScheme::Blue).WithAlpha(0.25f);
}

plVariant plPrefabDefaultStateProvider::GetDefaultValue(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index)
{
  const bool bIsValueType = plReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags);

  const plAbstractObjectGraph* pGraph = plPrefabCache::GetSingleton()->GetCachedPrefabGraph(m_CreateFromPrefab);
  plUuid objectPrefabGuid = pObject->GetGuid();
  objectPrefabGuid.RevertCombinationWithSeed(m_PrefabSeedGuid);
  if (pGraph)
  {
    bool bValueFound = true;
    plVariant defaultValue = plPrefabUtils::GetDefaultValue(*pGraph, objectPrefabGuid, pProp->GetPropertyName(), index, &bValueFound);
    if (!bValueFound)
    {
      return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
    }

    if (pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags) && defaultValue.IsA<plString>())
    {
      plInt64 iValue = 0;
      if (plReflectionUtils::StringToEnumeration(pProp->GetSpecificType(), defaultValue.Get<plString>(), iValue))
      {
        defaultValue = iValue;
      }
      else
      {
        defaultValue = superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
      }
    }
    else if (!bIsValueType)
    {
      // For object references we need to reverse the object GUID mapping from prefab -> instance.
      switch (pProp->GetCategory())
      {
        case plPropertyCategory::Member:
        {
          plUuid& targetGuid = defaultValue.GetWritable<plUuid>();
          targetGuid.CombineWithSeed(m_PrefabSeedGuid);
        }
        break;
        case plPropertyCategory::Array:
        case plPropertyCategory::Set:
        {
          if (index.IsValid())
          {
            plUuid& targetGuid = defaultValue.GetWritable<plUuid>();
            targetGuid.CombineWithSeed(m_PrefabSeedGuid);
          }
          else
          {
            plVariantArray& defaultValueArray = defaultValue.GetWritable<plVariantArray>();
            for (plVariant& value : defaultValueArray)
            {
              plUuid& targetGuid = value.GetWritable<plUuid>();
              targetGuid.CombineWithSeed(m_PrefabSeedGuid);
            }
          }
        }
        break;
        case plPropertyCategory::Map:
        {
          if (index.IsValid())
          {
            plUuid& targetGuid = defaultValue.GetWritable<plUuid>();
            targetGuid.CombineWithSeed(m_PrefabSeedGuid);
          }
          else
          {
            plVariantDictionary& defaultValueDict = defaultValue.GetWritable<plVariantDictionary>();
            for (auto it : defaultValueDict)
            {
              plUuid& targetGuid = it.Value().GetWritable<plUuid>();
              targetGuid.CombineWithSeed(m_PrefabSeedGuid);
            }
          }
        }
        break;
        default:
          break;
      }
    }

    if (defaultValue.IsValid())
    {
      if (defaultValue.IsString() && pProp->GetAttributeByType<plGameObjectReferenceAttribute>())
      {
        // While pretty expensive this restores the default state of game object references which are stored as strings.
        plStringView sValue = defaultValue.GetType() == plVariantType::StringView ? defaultValue.Get<plStringView>() : plStringView(defaultValue.Get<plString>().GetData());
        if (plConversionUtils::IsStringUuid(sValue))
        {
          plUuid guid = plConversionUtils::ConvertStringToUuid(sValue);
          guid.CombineWithSeed(m_PrefabSeedGuid);
          plStringBuilder sTemp;
          defaultValue = plConversionUtils::ToString(guid, sTemp).GetData();
        }
      }

      return defaultValue;
    }
  }
  return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp);
}

plStatus plPrefabDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plDeque<plAbstractGraphDiffOperation>& out_diff)
{
  plVariant defaultValue = GetDefaultValue(superPtr, pAccessor, pObject, pProp);
  plVariant currentValue;
  PL_SUCCEED_OR_RETURN(pAccessor->GetValue(pObject, pProp, currentValue));

  const plAbstractObjectGraph* pGraph = plPrefabCache::GetSingleton()->GetCachedPrefabGraph(m_CreateFromPrefab);
  plUuid objectPrefabGuid = pObject->GetGuid();
  objectPrefabGuid.RevertCombinationWithSeed(m_PrefabSeedGuid);
  if (pGraph)
  {
    // We create a sub-graph of only the parent node in both re-mapped prefab as well as from the actually object. We limit the graph to only the container property.
    auto pNode = pGraph->GetNode(objectPrefabGuid);
    plAbstractObjectGraph prefabSubGraph;
    pGraph->Clone(prefabSubGraph, pNode, [pRootNode = pNode, pRootProp = pProp](const plAbstractObjectNode* pNode, const plAbstractObjectNode::Property* pProp) {
      if (pNode == pRootNode && pProp->m_sPropertyName != pRootProp->GetPropertyName())
        return false;

      return true; //
    });

    prefabSubGraph.ReMapNodeGuids(m_PrefabSeedGuid);

    plAbstractObjectGraph instanceSubGraph;
    plDocumentObjectConverterWriter writer(&instanceSubGraph, pObject->GetDocumentObjectManager(), [pRootObject = pObject, pRootProp = pProp](const plDocumentObject* pObject, const plAbstractProperty* pProp) {
      if (pObject == pRootObject && pProp != pRootProp)
        return false;

      return true; //
    });

    writer.AddObjectToGraph(pObject);

    prefabSubGraph.CreateDiffWithBaseGraph(instanceSubGraph, out_diff);

    return plStatus(PL_SUCCESS);
  }

  return plStatus(plFmt("The object was not found in the base prefab graph."));
}
