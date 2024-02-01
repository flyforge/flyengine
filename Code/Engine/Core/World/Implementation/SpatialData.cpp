#include <Core/CorePCH.h>

#include <Core/World/SpatialData.h>

plHybridArray<plSpatialData::CategoryData, 32>& plSpatialData::GetCategoryData()
{
  static plHybridArray<plSpatialData::CategoryData, 32> CategoryData;
  return CategoryData;
}

// static
plSpatialData::Category plSpatialData::RegisterCategory(plStringView sCategoryName, const plBitflags<Flags>& flags)
{
  if (sCategoryName.IsEmpty())
    return plInvalidSpatialDataCategory;

  Category oldCategory = FindCategory(sCategoryName);
  if (oldCategory != plInvalidSpatialDataCategory)
  {
    PL_ASSERT_DEV(GetCategoryFlags(oldCategory) == flags, "Category registered with different flags");
    return oldCategory;
  }

  if (GetCategoryData().GetCount() == 32)
  {
    PL_REPORT_FAILURE("Too many spatial data categories");
    return plInvalidSpatialDataCategory;
  }

  Category newCategory = Category(static_cast<plUInt16>(GetCategoryData().GetCount()));

  auto& data = GetCategoryData().ExpandAndGetRef();
  data.m_sName.Assign(sCategoryName);
  data.m_Flags = flags;

  return newCategory;
}

// static
plSpatialData::Category plSpatialData::FindCategory(plStringView sCategoryName)
{
  plTempHashedString categoryName(sCategoryName);

  for (plUInt32 uiCategoryIndex = 0; uiCategoryIndex < GetCategoryData().GetCount(); ++uiCategoryIndex)
  {
    if (GetCategoryData()[uiCategoryIndex].m_sName == categoryName)
      return Category(static_cast<plUInt16>(uiCategoryIndex));
  }

  return plInvalidSpatialDataCategory;
}

// static
const plHashedString& plSpatialData::GetCategoryName(Category category)
{
  if (category.m_uiValue < GetCategoryData().GetCount())
  {
    return GetCategoryData()[category.m_uiValue].m_sName;
  }

  static plHashedString sInvalidSpatialDataCategoryName;
  return sInvalidSpatialDataCategoryName;
}

// static
const plBitflags<plSpatialData::Flags>& plSpatialData::GetCategoryFlags(Category category)
{
  return GetCategoryData()[category.m_uiValue].m_Flags;
}

//////////////////////////////////////////////////////////////////////////

plSpatialData::Category plDefaultSpatialDataCategories::RenderStatic = plSpatialData::RegisterCategory("RenderStatic", plSpatialData::Flags::None);
plSpatialData::Category plDefaultSpatialDataCategories::RenderDynamic = plSpatialData::RegisterCategory("RenderDynamic", plSpatialData::Flags::FrequentChanges);
plSpatialData::Category plDefaultSpatialDataCategories::OcclusionStatic = plSpatialData::RegisterCategory("OcclusionStatic", plSpatialData::Flags::None);
plSpatialData::Category plDefaultSpatialDataCategories::OcclusionDynamic = plSpatialData::RegisterCategory("OcclusionDynamic", plSpatialData::Flags::FrequentChanges);


