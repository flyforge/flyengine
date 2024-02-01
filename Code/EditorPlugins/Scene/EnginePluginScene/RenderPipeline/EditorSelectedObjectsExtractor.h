#pragma once

#include <Core/Graphics/Camera.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Pipeline/Extractor.h>

class plSceneContext;
class plCameraComponent;

class plEditorSelectedObjectsExtractor : public plSelectedObjectsExtractorBase
{
  PL_ADD_DYNAMIC_REFLECTION(plEditorSelectedObjectsExtractor, plSelectedObjectsExtractorBase);

public:
  plEditorSelectedObjectsExtractor();
  ~plEditorSelectedObjectsExtractor();

  virtual const plDeque<plGameObjectHandle>* GetSelection() override;

  virtual void Extract(const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& ref_extractedRenderData) override;
  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;

  void SetSceneContext(plSceneContext* pSceneContext) { m_pSceneContext = pSceneContext; }
  plSceneContext* GetSceneContext() const { return m_pSceneContext; }

private:
  void CreateRenderTargetTexture(const plView& view);
  void CreateRenderTargetView(const plView& view);
  void UpdateRenderTargetCamera(const plCameraComponent* pCamComp);

  plSceneContext* m_pSceneContext;
  plViewHandle m_hRenderTargetView;
  plRenderToTexture2DResourceHandle m_hRenderTarget;
  plCamera m_RenderTargetCamera;
};
