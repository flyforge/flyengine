#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Declarations.h>

class PLASMA_ENGINEPLUGINASSETS_DLL plAnimationClipContext : public PlasmaEngineProcessDocumentContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAnimationClipContext, PlasmaEngineProcessDocumentContext);

public:
  plAnimationClipContext();

  virtual void HandleMessage(const PlasmaEditorEngineDocumentMsg* pMsg) override;

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual PlasmaEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(PlasmaEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(PlasmaEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const PlasmaEditorEngineDocumentMsg* pMsg);
  void SetPlaybackPosition(double pos);

  plGameObject* m_pGameObject = nullptr;
  plString m_sAnimatedMeshToUse;
  plComponentHandle m_hAnimMeshComponent;
  plComponentHandle m_hAnimControllerComponent;
};
