#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginKraut/EnginePluginKrautDLL.h>
#include <KrautPlugin/Components/KrautTreeComponent.h>
#include <RendererCore/Meshes/MeshResource.h>

class plObjectSelectionMsgToEngine;
class plRenderContext;

class PLASMA_ENGINEPLUGINKRAUT_DLL plKrautTreeContext : public PlasmaEngineProcessDocumentContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plKrautTreeContext, PlasmaEngineProcessDocumentContext);

public:
  plKrautTreeContext();

  virtual void HandleMessage(const PlasmaEditorEngineDocumentMsg* pMsg) override;
  const plKrautGeneratorResourceHandle& GetResource() const { return m_hMainResource; }

protected:
  virtual void OnInitialize() override;

  virtual PlasmaEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(PlasmaEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(PlasmaEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const PlasmaEditorEngineDocumentMsg* pMsg);

  plGameObject* m_pMainObject;
  plComponentHandle m_hKrautComponent;
  plKrautGeneratorResourceHandle m_hMainResource;
  plMeshResourceHandle m_hPreviewMeshResource;
  plUInt32 m_uiDisplayRandomSeed = 0xFFFFFFFF;
};
