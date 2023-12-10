#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/AttributeDefaultStateProvider.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

static plSharedPtr<plDefaultStateProvider> g_pAttributeDefaultStateProvider = PLASMA_DEFAULT_NEW(plAttributeDefaultStateProvider);
plSharedPtr<plDefaultStateProvider> plAttributeDefaultStateProvider::CreateProvider(plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp)
{
  // One global instance handles all. No need to create a new instance per request as no state need to be tracked.
  return g_pAttributeDefaultStateProvider;
}

plInt32 plAttributeDefaultStateProvider::GetRootDepth() const
{
  return -1;
}

plColorGammaUB plAttributeDefaultStateProvider::GetBackgroundColor() const
{
  // Set alpha to 0 -> color will be ignored.
  return plColorGammaUB(0, 0, 0, 0);
}

plVariant plAttributeDefaultStateProvider::GetDefaultValue(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index)
{
  if (!pProp->GetFlags().IsSet(plPropertyFlags::Pointer) && pProp->GetFlags().IsSet(plPropertyFlags::Class) && pProp->GetCategory() == plPropertyCategory::Member && !plReflectionUtils::IsValueType(pProp))
  {
    // An embedded class that is not a value type can never change its value.
    plVariant value;
    pAccessor->GetValue(pObject, pProp, value).LogFailure();
    return value;
  }
  return plReflectionUtils::GetDefaultValue(pProp, index);
}

plStatus plAttributeDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plDeque<plAbstractGraphDiffOperation>& out_diff)
{
  auto RemoveObject = [&](const plUuid& object) {
    auto& op = out_diff.ExpandAndGetRef();
    op.m_Node = object;
    op.m_Operation = plAbstractGraphDiffOperation::Op::NodeRemoved;
    op.m_uiTypeVersion = 0;
    op.m_sProperty = pAccessor->GetObject(object)->GetType()->GetTypeName();
  };

  auto SetProperty = [&](const plVariant& newValue) {
    auto& op = out_diff.ExpandAndGetRef();
    op.m_Node = pObject->GetGuid();
    op.m_Operation = plAbstractGraphDiffOperation::Op::PropertyChanged;
    op.m_uiTypeVersion = 0;
    op.m_sProperty = pProp->GetPropertyName();
    op.m_Value = newValue;
  };

  plVariant currentValue;
  pAccessor->GetValue(pObject, pProp, currentValue).LogFailure();
  switch (pProp->GetCategory())
  {
    case plPropertyCategory::Member:
    {
      const auto& objectGuid = currentValue.Get<plUuid>();
      if (objectGuid.IsValid())
      {
        RemoveObject(objectGuid);
        SetProperty(plUuid());
      }
    }
    break;
    case plPropertyCategory::Array:
    case plPropertyCategory::Set:
    {
      const auto& currentArray = currentValue.Get<plVariantArray>();
      for (plInt32 i = (plInt32)currentArray.GetCount() - 1; i >= 0; i--)
      {
        const auto& objectGuid = currentArray[i].Get<plUuid>();
        if (objectGuid.IsValid())
        {
          RemoveObject(objectGuid);
        }
      }
      SetProperty(plVariantArray());
    }
    break;
    case plPropertyCategory::Map:
    {
      const auto& currentArray = currentValue.Get<plVariantDictionary>();
      for (auto val : currentArray)
      {
        const auto& objectGuid = val.Value().Get<plUuid>();
        if (objectGuid.IsValid())
        {
          RemoveObject(objectGuid);
        }
      }
      SetProperty(plVariantDictionary());
    }
    break;
    default:
      PLASMA_REPORT_FAILURE("Unreachable code");
      break;
  }
  return plStatus(PLASMA_SUCCESS);
}
