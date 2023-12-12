#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Meshes/MeshResource.h>

class plObjectSelectionMsgToEngine;
class plRenderContext;

class PLASMA_ENGINEPLUGINASSETS_DLL plMeshContext : public PlasmaEngineProcessDocumentContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMeshContext, PlasmaEngineProcessDocumentContext);

public:
  plMeshContext();

  virtual void HandleMessage(const PlasmaEditorEngineDocumentMsg* pMsg) override;

  const plMeshResourceHandle& GetMesh() const { return m_hMesh; }

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual PlasmaEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(PlasmaEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(PlasmaEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const PlasmaEditorEngineDocumentMsg* pMsg);
  void OnResourceEvent(const plResourceEvent& e);

  plGameObject* m_pMeshObject;
  plMeshResourceHandle m_hMesh;

  plAtomicBool m_bBoundsDirty = false;
  plEvent<const plResourceEvent&, plMutex>::Unsubscriber m_MeshResourceEventSubscriber;
};
