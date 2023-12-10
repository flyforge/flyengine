#pragma once

#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Textures/Texture2DResource.h>

class plSceneContext;

class plEditorShapeIconsExtractor : public plExtractor
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plEditorShapeIconsExtractor, plExtractor);

public:
  plEditorShapeIconsExtractor(const char* szName = "EditorShapeIconsExtractor");
  ~plEditorShapeIconsExtractor();

  virtual void Extract(
    const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& extractedRenderData) override;

  void SetSceneContext(plSceneContext* pSceneContext) { m_pSceneContext = pSceneContext; }
  plSceneContext* GetSceneContext() const { return m_pSceneContext; }

private:
  void ExtractShapeIcon(const plGameObject* pObject, const plView& view, plExtractedRenderData& extractedRenderData, plRenderData::Category category);
  const plTypedMemberProperty<plColor>* FindColorProperty(const plRTTI* pRtti) const;
  const plTypedMemberProperty<plColorGammaUB>* FindColorGammaProperty(const plRTTI* pRtti) const;
  void FillShapeIconInfo();

  float m_fSize;
  float m_fMaxScreenSize;
  plSceneContext* m_pSceneContext;

  struct ShapeIconInfo
  {
    plTexture2DResourceHandle m_hTexture;
    const plTypedMemberProperty<plColor>* m_pColorProperty;
    const plTypedMemberProperty<plColorGammaUB>* m_pColorGammaProperty;
    plColor m_FallbackColor = plColor::White;
  };

  plHashTable<const plRTTI*, ShapeIconInfo> m_ShapeIconInfos;
};
