#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/BaseActions.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plNamedAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCategoryAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMenuAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDynamicMenuAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDynamicActionAndMenuAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plEnumerationMenuAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plButtonAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSliderAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plDynamicActionAndMenuAction::plDynamicActionAndMenuAction(const plActionContext& context, const char* szName, const char* szIconPath)
  : plDynamicMenuAction(context, szName, szIconPath)
{
  m_bEnabled = true;
  m_bVisible = true;
}

plEnumerationMenuAction::plEnumerationMenuAction(const plActionContext& context, const char* szName, const char* szIconPath)
  : plDynamicMenuAction(context, szName, szIconPath)
{
  m_pEnumerationType = nullptr;
}

void plEnumerationMenuAction::InitEnumerationType(const plRTTI* pEnumerationType)
{
  m_pEnumerationType = pEnumerationType;
}

void plEnumerationMenuAction::GetEntries(plHybridArray<plDynamicMenuAction::Item, 16>& out_Entries)
{
  out_Entries.Clear();
  out_Entries.Reserve(m_pEnumerationType->GetProperties().GetCount() - 1);
  plInt64 iCurrentValue = plReflectionUtils::MakeEnumerationValid(m_pEnumerationType, GetValue());

  // sort entries by group / category
  // categories appear in the order in which they are used on the reflected properties
  // within each category, items are sorted by 'order'
  // all items that have the same 'order' are sorted alphabetically by display string

  plStringBuilder sCurGroup;
  float fPrevOrder = -1;
  struct ItemWithOrder
  {
    float m_fOrder = -1;
    plDynamicMenuAction::Item m_Item;

    bool operator<(const ItemWithOrder& rhs) const
    {
      if (m_fOrder == rhs.m_fOrder)
      {
        return m_Item.m_sDisplay < rhs.m_Item.m_sDisplay;
      }

      return m_fOrder < rhs.m_fOrder;
    }
  };

  plHybridArray<ItemWithOrder, 16> unsortedItems;

  auto appendToOutput = [&]() {
    if (unsortedItems.IsEmpty())
      return;

    unsortedItems.Sort();

    if (!out_Entries.IsEmpty())
    {
      // add a separator between groups
      out_Entries.ExpandAndGetRef().m_ItemFlags.Add(plDynamicMenuAction::Item::ItemFlags::Separator);
    }

    for (const auto& sortedItem : unsortedItems)
    {
      out_Entries.PushBack(sortedItem.m_Item);
    }

    unsortedItems.Clear();
  };

  for (auto pProp : m_pEnumerationType->GetProperties().GetSubArray(1))
  {
    if (pProp->GetCategory() == plPropertyCategory::Constant)
    {
      if (const plGroupAttribute* pGroup = pProp->GetAttributeByType<plGroupAttribute>())
      {
        if (sCurGroup != pGroup->GetGroup())
        {
          sCurGroup = pGroup->GetGroup();

          appendToOutput();
        }

        fPrevOrder = pGroup->GetOrder();
      }

      ItemWithOrder& newItem = unsortedItems.ExpandAndGetRef();
      newItem.m_fOrder = fPrevOrder;
      auto& item = newItem.m_Item;

      {
        plInt64 iValue = static_cast<const plAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<plInt64>();

        item.m_sDisplay = plTranslate(pProp->GetPropertyName());

        item.m_UserValue = iValue;
        if (m_pEnumerationType->IsDerivedFrom<plEnumBase>())
        {
          item.m_CheckState =
            (iCurrentValue == iValue) ? plDynamicMenuAction::Item::CheckMark::Checked : plDynamicMenuAction::Item::CheckMark::Unchecked;
        }
        else if (m_pEnumerationType->IsDerivedFrom<plBitflagsBase>())
        {
          item.m_CheckState =
            ((iCurrentValue & iValue) != 0) ? plDynamicMenuAction::Item::CheckMark::Checked : plDynamicMenuAction::Item::CheckMark::Unchecked;
        }
      }
    }
  }

  appendToOutput();
}

plButtonAction::plButtonAction(const plActionContext& context, const char* szName, bool bCheckable, const char* szIconPath)
  : plNamedAction(context, szName, szIconPath)
{
  m_bCheckable = false;
  m_bChecked = false;
  m_bEnabled = true;
  m_bVisible = true;
}


plSliderAction::plSliderAction(const plActionContext& context, const char* szName)
  : plNamedAction(context, szName, nullptr)
{
  m_bEnabled = true;
  m_bVisible = true;
  m_iMinValue = 0;
  m_iMaxValue = 100;
  m_iCurValue = 50;
}

void plSliderAction::SetRange(plInt32 iMin, plInt32 iMax, bool bTriggerUpdate /*= true*/)
{
  PLASMA_ASSERT_DEBUG(iMin < iMax, "Invalid range");

  m_iMinValue = iMin;
  m_iMaxValue = iMax;

  if (bTriggerUpdate)
    TriggerUpdate();
}

void plSliderAction::SetValue(plInt32 val, bool bTriggerUpdate /*= true*/)
{
  m_iCurValue = plMath::Clamp(val, m_iMinValue, m_iMaxValue);
  if (bTriggerUpdate)
    TriggerUpdate();
}
