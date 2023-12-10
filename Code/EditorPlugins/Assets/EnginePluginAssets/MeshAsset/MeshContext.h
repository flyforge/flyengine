#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Meshes/MeshResource.h>

class plObjectSelectionMsgToEngine;
class plRenderContext;

class PLASMA_ENGINEPLUGINASSETS_DLL plMeshContext : public plEngineProcessDocumentContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMeshContext, plEngineProcessDocumentContext);

public:
  plMeshContext();

  virtual void HandleMessage(const plEditorEngineDocumentMsg* pMsg) override;

  const plMeshResourceHandle& GetMesh() const { return m_hMesh; }

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual plEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(plEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const plEditorEngineDocumentMsg* pMsg);
  void OnResourceEvent(const plResourceEvent& e);

  plGameObject* m_pMeshObject;
  plMeshResourceHandle m_hMesh;

  plAtomicBool m_bBoundsDirty = false;
  plEvent<const plResourceEvent&, plMutex>::Unsubscriber m_MeshResourceEventSubscriber;
};
