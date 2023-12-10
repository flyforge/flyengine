#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Meshes/MeshResource.h>

class plObjectSelectionMsgToEngine;
class plRenderContext;

class PLASMA_ENGINEPLUGINASSETS_DLL plAnimatedMeshContext : public plEngineProcessDocumentContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAnimatedMeshContext, plEngineProcessDocumentContext);

public:
  plAnimatedMeshContext();

  virtual void HandleMessage(const plEditorEngineDocumentMsg* pMsg) override;

  const plMeshResourceHandle& GetAnimatedMesh() const { return m_hAnimatedMesh; }

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual plEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(plEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const plEditorEngineDocumentMsg* pMsg);

  plGameObject* m_pAnimatedMeshObject;
  plMeshResourceHandle m_hAnimatedMesh;
};
