#include <RendererCore/RendererCorePCH.h>

#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
plCVarBool cvar_SpatialVisBounds("Spatial.VisBounds", false, plCVarFlags::Default, "Enables debug visualization of object bounds");
plCVarBool cvar_SpatialVisLocalBBox("Spatial.VisLocalBBox", false, plCVarFlags::Default, "Enables debug visualization of object local bounding box");
plCVarBool cvar_SpatialVisData("Spatial.VisData", false, plCVarFlags::Default, "Enables debug visualization of the spatial data structure");
plCVarString cvar_SpatialVisDataOnlyCategory("Spatial.VisData.OnlyCategory", "", plCVarFlags::Default, "When set the debug visualization is only shown for the given spatial data category");
plCVarBool cvar_SpatialVisDataOnlySelected("Spatial.VisData.OnlySelected", false, plCVarFlags::Default, "When set the debug visualization is only shown for selected objects");
plCVarString cvar_SpatialVisDataOnlyObject("Spatial.VisData.OnlyObject", "", plCVarFlags::Default, "When set the debug visualization is only shown for objects with the given name");

plCVarBool cvar_SpatialExtractionShowStats("Spatial.Extraction.ShowStats", false, plCVarFlags::Default, "Display some stats of the render data extraction");
#endif

namespace
{
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  void VisualizeSpatialData(const plView& view)
  {
    if (cvar_SpatialVisData && cvar_SpatialVisDataOnlyObject.GetValue().IsEmpty() && !cvar_SpatialVisDataOnlySelected)
    {
      const plSpatialSystem& spatialSystem = *view.GetWorld()->GetSpatialSystem();
      if (auto pSpatialSystemGrid = plDynamicCast<const plSpatialSystem_RegularGrid*>(&spatialSystem))
      {
        plSpatialData::Category filterCategory = plSpatialData::FindCategory(cvar_SpatialVisDataOnlyCategory.GetValue());

        plHybridArray<plBoundingBox, 16> boxes;
        pSpatialSystemGrid->GetAllCellBoxes(boxes, filterCategory);

        for (auto& box : boxes)
        {
          plDebugRenderer::DrawLineBox(view.GetHandle(), box, plColor::Cyan);
        }
      }
    }
  }

  void VisualizeObject(const plView& view, const plGameObject* pObject)
  {
    if (!cvar_SpatialVisBounds && !cvar_SpatialVisLocalBBox && !cvar_SpatialVisData)
      return;

    if (cvar_SpatialVisLocalBBox)
    {
      const plBoundingBoxSphere& localBounds = pObject->GetLocalBounds();
      if (localBounds.IsValid())
      {
        plDebugRenderer::DrawLineBox(view.GetHandle(), localBounds.GetBox(), plColor::Yellow, pObject->GetGlobalTransform());
      }
    }

    if (cvar_SpatialVisBounds)
    {
      const plBoundingBoxSphere& globalBounds = pObject->GetGlobalBounds();
      if (globalBounds.IsValid())
      {
        plDebugRenderer::DrawLineBox(view.GetHandle(), globalBounds.GetBox(), plColor::Lime);
        plDebugRenderer::DrawLineSphere(view.GetHandle(), globalBounds.GetSphere(), plColor::Magenta);
      }
    }

    if (cvar_SpatialVisData && cvar_SpatialVisDataOnlyCategory.GetValue().IsEmpty())
    {
      const plSpatialSystem& spatialSystem = *view.GetWorld()->GetSpatialSystem();
      if (auto pSpatialSystemGrid = plDynamicCast<const plSpatialSystem_RegularGrid*>(&spatialSystem))
      {
        plBoundingBox box;
        if (pSpatialSystemGrid->GetCellBoxForSpatialData(pObject->GetSpatialData(), box).Succeeded())
        {
          plDebugRenderer::DrawLineBox(view.GetHandle(), box, plColor::Cyan);
        }
      }
    }
  }
#endif
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plExtractor, 1, plRTTINoAllocator)
  {
    PLASMA_BEGIN_PROPERTIES
    {
      PLASMA_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new plDefaultValueAttribute(true)),
      PLASMA_ACCESSOR_PROPERTY("Name", GetName, SetName),
    }
    PLASMA_END_PROPERTIES;
    PLASMA_BEGIN_ATTRIBUTES
    {
      new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Red)),
    }
    PLASMA_END_ATTRIBUTES;
  }
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format om

