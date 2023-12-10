#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/Renderer.h>
#include <RendererCore/Pipeline/SortingFunctions.h>

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, RenderData)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    plRenderData::UpdateRendererTypes();

    plPlugin::Events().AddEventHandler(plRenderData::PluginEventHandler);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    plPlugin::Events().RemoveEventHandler(plRenderData::PluginEventHandler);

    plRenderData::ClearRendererInstances();
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRenderData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRenderer, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgExtractRenderData);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgExtractRenderData, 1, plRTTIDefaultAllocator<plMsgExtractRenderData>)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgExtractOccluderData);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgExtractOccluderData, 1, plRTTIDefaultAllocator<plMsgExtractOccluderData>)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plExcludeFromScript()
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plHybridArray<plRenderData::CategoryData, 32> plRenderData::s_CategoryData;

plHybridArray<const plRTTI*, 16> plRenderData::s_RendererTypes;
plDynamicArray<plUniquePtr<plRenderer>> plRenderData::s_RendererInstances;
bool plRenderData::s_bRendererInstancesDirty = false;

// static
plRenderData::Category plRenderData::RegisterCategory(const char* szCategoryName, SortingKeyFunc sortingKeyFunc)
{
  plHashedString sCategoryName;
  sCategoryName.Assign(szCategoryName);

  Category oldCategory = FindCategory(sCategoryName);
  if (oldCategory != plInvalidRenderDataCategory)
    return oldCategory;

  Category newCategory = Category(static_cast<plUInt16>(s_CategoryData.GetCount()));

  auto& data = s_CategoryData.ExpandAndGetRef();
  data.m_sName = sCategoryName;
  data.m_sortingKeyFunc = sortingKeyFunc;

  return newCategory;
}

// static
plRenderData::Category plRenderData::FindCategory(plTempHashedString sCategoryName)
{
  for (plUInt32 uiCategoryIndex = 0; uiCategoryIndex < s_CategoryData.GetCount(); ++uiCategoryIndex)
  {
    if (s_CategoryData[uiCategoryIndex].m_sName == sCategoryName)
      return Category(static_cast<plUInt16>(uiCategoryIndex));
  }

  return plInvalidRenderDataCategory;
}

// static
void plRenderData::GetAllCategoryNames(plDynamicArray<plHashedString>& out_categoryNames)
{
  out_categoryNames.Clear();

  for (auto& data : s_CategoryData)
  {
    out_categoryNames.PushBack(data.m_sName);
  }
}

// static
void plRenderData::PluginEventHandler(const plPluginEvent& e)
{
  switch (e.m_EventType)
  {
    case plPluginEvent::AfterPluginChanges:
      UpdateRendererTypes();
      break;

    default:
      break;
  }
}

// static
void plRenderData::UpdateRendererTypes()
{
  s_RendererTypes.Clear();

  plRTTI::ForEachDerivedType<plRenderer>([](const plRTTI* pRtti) { s_RendererTypes.PushBack(pRtti); },
    plRTTI::ForEachOptions::ExcludeNonAllocatable);

  s_bRendererInstancesDirty = true;
}

// static
void plRenderData::CreateRendererInstances()
{
  ClearRendererInstances();

  for (auto pRendererType : s_RendererTypes)
  {
    PLASMA_ASSERT_DEV(pRendererType->IsDerivedFrom(plGetStaticRTTI<plRenderer>()), "Renderer type '{}' must be derived from plRenderer",
      pRendererType->GetTypeName());

    auto pRenderer = pRendererType->GetAllocator()->Allocate<plRenderer>();

    plUInt32 uiIndex = s_RendererInstances.GetCount();
    s_RendererInstances.PushBack(pRenderer);

    plHybridArray<Category, 8> supportedCategories;
    pRenderer->GetSupportedRenderDataCategories(supportedCategories);

    plHybridArray<const plRTTI*, 8> supportedTypes;
    pRenderer->GetSupportedRenderDataTypes(supportedTypes);

    for (Category category : supportedCategories)
    {
      auto& categoryData = s_CategoryData[category.m_uiValue];

      for (plUInt32 i = 0; i < supportedTypes.GetCount(); ++i)
      {
        categoryData.m_TypeToRendererIndex.Insert(supportedTypes[i], uiIndex);
      }
    }
  }

  s_bRendererInstancesDirty = false;
}

