#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/GUI/ExposedParametersDefaultStateProvider.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

plSharedPtr<plDefaultStateProvider> plExposedParametersDefaultStateProvider::CreateProvider(plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp)
{
  if (pProp)
  {
    const auto* pAttrib = pProp->GetAttributeByType<plExposedParametersAttribute>();
    if (pAttrib)
    {
      return PL_DEFAULT_NEW(plExposedParametersDefaultStateProvider, pAccessor, pObject, pProp);
    }
  }
  return nullptr;
}

plExposedParametersDefaultStateProvider::plExposedParametersDefaultStateProvider(plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp)
  : m_pObject(pObject)
  , m_pProp(pProp)
{
  PL_ASSERT_DEBUG(pProp->GetCategory() == plPropertyCategory::Map, "plExposedParametersAttribute must be on a map property");
  m_pAttrib = pProp->GetAttributeByType<plExposedParametersAttribute>();
  PL_ASSERT_DEBUG(m_pAttrib, "plExposedParametersDefaultStateProvider was created for a property that does not have the plExposedParametersAttribute.");
  m_pParameterSourceProp = pObject->GetType()->FindPropertyByName(m_pAttrib->GetParametersSource());
  PL_ASSERT_DEBUG(
    m_pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", m_pAttrib->GetParametersSource(), pObject->GetType()->GetTypeName());
}

plInt32 plExposedParametersDefaultStateProvider::GetRootDepth() const
{
  return 0;
}

plColorGammaUB plExposedParametersDefaultStateProvider::GetBackgroundColor() const
{
  // Set alpha to 0 -> color will be ignored.
  return plColorGammaUB(0, 0, 0, 0);
}

plVariant plExposedParametersDefaultStateProvider::GetDefaultValue(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index)
{
  PL_ASSERT_DEBUG(pObject == m_pObject && pProp == m_pProp, "plDefaultContainerState is only valid on the object and container it was created on.");
  plExposedParameterCommandAccessor accessor(pAccessor, pProp, m_pParameterSourceProp);
  if (index.IsValid())
  {
    const plExposedParameter* pParam = accessor.GetExposedParam(pObject, index.Get<plString>());
    if (pParam)
    {
      return pParam->m_DefaultValue;
    }
    return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
  }
  else
  {
    plVariantDictionary defaultDict;
    if (const plExposedParameters* pParams = accessor.GetExposedParams(pObject))
    {
      for (plExposedParameter* pParam : pParams->m_Parameters)
      {
        defaultDict.Insert(pParam->m_sName, pParam->m_DefaultValue);
      }
    }
    return defaultDict;
  }
}

plStatus plExposedParametersDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plDeque<plAbstractGraphDiffOperation>& out_diff)
{
  PL_REPORT_FAILURE("Unreachable code");
  return plStatus(PL_SUCCESS);
}

bool plExposedParametersDefaultStateProvider::IsDefaultValue(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index)
{
  PL_ASSERT_DEBUG(pObject == m_pObject && pProp == m_pProp, "plDefaultContainerState is only valid on the object and container it was created on.");
  plExposedParameterCommandAccessor accessor(pAccessor, pProp, m_pParameterSourceProp);

  const plVariant def = GetDefaultValue(superPtr, pAccessor, pObject, pProp, index);
  if (index.IsValid())
  {
    plVariant value;
    plStatus res = accessor.GetValue(pObject, pProp, value, index);
    // If the key is not valid, the exposed parameter is not overwritten and thus remains at the default value.
    return res.Failed() || def == value;
  }
  else
  {
    // We consider an exposed params map to be the default if it is empty.
    // We deliberately do not use the accessor here and go directly to the object storage as the passed in pAccessor could already be an plExposedParameterCommandAccessor in which case we wouldn't truly know if anything was overwritten.
    plVariant value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), index);
    return value.Get<plVariantDictionary>().GetCount() == 0;
  }
}

plStatus plExposedParametersDefaultStateProvider::RevertProperty(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index)
{
  if (!index.IsValid())
  {
    // We override the standard implementation here to just clear the array on revert to default. This is because the exposed params work as an override of the default behavior and we can safe space and time by simply not overriding anything.
    // The GUI will take care of pretending that the values are present with their default value.
    plDeque<plAbstractGraphDiffOperation> diff;
    auto& op = diff.ExpandAndGetRef();
    op.m_Node = pObject->GetGuid();
    op.m_Operation = plAbstractGraphDiffOperation::Op::PropertyChanged;
    op.m_sProperty = pProp->GetPropertyName();
    op.m_uiTypeVersion = 0;
    op.m_Value = plVariantDictionary();
    plDocumentObjectConverterReader::ApplyDiffToObject(pAccessor, pObject, diff);
    return plStatus(PL_SUCCESS);
  }
  return plDefaultStateProvider::RevertProperty(superPtr, pAccessor, pObject, pProp, index);
}
