#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/AnimatedMeshAsset/AnimatedMeshContext.h>
#include <EnginePluginAssets/AnimatedMeshAsset/AnimatedMeshView.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Resources/Buffer.h>

plAnimatedMeshViewContext::plAnimatedMeshViewContext(plAnimatedMeshContext* pAnimatedMeshContext)
  : PlasmaEngineProcessViewContext(pAnimatedMeshContext)
{
  m_pContext = pAnimatedMeshContext;

  // Start with something valid.
  m_Camera.SetCameraMode(plCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(plVec3(1, 1, 1), plVec3::ZeroVector(), plVec3(0.0f, 0.0f, 1.0f));
}

plAnimatedMeshViewContext::~plAnimatedMeshViewContext() {}

bool plAnimatedMeshViewContext::UpdateThumbnailCamera(const plBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -plVec3(5, -2, 3));
}


plViewHandle plAnimatedMeshViewContext::CreateView()
{
  plView* pView = nullptr;
  plRenderWorld::CreateView("AnimatedMesh Editor - View", pView);
  pView->SetCameraUsageHint(plCameraUsageHint::EditorView);
  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  PlasmaEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void plAnimatedMeshViewContext::SetCamera(const plViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    PlasmaEngineProcessViewContext::DrawSimpleGrid();
  }

  PlasmaEngineProcessViewContext::SetCamera(pMsg);

  const plUInt32 viewHeight = pMsg->m_uiWindowHeight;

  auto hAnimatedMesh = m_pContext->GetAnimatedMesh();
  if (hAnimatedMesh.IsValid())
  {
    plResourceLock<plMeshResource> pAnimatedMesh(hAnimatedMesh, plResourceAcquireMode::AllowLoadingFallback);
    plResourceLock<plMeshBufferResource> pAnimatedMeshBuffer(pAnimatedMesh->GetMeshBuffer(), plResourceAcquireMode::AllowLoadingFallback);

    auto& bufferDesc = plGALDevice::GetDefaultDevice()->GetBuffer(pAnimatedMeshBuffer->GetVertexBuffer())->GetDescription();

    plUInt32 uiNumVertices = bufferDesc.m_uiTotalSize / bufferDesc.m_uiStructSize;
    plUInt32 uiNumTriangles = pAnimatedMeshBuffer->GetPrimitiveCount();
    const plBoundingBox& bbox = pAnimatedMeshBuffer->GetBounds().GetBox();

    plUInt32 uiNumUVs = 0;
    plUInt32 uiNumColors = 0;
    for (auto& vertexStream : pAnimatedMeshBuffer->GetVertexDeclaration().m_VertexStreams)
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
    sText.AppendFormat("Bones: \t{}\n", pAnimatedMesh->m_Bones.GetCount());
    sText.AppendFormat("Triangles: \t{}\n", uiNumTriangles);
    sText.AppendFormat("Vertices: \t{}\n", uiNumVertices);
    sText.AppendFormat("UV Channels: \t{}\n", uiNumUVs);
    sText.AppendFormat("Color Channels: \t{}\n", uiNumColors);
    sText.AppendFormat("Bytes Per Vertex: \t{}\n", bufferDesc.m_uiStructSize);
    sText.AppendFormat("Bounding Box: \twidth={0}, depth={1}, height={2}", plArgF(bbox.GetHalfExtents().x * 2, 2),
      plArgF(bbox.GetHalfExtents().y * 2, 2), plArgF(bbox.GetHalfExtents().z * 2, 2));

    plDebugRenderer::DrawInfoText(m_hView, plDebugTextPlacement::BottomLeft, "AssetStats", sText);
  }
}
