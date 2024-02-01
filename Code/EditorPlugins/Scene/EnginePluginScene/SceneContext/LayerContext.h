#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginScene/EnginePluginSceneDLL.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <SharedPluginScene/Common/Messages.h>

class plDocumentOpenMsgToEngine;

/// \brief Layers that are loaded as sub-documents of a scene share the plWorld with their main document scene. Thus, this context attaches itself to its parent plSceneContext.
class PL_ENGINEPLUGINSCENE_DLL plLayerContext : public plEngineProcessDocumentContext
{
  PL_ADD_DYNAMIC_REFLECTION(plLayerContext, plEngineProcessDocumentContext);

public:
  static plEngineProcessDocumentContext* AllocateContext(const plDocumentOpenMsgToEngine* pMsg);
  plLayerContext();
  ~plLayerContext();

  virtual void HandleMessage(const plEditorEngineDocumentMsg* pMsg) override;
  void SceneDeinitialized();
  const plTag& GetLayerTag() const;

protected:
  virtual void OnInitialize() override;
  virtual void OnDeinitialize() override;

  virtual plEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(plEngineProcessViewContext* pContext) override;
  virtual plStatus ExportDocument(const plExportDocumentMsgToEngine* pMsg) override;

  virtual void UpdateDocumentContext() override;

private:
  plSceneContext* m_pParentSceneContext = nullptr;
  plTag m_LayerTag;
};
