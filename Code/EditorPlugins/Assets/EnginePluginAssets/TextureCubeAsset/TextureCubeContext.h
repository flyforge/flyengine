#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>

class plObjectSelectionMsgToEngine;
class plRenderContext;

class PLASMA_ENGINEPLUGINASSETS_DLL plTextureCubeContext : public plEngineProcessDocumentContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTextureCubeContext, plEngineProcessDocumentContext);

public:
  plTextureCubeContext();

  virtual void HandleMessage(const plEditorEngineDocumentMsg* pMsg) override;

  const plTextureCubeResourceHandle& GetTexture() const { return m_hTexture; }

protected:
  virtual void OnInitialize() override;

  virtual plEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(plEngineProcessViewContext* pContext) override;

private:
  void OnResourceEvent(const plResourceEvent& e);

  plGameObjectHandle m_hPreviewObject;
  plComponentHandle m_hPreviewMesh2D;
  plMeshResourceHandle m_hPreviewMeshResource;
  plMaterialResourceHandle m_hMaterial;
  plTextureCubeResourceHandle m_hTexture;

  plEvent<const plResourceEvent&, plMutex>::Unsubscriber m_TextureResourceEventSubscriber;
};