plExtractor::plExtractor(const char* szName)
{
  m_bActive = true;
  m_sName.Assign(szName);

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  m_uiNumCachedRenderData = 0;
  m_uiNumUncachedRenderData = 0;
#endif
}

plExtractor::~plExtractor() = default;

void plExtractor::SetName(const char* szName)
{
  if (!plStringUtils::IsNullOrEmpty(szName))
  {
    m_sName.Assign(szName);
  }
}

const char* plExtractor::GetName() const
{
  return m_sName.GetData();
}

bool plExtractor::FilterByViewTags(const plView& view, const plGameObject* pObject) const
{
  if (!view.m_ExcludeTags.IsEmpty() && view.m_ExcludeTags.IsAnySet(pObject->GetTags()))
    return true;

  if (!view.m_IncludeTags.IsEmpty() && !view.m_IncludeTags.IsAnySet(pObject->GetTags()))
    return true;

  return false;
}

void plExtractor::ExtractRenderData(const plView& view, const plGameObject* pObject, plMsgExtractRenderData& msg, plExtractedRenderData& extractedRenderData) const
{
  auto AddRenderDataFromMessage = [&](const plMsgExtractRenderData& msg) {
    if (msg.m_OverrideCategory != plInvalidRenderDataCategory)
    {
      for (auto& data : msg.m_ExtractedRenderData)
      {
        extractedRenderData.AddRenderData(data.m_pRenderData, msg.m_OverrideCategory);
      }
    }
    else
    {
      for (auto& data : msg.m_ExtractedRenderData)
      {
        extractedRenderData.AddRenderData(data.m_pRenderData, plRenderData::Category(data.m_uiCategory));
      }
    }

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
    m_uiNumUncachedRenderData += msg.m_ExtractedRenderData.GetCount();
#endif
  };

  if (pObject->IsStatic())
  {
    plUInt16 uiComponentVersion = pObject->GetComponentVersion();

    auto cachedRenderData = plRenderWorld::GetCachedRenderData(view, pObject->GetHandle(), uiComponentVersion);

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    for (plUInt32 i = 1; i < cachedRenderData.GetCount(); ++i)
    {
      PLASMA_ASSERT_DEBUG(cachedRenderData[i - 1].m_uiComponentIndex <= cachedRenderData[i].m_uiComponentIndex, "Cached render data needs to be sorted");
      if (cachedRenderData[i - 1].m_uiComponentIndex == cachedRenderData[i].m_uiComponentIndex)
      {
        PLASMA_ASSERT_DEBUG(cachedRenderData[i - 1].m_uiPartIndex < cachedRenderData[i].m_uiPartIndex, "Cached render data needs to be sorted");
      }
    }
#endif

    plUInt32 uiCacheIndex = 0;

    auto components = pObject->GetComponents();
    const plUInt32 uiNumComponents = components.GetCount();
    for (plUInt32 uiComponentIndex = 0; uiComponentIndex < uiNumComponents; ++uiComponentIndex)
    {
      bool bCacheFound = false;
      while (uiCacheIndex < cachedRenderData.GetCount() && cachedRenderData[uiCacheIndex].m_uiComponentIndex == uiComponentIndex)
      {
        const plInternal::RenderDataCacheEntry& cacheEntry = cachedRenderData[uiCacheIndex];
        if (cacheEntry.m_pRenderData != nullptr)
        {
          extractedRenderData.AddRenderData(cacheEntry.m_pRenderData, msg.m_OverrideCategory != plInvalidRenderDataCategory ? msg.m_OverrideCategory : plRenderData::Category(cacheEntry.m_uiCategory));

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
          ++m_uiNumCachedRenderData;
#endif
        }
        ++uiCacheIndex;

        bCacheFound = true;
      }

      if (bCacheFound)
      {
        continue;
      }

      const plComponent* pComponent = components[uiComponentIndex];

      msg.m_ExtractedRenderData.Clear();
      msg.m_uiNumCacheIfStatic = 0;

      if (pComponent->SendMessage(msg))
      {
        // Only cache render data if all parts should be cached otherwise the cache is incomplete and we won't call SendMessage again
        if (msg.m_uiNumCacheIfStatic > 0 && msg.m_ExtractedRenderData.GetCount() == msg.m_uiNumCacheIfStatic)
        {
          plHybridArray<plInternal::RenderDataCacheEntry, 16> newCacheEntries(plFrameAllocator::GetCurrentAllocator());

          for (plUInt32 uiPartIndex = 0; uiPartIndex < msg.m_ExtractedRenderData.GetCount(); ++uiPartIndex)
          {
            auto& newCacheEntry = newCacheEntries.ExpandAndGetRef();
            newCacheEntry.m_pRenderData = msg.m_ExtractedRenderData[uiPartIndex].m_pRenderData;
            newCacheEntry.m_uiCategory = msg.m_ExtractedRenderData[uiPartIndex].m_uiCategory;
            newCacheEntry.m_uiComponentIndex = static_cast<plUInt16>(uiComponentIndex);
            newCacheEntry.m_uiPartIndex = static_cast<plUInt16>(uiPartIndex);
          }

          plRenderWorld::CacheRenderData(view, pObject->GetHandle(), pComponent->GetHandle(), uiComponentVersion, newCacheEntries);
        }

        AddRenderDataFromMessage(msg);
      }
      else if (pComponent->IsActiveAndInitialized()) // component does not handle extract message at all
      {
        PLASMA_ASSERT_DEV(pComponent->GetDynamicRTTI()->CanHandleMessage<plMsgExtractRenderData>() == false, "");

        // Create a dummy cache entry so we don't call send message next time
        plInternal::RenderDataCacheEntry dummyEntry;
        dummyEntry.m_pRenderData = nullptr;
        dummyEntry.m_uiCategory = plInvalidRenderDataCategory.m_uiValue;
        dummyEntry.m_uiComponentIndex = static_cast<plUInt16>(uiComponentIndex);

        plRenderWorld::CacheRenderData(view, pObject->GetHandle(), pComponent->GetHandle(), uiComponentVersion, plMakeArrayPtr(&dummyEntry, 1));
      }
    }
  }
  else
  {
    msg.m_ExtractedRenderData.Clear();
    pObject->SendMessage(msg);

    AddRenderDataFromMessage(msg);
  }
}

