#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/RenderPipeline/EditorShapeIconsExtractor.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <RendererCore/Components/SpriteComponent.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditorShapeIconsExtractor, 1, plRTTIDefaultAllocator<plEditorShapeIconsExtractor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Size", m_fSize)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(1.0f), new plSuffixAttribute(" m")),
    PLASMA_MEMBER_PROPERTY("MaxScreenSize", m_fMaxScreenSize)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(64.0f), new plSuffixAttribute(" px")),
    PLASMA_ACCESSOR_PROPERTY("SceneContext", GetSceneContext, SetSceneContext),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plEditorShapeIconsExtractor::plEditorShapeIconsExtractor(const char* szName)
  : plExtractor(szName)
{
  m_fSize = 1.0f;
  m_fMaxScreenSize = 64.0f;
  m_pSceneContext = nullptr;

  FillShapeIconInfo();
}

plEditorShapeIconsExtractor::~plEditorShapeIconsExtractor() {}

void plEditorShapeIconsExtractor::Extract(
  const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& extractedRenderData)
{
  PLASMA_LOCK(view.GetWorld()->GetReadMarker());

  /// \todo Once we have a solution for objects that only have a shape icon we can switch this loop to use visibleObjects instead.
  for (auto it = view.GetWorld()->GetObjects(); it.IsValid(); ++it)
  {
    const plGameObject* pObject = it;
    if (FilterByViewTags(view, pObject))
      continue;

    ExtractShapeIcon(pObject, view, extractedRenderData, plDefaultRenderDataCategories::SimpleOpaque);
  }

  if (m_pSceneContext != nullptr)
  {
    auto objects = m_pSceneContext->GetSelectionWithChildren();

    for (const auto& hObject : objects)
    {
      const plGameObject* pObject = nullptr;
      if (view.GetWorld()->TryGetObject(hObject, pObject))
      {
        if (FilterByViewTags(view, pObject))
          continue;

        ExtractShapeIcon(pObject, view, extractedRenderData, plDefaultRenderDataCategories::Selection);
      }
    }
  }
}

void plEditorShapeIconsExtractor::ExtractShapeIcon(const plGameObject* pObject, const plView& view, plExtractedRenderData& extractedRenderData, plRenderData::Category category)
{
  static const plTag& tagHidden = plTagRegistry::GetGlobalRegistry().RegisterTag("EditorHidden");
  static const plTag& tagEditor = plTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

  if (pObject->GetTags().IsSet(tagEditor) || pObject->GetTags().IsSet(tagHidden))
    return;

  if (pObject->WasCreatedByPrefab())
    return;

  if (pObject->GetComponents().IsEmpty())
    return;

  const plComponent* pComponent = nullptr;
  for (auto it : pObject->GetComponents())
  {
    if (it->IsActive())
    {
      pComponent = it;
      break;
    }
  }

  if (pComponent == nullptr)
    return;

  const plRTTI* pRtti = pComponent->GetDynamicRTTI();

  ShapeIconInfo* pShapeIconInfo = nullptr;
  if (m_ShapeIconInfos.TryGetValue(pRtti, pShapeIconInfo))
  {
    plSpriteRenderData* pRenderData = plCreateRenderDataForThisFrame<plSpriteRenderData>(pObject);
    {
      pRenderData->m_GlobalTransform = pObject->GetGlobalTransform();
      pRenderData->m_GlobalBounds = pObject->GetGlobalBounds();
      pRenderData->m_hTexture = pShapeIconInfo->m_hTexture;
      pRenderData->m_fSize = m_fSize;
      pRenderData->m_fMaxScreenSize = m_fMaxScreenSize;
      pRenderData->m_fAspectRatio = 1.0f;
      pRenderData->m_BlendMode = plSpriteBlendMode::ShapeIcon;
      pRenderData->m_texCoordScale = plVec2(1.0f);
      pRenderData->m_texCoordOffset = plVec2(0.0f);
      pRenderData->m_uiUniqueID = plRenderComponent::GetUniqueIdForRendering(pComponent);

      // prefer color gamma properties
      if (pShapeIconInfo->m_pColorGammaProperty != nullptr)
      {
        pRenderData->m_color = plColor(pShapeIconInfo->m_pColorGammaProperty->GetValue(pComponent));
      }
      else if (pShapeIconInfo->m_pColorProperty != nullptr)
      {
        pRenderData->m_color = pShapeIconInfo->m_pColorProperty->GetValue(pComponent);
      }
      else
      {
        pRenderData->m_color = pShapeIconInfo->m_FallbackColor;
      }

      pRenderData->m_color.a = 1.0f;

      pRenderData->FillBatchIdAndSortingKey();
    }

    extractedRenderData.AddRenderData(pRenderData, category);
  }
}

const plTypedMemberProperty<plColor>* plEditorShapeIconsExtractor::FindColorProperty(const plRTTI* pRtti) const
{
  plHybridArray<const plAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (const plAbstractProperty* pProperty : properties)
  {
    if (pProperty->GetCategory() == plPropertyCategory::Member && pProperty->GetSpecificType() == plGetStaticRTTI<plColor>())
    {
      return static_cast<const plTypedMemberProperty<plColor>*>(pProperty);
    }
  }

  return nullptr;
}

const plTypedMemberProperty<plColorGammaUB>* plEditorShapeIconsExtractor::FindColorGammaProperty(const plRTTI* pRtti) const
{
  plHybridArray<const plAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (const plAbstractProperty* pProperty : properties)
  {
    if (pProperty->GetCategory() == plPropertyCategory::Member && pProperty->GetSpecificType() == plGetStaticRTTI<plColorGammaUB>())
    {
      return static_cast<const plTypedMemberProperty<plColorGammaUB>*>(pProperty);
    }
  }

  return nullptr;
}

void plEditorShapeIconsExtractor::FillShapeIconInfo()
{
  PLASMA_LOG_BLOCK("LoadShapeIconTextures");

  plStringBuilder sPath;

  plRTTI::ForEachDerivedType<plComponent>(
    [&](const plRTTI* pRtti) {
      sPath.Set("Editor/ShapeIcons/", pRtti->GetTypeName(), ".dds");

      if (plFileSystem::ExistsFile(sPath))
      {
        auto& shapeIconInfo = m_ShapeIconInfos[pRtti];
        shapeIconInfo.m_hTexture = plResourceManager::LoadResource<plTexture2DResource>(sPath);
        shapeIconInfo.m_pColorProperty = FindColorProperty(pRtti);
        shapeIconInfo.m_pColorGammaProperty = FindColorGammaProperty(pRtti);

        if (auto pCatAttribute = pRtti->GetAttributeByType<plCategoryAttribute>())
        {
          shapeIconInfo.m_FallbackColor = plColorScheme::GetCategoryColor(pCatAttribute->GetCategory(), plColorScheme::CategoryColorUsage::ViewportIcon);
        }
      }
    });
}
