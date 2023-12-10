#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginKraut/EnginePluginKrautDLL.h>
#include <KrautPlugin/Components/KrautTreeComponent.h>
#include <RendererCore/Meshes/MeshResource.h>

class plObjectSelectionMsgToEngine;
class plRenderContext;

class PLASMA_ENGINEPLUGINKRAUT_DLL plKrautTreeContext : public plEngineProcessDocumentContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plKrautTreeContext, plEngineProcessDocumentContext);

public:
  plKrautTreeContext();

  virtual void HandleMessage(const plEditorEngineDocumentMsg* pMsg) override;
  const plKrautGeneratorResourceHandle& GetResource() const { return m_hMainResource; }

protected:
  virtual void OnInitialize() override;

  virtual plEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(plEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const plEditorEngineDocumentMsg* pMsg);

  plGameObject* m_pMainObject;
  plComponentHandle m_hKrautComponent;
  plKrautGeneratorResourceHandle m_hMainResource;
  plMeshResourceHandle m_hPreviewMeshResource;
  plUInt32 m_uiDisplayRandomSeed = 0xFFFFFFFF;
};