void plExtractor::Extract(const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& ref_extractedRenderData)
{
}

void plExtractor::PostSortAndBatch(
  const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& ref_extractedRenderData)
{
}


plResult plExtractor::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream << m_bActive;
  inout_stream << m_sName;
  return PLASMA_SUCCESS;
}


plResult plExtractor::Deserialize(plStreamReader& inout_stream)
{
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PLASMA_ASSERT_DEBUG(uiVersion == 1, "Unknown version encountered");

  inout_stream >> m_bActive;
  inout_stream >> m_sName;
  return PLASMA_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plVisibleObjectsExtractor, 1, plRTTIDefaultAllocator<plVisibleObjectsExtractor>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plVisibleObjectsExtractor::plVisibleObjectsExtractor(const char* szName)
  : plExtractor(szName)
{
}

plVisibleObjectsExtractor::~plVisibleObjectsExtractor() = default;

void plVisibleObjectsExtractor::Extract(
  const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& ref_extractedRenderData)
{
  plMsgExtractRenderData msg;
  msg.m_pView = &view;

  PLASMA_LOCK(view.GetWorld()->GetReadMarker());

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  VisualizeSpatialData(view);

  m_uiNumCachedRenderData = 0;
  m_uiNumUncachedRenderData = 0;
#endif

  for (auto pObject : visibleObjects)
  {
    ExtractRenderData(view, pObject, msg, ref_extractedRenderData);

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
    if (cvar_SpatialVisBounds || cvar_SpatialVisLocalBBox || cvar_SpatialVisData)
    {
      if ((cvar_SpatialVisDataOnlyObject.GetValue().IsEmpty() ||
            pObject->GetName().FindSubString_NoCase(cvar_SpatialVisDataOnlyObject.GetValue()) != nullptr) &&
          !cvar_SpatialVisDataOnlySelected)
      {
        VisualizeObject(view, pObject);
      }
    }
#endif
  }

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  const bool bIsMainView = (view.GetCameraUsageHint() == plCameraUsageHint::MainView || view.GetCameraUsageHint() == plCameraUsageHint::EditorView);

  if (cvar_SpatialExtractionShowStats && bIsMainView)
  {
    plViewHandle hView = view.GetHandle();

    plStringBuilder sb;

    plDebugRenderer::DrawInfoText(hView, plDebugTextPlacement::TopLeft, "ExtractionStats", "Extraction Stats:");

    sb.Format("Num Cached Render Data: {0}", m_uiNumCachedRenderData);
    plDebugRenderer::DrawInfoText(hView, plDebugTextPlacement::TopLeft, "ExtractionStats", sb);

    sb.Format("Num Uncached Render Data: {0}", m_uiNumUncachedRenderData);
    plDebugRenderer::DrawInfoText(hView, plDebugTextPlacement::TopLeft, "ExtractionStats", sb);
  }
#endif
}

plResult plVisibleObjectsExtractor::Serialize(plStreamWriter& inout_stream) const
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return PLASMA_SUCCESS;
}

