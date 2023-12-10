#include <EnginePluginJolt/EnginePluginJoltPCH.h>

#include <EnginePluginJolt/CollisionMeshAsset/JoltCollisionMeshContext.h>
#include <EnginePluginJolt/CollisionMeshAsset/JoltCollisionMeshView.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

plJoltCollisionMeshViewContext::plJoltCollisionMeshViewContext(plJoltCollisionMeshContext* pMeshContext)
  : plEngineProcessViewContext(pMeshContext)
{
  m_pContext = pMeshContext;

  // Start with something valid.
  m_Camera.SetCameraMode(plCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(plVec3(1, 1, 1), plVec3::MakeZero(), plVec3(0.0f, 0.0f, 1.0f));
}

plJoltCollisionMeshViewContext::~plJoltCollisionMeshViewContext() {}

bool plJoltCollisionMeshViewContext::UpdateThumbnailCamera(const plBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -plVec3(5, -2, 3));
}


plViewHandle plJoltCollisionMeshViewContext::CreateView()
{
  plView* pView = nullptr;
  plRenderWorld::CreateView("Collision Mesh Editor - View", pView);
  pView->SetCameraUsageHint(plCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  plEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void plJoltCollisionMeshViewContext::SetCamera(const plViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    plEngineProcessViewContext::DrawSimpleGrid();
  }

  plEngineProcessViewContext::SetCamera(pMsg);

  const plUInt32 viewHeight = pMsg->m_uiWindowHeight;

  auto hResource = m_pContext->GetMesh();
  if (hResource.IsValid())
  {
    plResourceLock<plJoltMeshResource> pResource(hResource, plResourceAcquireMode::AllowLoadingFallback);
    plBoundingBox bbox = pResource->GetBounds().GetBox();
    plUInt32 uiNumTris = pResource->GetNumTriangles();
    plUInt32 uiNumVertices = pResource->GetNumVertices();
    plUInt32 uiNumPieces = pResource->GetNumConvexParts() + (pResource->HasTriangleMesh() ? 1 : 0);

    plStringBuilder sText;
    sText.AppendFormat("Triangles: \t{}\n", uiNumTris);
    sText.AppendFormat("Vertices: \t{}\n", uiNumVertices);
    sText.AppendFormat("Pieces: \t{}\n", uiNumPieces);
    sText.AppendFormat("Bounding Box: \twidth={0}, depth={1}, height={2}", plArgF(bbox.GetHalfExtents().x * 2, 2),
      plArgF(bbox.GetHalfExtents().y * 2, 2), plArgF(bbox.GetHalfExtents().z * 2, 2));

    plDebugRenderer::DrawInfoText(m_hView, plDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
