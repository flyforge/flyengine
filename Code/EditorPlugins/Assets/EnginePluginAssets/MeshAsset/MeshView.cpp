#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/MeshAsset/MeshContext.h>
#include <EnginePluginAssets/MeshAsset/MeshView.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Resources/Buffer.h>

plMeshViewContext::plMeshViewContext(plMeshContext* pMeshContext)
  : plEngineProcessViewContext(pMeshContext)
{
  m_pContext = pMeshContext;

  // Start with something valid.
  m_Camera.SetCameraMode(plCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(plVec3(1, 1, 1), plVec3::MakeZero(), plVec3(0.0f, 0.0f, 1.0f));
}

plMeshViewContext::~plMeshViewContext() = default;

bool plMeshViewContext::UpdateThumbnailCamera(const plBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -plVec3(5, -2, 3));
}


plViewHandle plMeshViewContext::CreateView()
{
  plView* pView = nullptr;
  plRenderWorld::CreateView("Mesh Editor - View", pView);
  pView->SetCameraUsageHint(plCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  plEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void plMeshViewContext::SetCamera(const plViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    plEngineProcessViewContext::DrawSimpleGrid();
  }

  plEngineProcessViewContext::SetCamera(pMsg);

  auto hMesh = m_pContext->GetMesh();
  if (hMesh.IsValid())
  {
    plResourceLock<plMeshResource> pMesh(hMesh, plResourceAcquireMode::AllowLoadingFallback);
    plResourceLock<plMeshBufferResource> pMeshBuffer(pMesh->GetMeshBuffer(), plResourceAcquireMode::AllowLoadingFallback);

    auto& bufferDesc = plGALDevice::GetDefaultDevice()->GetBuffer(pMeshBuffer->GetVertexBuffer())->GetDescription();

    plUInt32 uiNumVertices = bufferDesc.m_uiTotalSize / bufferDesc.m_uiStructSize;
    plUInt32 uiNumTriangles = pMeshBuffer->GetPrimitiveCount();
    const plBoundingBox& bbox = pMeshBuffer->GetBounds().GetBox();

    plUInt32 uiNumUVs = 0;
    plUInt32 uiNumColors = 0;
    for (auto& vertexStream : pMeshBuffer->GetVertexDeclaration().m_VertexStreams)
    {
      if (vertexStream.m_Semantic >= plGALVertexAttributeSemantic::TexCoord0 && vertexStream.m_Semantic <= plGALVertexAttributeSemantic::TexCoord9)
      {
        ++uiNumUVs;
      }
      else if (vertexStream.m_Semantic >= plGALVertexAttributeSemantic::Color0 && vertexStream.m_Semantic <= plGALVertexAttributeSemantic::Color7)
      {
        ++uiNumColors;
      }
    }

    plStringBuilder sText;
    sText.AppendFormat("Triangles: \t{}\t\n", uiNumTriangles);
    sText.AppendFormat("Vertices: \t{}\t\n", uiNumVertices);
    sText.AppendFormat("UV Channels: \t{}\t\n", uiNumUVs);
    sText.AppendFormat("Color Channels: \t{}\t\n", uiNumColors);
    sText.AppendFormat("Bytes Per Vertex: \t{}\t\n", bufferDesc.m_uiStructSize);
    sText.AppendFormat("Bounding Box: \twidth={0}, depth={1}, height={2}\t", plArgF(bbox.GetHalfExtents().x * 2, 2),
      plArgF(bbox.GetHalfExtents().y * 2, 2), plArgF(bbox.GetHalfExtents().z * 2, 2));

    plDebugRenderer::DrawInfoText(m_hView, plDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
