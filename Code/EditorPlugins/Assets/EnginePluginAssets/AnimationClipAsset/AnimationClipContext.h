#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Declarations.h>

class PLASMA_ENGINEPLUGINASSETS_DLL plAnimationClipContext : public plEngineProcessDocumentContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAnimationClipContext, plEngineProcessDocumentContext);

public:
  plAnimationClipContext();

  virtual void HandleMessage(const plEditorEngineDocumentMsg* pMsg) override;

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual plEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(plEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const plEditorEngineDocumentMsg* pMsg);
  void SetPlaybackPosition(double pos);

  plGameObject* m_pGameObject = nullptr;
  plString m_sAnimatedMeshToUse;
  plComponentHandle m_hAnimMeshComponent;
  plComponentHandle m_hAnimControllerComponent;
};