// static
void plRenderData::ClearRendererInstances()
{
  s_RendererInstances.Clear();

  for (auto& categoryData : s_CategoryData)
  {
    categoryData.m_TypeToRendererIndex.Clear();
  }
}

//////////////////////////////////////////////////////////////////////////

plRenderData::Category plDefaultRenderDataCategories::Light = plRenderData::RegisterCategory("Light", &plRenderSortingFunctions::ByRenderDataThenFrontToBack);
plRenderData::Category plDefaultRenderDataCategories::Decal = plRenderData::RegisterCategory("Decal", &plRenderSortingFunctions::ByRenderDataThenFrontToBack);
plRenderData::Category plDefaultRenderDataCategories::ReflectionProbe = plRenderData::RegisterCategory("ReflectionProbe", &plRenderSortingFunctions::ByRenderDataThenFrontToBack);
plRenderData::Category plDefaultRenderDataCategories::Sky = plRenderData::RegisterCategory("Sky", &plRenderSortingFunctions::ByRenderDataThenFrontToBack);
plRenderData::Category plDefaultRenderDataCategories::LitOpaque = plRenderData::RegisterCategory("LitOpaque", &plRenderSortingFunctions::ByRenderDataThenFrontToBack);
plRenderData::Category plDefaultRenderDataCategories::LitMasked = plRenderData::RegisterCategory("LitMasked", &plRenderSortingFunctions::ByRenderDataThenFrontToBack);
plRenderData::Category plDefaultRenderDataCategories::LitTransparent = plRenderData::RegisterCategory("LitTransparent", &plRenderSortingFunctions::BackToFrontThenByRenderData);
plRenderData::Category plDefaultRenderDataCategories::LitForeground = plRenderData::RegisterCategory("LitForeground", &plRenderSortingFunctions::ByRenderDataThenFrontToBack);
plRenderData::Category plDefaultRenderDataCategories::LitScreenFX = plRenderData::RegisterCategory("LitScreenFX", &plRenderSortingFunctions::BackToFrontThenByRenderData);
plRenderData::Category plDefaultRenderDataCategories::SimpleOpaque = plRenderData::RegisterCategory("SimpleOpaque", &plRenderSortingFunctions::ByRenderDataThenFrontToBack);
plRenderData::Category plDefaultRenderDataCategories::SimpleTransparent = plRenderData::RegisterCategory("SimpleTransparent", &plRenderSortingFunctions::BackToFrontThenByRenderData);
plRenderData::Category plDefaultRenderDataCategories::SimpleForeground = plRenderData::RegisterCategory("SimpleForeground", &plRenderSortingFunctions::ByRenderDataThenFrontToBack);
plRenderData::Category plDefaultRenderDataCategories::Selection = plRenderData::RegisterCategory("Selection", &plRenderSortingFunctions::ByRenderDataThenFrontToBack);
plRenderData::Category plDefaultRenderDataCategories::GUI = plRenderData::RegisterCategory("GUI", &plRenderSortingFunctions::BackToFrontThenByRenderData);

//////////////////////////////////////////////////////////////////////////

void plMsgExtractRenderData::AddRenderData(
  const plRenderData* pRenderData, plRenderData::Category category, plRenderData::Caching::Enum cachingBehavior)
{
  auto& cached = m_ExtractedRenderData.ExpandAndGetRef();
  cached.m_pRenderData = pRenderData;
  cached.m_uiCategory = category.m_uiValue;

  if (cachingBehavior == plRenderData::Caching::IfStatic)
  {
    ++m_uiNumCacheIfStatic;
  }
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderData);
