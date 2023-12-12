#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Meshes/MeshResource.h>

class PLASMA_ENGINEPLUGINASSETS_DLL plDecalContext : public PlasmaEngineProcessDocumentContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDecalContext, PlasmaEngineProcessDocumentContext);

public:
  plDecalContext();

protected:
  virtual void OnInitialize() override;

  virtual PlasmaEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(PlasmaEngineProcessViewContext* pContext) override;

private:
  plMeshResourceHandle m_hPreviewMeshResource;

  // plDecalResourceHandle m_hDecal;
};