plResult plVisibleObjectsExtractor::Deserialize(plStreamReader& inout_stream)
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PLASMA_IGNORE_UNUSED(uiVersion);
  return PLASMA_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSelectedObjectsExtractorBase, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plSelectedObjectsExtractorBase::plSelectedObjectsExtractorBase(const char* szName)
  : plExtractor(szName)
  , m_OverrideCategory(plDefaultRenderDataCategories::Selection)
{
}

plSelectedObjectsExtractorBase::~plSelectedObjectsExtractorBase() = default;

void plSelectedObjectsExtractorBase::Extract(
  const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& ref_extractedRenderData)
{
  const plDeque<plGameObjectHandle>* pSelection = GetSelection();
  if (pSelection == nullptr)
    return;

  plMsgExtractRenderData msg;
  msg.m_pView = &view;
  msg.m_OverrideCategory = m_OverrideCategory;

  PLASMA_LOCK(view.GetWorld()->GetReadMarker());

  for (const auto& hObj : *pSelection)
  {
    const plGameObject* pObject = nullptr;
    if (!view.GetWorld()->TryGetObject(hObj, pObject))
      continue;

    ExtractRenderData(view, pObject, msg, ref_extractedRenderData);

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
    if (cvar_SpatialVisBounds || cvar_SpatialVisLocalBBox || cvar_SpatialVisData)
    {
      if (cvar_SpatialVisDataOnlySelected)
      {
        VisualizeObject(view, pObject);
      }
    }
#endif
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSelectedObjectsContext, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSelectedObjectsExtractor, 1, plRTTIDefaultAllocator<plSelectedObjectsExtractor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("SelectionContext", GetSelectionContext, SetSelectionContext),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSelectedObjectsContext::plSelectedObjectsContext() = default;
plSelectedObjectsContext::~plSelectedObjectsContext() = default;

void plSelectedObjectsContext::RemoveDeadObjects(const plWorld& world)
{
  for (plUInt32 i = 0; i < m_Objects.GetCount();)
  {
    const plGameObject* pObj;
    if (world.TryGetObject(m_Objects[i], pObj) == false)
    {
      m_Objects.RemoveAtAndSwap(i);
    }
    else
      ++i;
  }
}

void plSelectedObjectsContext::AddObjectAndChildren(const plWorld& world, const plGameObjectHandle& hObject)
{
  const plGameObject* pObj;
  if (world.TryGetObject(hObject, pObj))
  {
    m_Objects.PushBack(hObject);

    for (auto it = pObj->GetChildren(); it.IsValid(); ++it)
    {
      AddObjectAndChildren(world, it);
    }
  }
}

void plSelectedObjectsContext::AddObjectAndChildren(const plWorld& world, const plGameObject* pObject)
{
  m_Objects.PushBack(pObject->GetHandle());

  for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
  {
    AddObjectAndChildren(world, it);
  }
}

plSelectedObjectsExtractor::plSelectedObjectsExtractor(const char* szName /*= "ExplicitlySelectedObjectsExtractor"*/)
  : plSelectedObjectsExtractorBase(szName)
{
}

plSelectedObjectsExtractor::~plSelectedObjectsExtractor() = default;

const plDeque<plGameObjectHandle>* plSelectedObjectsExtractor::GetSelection()
{
  if (m_pSelectionContext)
    return &m_pSelectionContext->m_Objects;

  return nullptr;
}

plResult plSelectedObjectsExtractor::Serialize(plStreamWriter& inout_stream) const
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return PLASMA_SUCCESS;
}

plResult plSelectedObjectsExtractor::Deserialize(plStreamReader& inout_stream)
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PLASMA_IGNORE_UNUSED(uiVersion);
  return PLASMA_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Extractor);
