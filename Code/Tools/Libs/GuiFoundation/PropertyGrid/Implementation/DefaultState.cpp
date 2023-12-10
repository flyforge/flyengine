#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GuiFoundation/PropertyGrid/AttributeDefaultStateProvider.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <GuiFoundation/PropertyGrid/PrefabDefaultStateProvider.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, DefaultState)
  ON_CORESYSTEMS_STARTUP
  {
    plDefaultState::RegisterDefaultStateProvider(plAttributeDefaultStateProvider::CreateProvider);
    plDefaultState::RegisterDefaultStateProvider(plPrefabDefaultStateProvider::CreateProvider);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plDefaultState::UnregisterDefaultStateProvider(plAttributeDefaultStateProvider::CreateProvider);
    plDefaultState::UnregisterDefaultStateProvider(plPrefabDefaultStateProvider::CreateProvider);
  }
PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on


plDynamicArray<plDefaultState::CreateStateProviderFunc> plDefaultState::s_Factories;

void plDefaultState::RegisterDefaultStateProvider(CreateStateProviderFunc func)
{
  s_Factories.PushBack(func);
}

void plDefaultState::UnregisterDefaultStateProvider(CreateStateProviderFunc func)
{
  s_Factories.RemoveAndCopy(func);
}

//////////////////////////////////////////////////////////////////////////

plDefaultObjectState::plDefaultObjectState(plObjectAccessorBase* pAccessor, const plArrayPtr<plPropertySelection> selection)
{
  m_pAccessor = pAccessor;
  m_Selection = selection;
  m_Providers.Reserve(m_Selection.GetCount());
  for (const plPropertySelection& sel : m_Selection)
  {
    auto& pProviders = m_Providers.ExpandAndGetRef();
    for (auto& func : plDefaultState::s_Factories)
    {
      plSharedPtr<plDefaultStateProvider> pProvider = func(pAccessor, sel.m_pObject, nullptr);
      if (pProvider != nullptr)
      {
        pProviders.PushBack(std::move(pProvider));
      }
      pProviders.Sort([](const plSharedPtr<plDefaultStateProvider>& pA, const plSharedPtr<plDefaultStateProvider>& pB) -> bool { return pA->GetRootDepth() > pB->GetRootDepth(); });
    }
  }
}

plColorGammaUB plDefaultObjectState::GetBackgroundColor() const
{
  return m_Providers[0][0]->GetBackgroundColor();
}

plString plDefaultObjectState::GetStateProviderName() const
{
  return m_Providers[0][0]->GetStateProviderName();
}

bool plDefaultObjectState::IsDefaultValue(const char* szProperty) const
{
  const plAbstractProperty* pProp = m_Selection[0].m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(szProperty);
  return IsDefaultValue(pProp);
}

bool plDefaultObjectState::IsDefaultValue(const plAbstractProperty* pProp) const
{
  const plUInt32 uiObjects = m_Providers.GetCount();
  for (plUInt32 i = 0; i < uiObjects; i++)
  {
    plDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    const bool bNewDefault = m_Providers[i][0]->IsDefaultValue(super, m_pAccessor, m_Selection[i].m_pObject, pProp);
    if (!bNewDefault)
      return false;
  }
  return true;
}

plStatus plDefaultObjectState::RevertProperty(const char* szProperty)
{
  const plAbstractProperty* pProp = m_Selection[0].m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(szProperty);
  return RevertProperty(pProp);
}

plStatus plDefaultObjectState::RevertProperty(const plAbstractProperty* pProp)
{
  const plUInt32 uiObjects = m_Providers.GetCount();
  for (plUInt32 i = 0; i < uiObjects; i++)
  {
    plDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    plStatus res = m_Providers[i][0]->RevertProperty(super, m_pAccessor, m_Selection[i].m_pObject, pProp);
    if (res.Failed())
      return res;
  }
  return plStatus(PLASMA_SUCCESS);
}

