#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginJolt/EnginePluginJoltDLL.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>

class plObjectSelectionMsgToEngine;
class plRenderContext;

class PLASMA_ENGINEPLUGINJOLT_DLL plJoltCollisionMeshContext : public PlasmaEngineProcessDocumentContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plJoltCollisionMeshContext, PlasmaEngineProcessDocumentContext);

public:
  plJoltCollisionMeshContext();

  virtual void HandleMessage(const PlasmaEditorEngineDocumentMsg* pMsg) override;

  const plJoltMeshResourceHandle& GetMesh() const { return m_hMesh; }

  bool m_bDisplayGrid = true;

protected:
  virtual void OnInitialize() override;

  virtual PlasmaEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(PlasmaEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(PlasmaEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const PlasmaEditorEngineDocumentMsg* pMsg);

  plGameObject* m_pMeshObject;
  plJoltMeshResourceHandle m_hMesh;
};
