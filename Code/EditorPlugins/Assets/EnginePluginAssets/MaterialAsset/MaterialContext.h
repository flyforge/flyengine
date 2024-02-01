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

class PL_ENGINEPLUGINASSETS_DLL plMaterialContext : public plEngineProcessDocumentContext
{
  PL_ADD_DYNAMIC_REFLECTION(plMaterialContext, plEngineProcessDocumentContext);

public:
  plMaterialContext();

  virtual void HandleMessage(const plEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;

  virtual plEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(plEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext) override;

private:
  plMaterialResourceHandle m_hMaterial;
  plMeshResourceHandle m_hBallMesh;
  plMeshResourceHandle m_hSphereMesh;
  plMeshResourceHandle m_hBoxMesh;
  plMeshResourceHandle m_hPlaneMesh;
  plComponentHandle m_hMeshComponent;

  enum class PreviewModel : plUInt8
  {
    Ball,
    Sphere,
    Box,
    Plane,
  };

  PreviewModel m_PreviewModel = PreviewModel::Ball;
};