plStatus plDefaultObjectState::RevertObject()
{
  const plUInt32 uiObjects = m_Providers.GetCount();
  for (plUInt32 i = 0; i < uiObjects; i++)
  {
    plDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);

    plHybridArray<const plAbstractProperty*, 32> properties;
    m_Selection[i].m_pObject->GetType()->GetAllProperties(properties);
    for (auto pProp : properties)
    {
      if (pProp->GetFlags().IsAnySet(plPropertyFlags::Hidden | plPropertyFlags::ReadOnly))
        continue;
      plStatus res = m_Providers[i][0]->RevertProperty(super, m_pAccessor, m_Selection[i].m_pObject, pProp);
      if (res.Failed())
        return res;
    }
  }
  return plStatus(PLASMA_SUCCESS);
}

plVariant plDefaultObjectState::GetDefaultValue(const char* szProperty, plUInt32 uiSelectionIndex) const
{
  const plAbstractProperty* pProp = m_Selection[0].m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(szProperty);
  return GetDefaultValue(pProp, uiSelectionIndex);
}

plVariant plDefaultObjectState::GetDefaultValue(const plAbstractProperty* pProp, plUInt32 uiSelectionIndex) const
{
  PLASMA_ASSERT_DEBUG(uiSelectionIndex < m_Selection.GetCount(), "Selection index is out of bounds.");
  plDefaultStateProvider::SuperArray super = m_Providers[uiSelectionIndex].GetArrayPtr().GetSubArray(1);
  return m_Providers[uiSelectionIndex][0]->GetDefaultValue(super, m_pAccessor, m_Selection[uiSelectionIndex].m_pObject, pProp);
}

//////////////////////////////////////////////////////////////////////////

plDefaultContainerState::plDefaultContainerState(plObjectAccessorBase* pAccessor, const plArrayPtr<plPropertySelection> selection, const char* szProperty)
{
  m_pAccessor = pAccessor;
  m_Selection = selection;
  // We assume selections can only contain objects of the same (base) type.
  m_pProp = szProperty ? selection[0].m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(szProperty) : nullptr;
  m_Providers.Reserve(m_Selection.GetCount());
  for (const plPropertySelection& sel : m_Selection)
  {
    auto& pProviders = m_Providers.ExpandAndGetRef();
    for (auto& func : plDefaultState::s_Factories)
    {
      plSharedPtr<plDefaultStateProvider> pProvider = func(pAccessor, sel.m_pObject, m_pProp);
      if (pProvider != nullptr)
      {
        pProviders.PushBack(std::move(pProvider));
      }
      pProviders.Sort([](const plSharedPtr<plDefaultStateProvider>& pA, const plSharedPtr<plDefaultStateProvider>& pB) -> bool { return pA->GetRootDepth() > pB->GetRootDepth(); });
    }
  }
}

plColorGammaUB plDefaultContainerState::GetBackgroundColor() const
{
  return m_Providers[0][0]->GetBackgroundColor();
}

plString plDefaultContainerState::GetStateProviderName() const
{
  return m_Providers[0][0]->GetStateProviderName();
}

bool plDefaultContainerState::IsDefaultElement(plVariant index) const
{
  const plUInt32 uiObjects = m_Providers.GetCount();
  for (plUInt32 i = 0; i < uiObjects; i++)
  {
    plDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    PLASMA_ASSERT_DEBUG(index.IsValid() || m_Selection[i].m_Index.IsValid(), "If plDefaultContainerState is constructed without giving an indices in the selection, one must be provided on the IsDefaultElement call.");
    const bool bNewDefault = m_Providers[i][0]->IsDefaultValue(super, m_pAccessor, m_Selection[i].m_pObject, m_pProp, index.IsValid() ? index : m_Selection[i].m_Index);
    if (!bNewDefault)
      return false;
  }
  return true;
}

