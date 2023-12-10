#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Meshes/MeshResource.h>

class PLASMA_ENGINEPLUGINASSETS_DLL plDecalContext : public plEngineProcessDocumentContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDecalContext, plEngineProcessDocumentContext);

public:
  plDecalContext();

protected:
  virtual void OnInitialize() override;

  virtual plEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(plEngineProcessViewContext* pContext) override;

private:
  plMeshResourceHandle m_hPreviewMeshResource;

  // plDecalResourceHandle m_hDecal;
};
