#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>

class plObjectSelectionMsgToEngine;
class plRenderContext;

class PLASMA_ENGINEPLUGINASSETS_DLL plTextureContext : public PlasmaEngineProcessDocumentContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTextureContext, PlasmaEngineProcessDocumentContext);

public:
  plTextureContext();

  virtual void HandleMessage(const PlasmaEditorEngineDocumentMsg* pMsg) override;

  const plTexture2DResourceHandle& GetTexture() const { return m_hTexture; }

protected:
  virtual void OnInitialize() override;

  virtual PlasmaEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(PlasmaEngineProcessViewContext* pContext) override;

private:
  void OnResourceEvent(const plResourceEvent& e);

  plGameObjectHandle m_hPreviewObject;
  plComponentHandle m_hPreviewMesh2D;
  plMeshResourceHandle m_hPreviewMeshResource;
  plMaterialResourceHandle m_hMaterial;
  plTexture2DResourceHandle m_hTexture;

  plEvent<const plResourceEvent&, plMutex>::Unsubscriber m_TextureResourceEventSubscriber;
};