bool plDefaultContainerState::IsDefaultContainer() const
{
  const plUInt32 uiObjects = m_Providers.GetCount();
  for (plUInt32 i = 0; i < uiObjects; i++)
  {
    plDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    const bool bNewDefault = m_Providers[i][0]->IsDefaultValue(super, m_pAccessor, m_Selection[i].m_pObject, m_pProp);
    if (!bNewDefault)
      return false;
  }
  return true;
}

plStatus plDefaultContainerState::RevertElement(plVariant index)
{
  const plUInt32 uiObjects = m_Providers.GetCount();
  for (plUInt32 i = 0; i < uiObjects; i++)
  {
    plDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    PLASMA_ASSERT_DEBUG(index.IsValid() || m_Selection[i].m_Index.IsValid(), "If plDefaultContainerState is constructed without giving an indices in the selection, one must be provided on the RevertElement call.");
    plStatus res = m_Providers[i][0]->RevertProperty(super, m_pAccessor, m_Selection[i].m_pObject, m_pProp, index.IsValid() ? index : m_Selection[i].m_Index);
    if (res.Failed())
      return res;
  }
  return plStatus(PLASMA_SUCCESS);
}

plStatus plDefaultContainerState::RevertContainer()
{
  const plUInt32 uiObjects = m_Providers.GetCount();
  for (plUInt32 i = 0; i < uiObjects; i++)
  {
    plDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    plStatus res = m_Providers[i][0]->RevertProperty(super, m_pAccessor, m_Selection[i].m_pObject, m_pProp);
    if (res.Failed())
      return res;
  }
  return plStatus(PLASMA_SUCCESS);
}

plVariant plDefaultContainerState::GetDefaultElement(plVariant index, plUInt32 uiSelectionIndex) const
{
  PLASMA_ASSERT_DEBUG(uiSelectionIndex < m_Selection.GetCount(), "Selection index is out of bounds.");
  plDefaultStateProvider::SuperArray super = m_Providers[uiSelectionIndex].GetArrayPtr().GetSubArray(1);
  return m_Providers[uiSelectionIndex][0]->GetDefaultValue(super, m_pAccessor, m_Selection[uiSelectionIndex].m_pObject, m_pProp, index);
}

plVariant plDefaultContainerState::GetDefaultContainer(plUInt32 uiSelectionIndex) const
{
  PLASMA_ASSERT_DEBUG(uiSelectionIndex < m_Selection.GetCount(), "Selection index is out of bounds.");
  plDefaultStateProvider::SuperArray super = m_Providers[uiSelectionIndex].GetArrayPtr().GetSubArray(1);
  return m_Providers[uiSelectionIndex][0]->GetDefaultValue(super, m_pAccessor, m_Selection[uiSelectionIndex].m_pObject, m_pProp);
}

//////////////////////////////////////////////////////////////////////////


bool plDefaultStateProvider::IsDefaultValue(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index)
{
  const plVariant def = GetDefaultValue(superPtr, pAccessor, pObject, pProp, index);
  plVariant value;
  pAccessor->GetValue(pObject, pProp, value, index).LogFailure();

  const bool bIsValueType = plReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags);
  if (index.IsValid() && !bIsValueType)
  {
    //#TODO we do not support reverting entire objects just yet.
    return true;
  }

  return def == value;
}

plStatus plDefaultStateProvider::RevertProperty(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index)
{
  const bool bIsValueType = plReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags);
  if (!bIsValueType)
  {
    PLASMA_ASSERT_DEBUG(!index.IsValid(), "Reverting non-value type container elements is not supported yet. IsDefaultValue should have returned true to prevent this call from being allowed.");

    return RevertObjectContainer(superPtr, pAccessor, pObject, pProp);
  }

  plDeque<plAbstractGraphDiffOperation> diff;
  auto& op = diff.ExpandAndGetRef();
  op.m_Node = pObject->GetGuid();
  op.m_Operation = plAbstractGraphDiffOperation::Op::PropertyChanged;
  op.m_sProperty = pProp->GetPropertyName();
  op.m_uiTypeVersion = 0;
  if (index.IsValid())
  {
    plVariant def = GetDefaultValue(superPtr, pAccessor, pObject, pProp, index);
    switch (pProp->GetCategory())
    {
      case plPropertyCategory::Array:
      case plPropertyCategory::Set:
      {
        PLASMA_ASSERT_DEBUG(index.CanConvertTo<plInt32>(), "Array / Set indices must be integers.");
        PLASMA_SUCCEED_OR_RETURN(pAccessor->GetValue(pObject, pProp, op.m_Value));
        PLASMA_ASSERT_DEBUG(op.m_Value.IsA<plVariantArray>(), "");

        plVariantArray& currentValue2 = op.m_Value.GetWritable<plVariantArray>();
        currentValue2[index.ConvertTo<plUInt32>()] = def;
      }
      break;
      case plPropertyCategory::Map:
      {
        PLASMA_ASSERT_DEBUG(index.IsString(), "Map indices must be strings.");
        PLASMA_SUCCEED_OR_RETURN(pAccessor->GetValue(pObject, pProp, op.m_Value));
        PLASMA_ASSERT_DEBUG(op.m_Value.IsA<plVariantDictionary>(), "");

        plVariantDictionary& currentValue2 = op.m_Value.GetWritable<plVariantDictionary>();
        currentValue2[index.ConvertTo<plString>()] = def;
      }
      break;
      default:
        break;
    }
  }
  else
  {
    plVariant def = GetDefaultValue(superPtr, pAccessor, pObject, pProp, index);
    op.m_Value = def;
  }

  plDocumentObjectConverterReader::ApplyDiffToObject(pAccessor, pObject, diff);
  return plStatus(PLASMA_SUCCESS);
}

plStatus plDefaultStateProvider::RevertObjectContainer(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp)
{
  plDeque<plAbstractGraphDiffOperation> diff;
  plStatus res = CreateRevertContainerDiff(superPtr, pAccessor, pObject, pProp, diff);
  if (res.Succeeded())
  {
    plDocumentObjectConverterReader::ApplyDiffToObject(pAccessor, pObject, diff);
  }
  return res;
}

bool plDefaultStateProvider::DoesVariantMatchProperty(const plVariant& value, const plAbstractProperty* pProp, plVariant index)
{
  const bool bIsValueType = plReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags);

  if (pProp->GetSpecificType() == plGetStaticRTTI<plVariant>())
    return true;

  auto MatchesElementType = [&](const plVariant& value2) -> bool {
    if (pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags))
    {
      return value2.IsNumber() && !value2.IsFloatingPoint();
    }
    else if (pProp->GetFlags().IsAnySet(plPropertyFlags::StandardType))
    {
      return value2.CanConvertTo(pProp->GetSpecificType()->GetVariantType());
    }
    else if (bIsValueType)
    {
      return value2.GetReflectedType() == pProp->GetSpecificType();
    }
    else
    {
      return value2.IsA<plUuid>();
    }
  };

  switch (pProp->GetCategory())
  {
    case plPropertyCategory::Member:
    {
      return MatchesElementType(value);
    }
    break;
    case plPropertyCategory::Array:
    case plPropertyCategory::Set:
    {
      if (index.IsValid())
      {
        return MatchesElementType(value);
      }
      else
      {
        if (value.IsA<plVariantArray>())
        {
          const plVariantArray& valueArray = value.Get<plVariantArray>();
          return std::all_of(cbegin(valueArray), cend(valueArray), MatchesElementType);
        }
      }
    }
    break;
    case plPropertyCategory::Map:
    {
      if (index.IsValid())
      {
        return MatchesElementType(value);
      }
      else
      {
        if (value.IsA<plVariantDictionary>())
        {
          const plVariantDictionary& valueDict = value.Get<plVariantDictionary>();
          return std::all_of(cbegin(valueDict), cend(valueDict), [&](const auto& it) { return MatchesElementType(it.Value()); });
        }
      }
    }
    break;
    default:
      break;
  }
  return false;
}
