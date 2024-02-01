#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/Grid/GridRenderer.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Shader/ShaderResource.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plGridRenderData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditorGridExtractor, 1, plRTTIDefaultAllocator<plEditorGridExtractor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("SceneContext", GetSceneContext, SetSceneContext),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plGridRenderer, 1, plRTTIDefaultAllocator<plGridRenderer>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plEditorGridExtractor::plEditorGridExtractor(const char* szName)
  : plExtractor(szName)
{
  m_pSceneContext = nullptr;
}

plGridRenderer::plGridRenderer()
{
  CreateVertexBuffer();
}

void plGridRenderer::GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(plGetStaticRTTI<plGridRenderData>());
}

void plGridRenderer::GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(plDefaultRenderDataCategories::SimpleTransparent);
}

void plGridRenderer::CreateVertexBuffer()
{
  if (!m_hVertexBuffer.IsInvalidated())
    return;

  // load the shader
  {
    m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Debug/DebugPrimitive.plShader");
  }

  // Create the vertex buffer
  {
    plGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(GridVertex);
    desc.m_uiTotalSize = s_uiBufferSize;
    desc.m_BufferType = plGALBufferType::VertexBuffer;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hVertexBuffer = plGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }

  // Setup the vertex declaration
  {
    {
      plVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = plGALVertexAttributeSemantic::Position;
      si.m_Format = plGALResourceFormat::XYZFloat;
      si.m_uiOffset = 0;
      si.m_uiElementSize = 12;
    }

    {
      plVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = plGALVertexAttributeSemantic::Color0;
      si.m_Format = plGALResourceFormat::RGBAUByteNormalized;
      si.m_uiOffset = 12;
      si.m_uiElementSize = 4;
    }
  }
}

void plGridRenderer::CreateGrid(const plGridRenderData& rd) const
{
  m_Vertices.Clear();
  m_Vertices.Reserve(100);

  const plVec3 vCenter = rd.m_GlobalTransform.m_vPosition;
  const plVec3 vTangent1 = rd.m_GlobalTransform.m_qRotation * plVec3(1, 0, 0);
  const plVec3 vTangent2 = rd.m_GlobalTransform.m_qRotation * plVec3(0, 1, 0);
  const plInt32 iNumLines1 = rd.m_iLastLine1 - rd.m_iFirstLine1;
  const plInt32 iNumLines2 = rd.m_iLastLine2 - rd.m_iFirstLine2;
  const float maxExtent1 = iNumLines1 * rd.m_fDensity;
  const float maxExtent2 = iNumLines2 * rd.m_fDensity;

  plColor cCenter = plColorScheme::GetColor(plColorScheme::Blue, 6, 0.5f);
  plColor cTen = plColorScheme::LightUI(plColorScheme::Gray) * 0.7f;
  plColor cOther = plColorScheme::LightUI(plColorScheme::Gray) * 0.5f;

  if (rd.m_bOrthoMode)
  {
    // dimmer colors in ortho mode, to be more in the background
    cCenter *= 0.5f;
    cTen *= 0.4f;
    cOther *= 0.4f;

    // in ortho mode, the origin lines are highlighted when global space is enabled
    if (!rd.m_bGlobal)
    {
      cCenter = cTen;
    }
  }
  else
  {
    // in perspective mode, the lines through the object are highlighted when local space is enabled
    if (rd.m_bGlobal)
    {
      cCenter = cTen;
    }
  }

  const plVec3 vCorner = vCenter + rd.m_iFirstLine1 * rd.m_fDensity * vTangent1 + rd.m_iFirstLine2 * rd.m_fDensity * vTangent2;

  for (plInt32 i = 0; i <= iNumLines1; ++i)
  {
    const plInt32 iLineIdx = rd.m_iFirstLine1 + i;

    plColor cCur = cOther;

    if (iLineIdx == 0)
      cCur = cCenter;
    else if (iLineIdx % 10 == 0)
      cCur = cTen;

    auto& v1 = m_Vertices.ExpandAndGetRef();
    auto& v2 = m_Vertices.ExpandAndGetRef();

    v1.m_color = cCur;
    v1.m_position = vCorner + vTangent1 * rd.m_fDensity * (float)i;

    v2.m_color = cCur;
    v2.m_position = vCorner + vTangent1 * rd.m_fDensity * (float)i + vTangent2 * maxExtent2;
  }

  for (plInt32 i = 0; i <= iNumLines2; ++i)
  {
    const plInt32 iLineIdx = rd.m_iFirstLine2 + i;

    plColor cCur = cOther;

    if (iLineIdx == 0)
      cCur = cCenter;
    else if (iLineIdx % 10 == 0)
      cCur = cTen;

    auto& v1 = m_Vertices.ExpandAndGetRef();
    auto& v2 = m_Vertices.ExpandAndGetRef();

    v1.m_color = cCur;
    v1.m_position = vCorner + vTangent2 * rd.m_fDensity * (float)i;

    v2.m_color = cCur;
    v2.m_position = vCorner + vTangent2 * rd.m_fDensity * (float)i + vTangent1 * maxExtent1;
  }
}

void plGridRenderer::RenderBatch(const plRenderViewContext& renderViewContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const
{
  for (auto it = batch.GetIterator<plGridRenderData>(); it.IsValid(); ++it)
  {
    CreateGrid(*it);

    if (m_Vertices.IsEmpty())
      return;

    plRenderContext* pRenderContext = renderViewContext.m_pRenderContext;

    pRenderContext->SetShaderPermutationVariable("PRE_TRANSFORMED_VERTICES", "FALSE");
    pRenderContext->BindShader(m_hShader);

    plUInt32 uiNumLineVertices = m_Vertices.GetCount();
    const GridVertex* pLineData = m_Vertices.GetData();

    while (uiNumLineVertices > 0)
    {
      const plUInt32 uiNumLineVerticesInBatch = plMath::Min<plUInt32>(uiNumLineVertices, s_uiLineVerticesPerBatch);
      PL_ASSERT_DEBUG(uiNumLineVerticesInBatch % 2 == 0, "Vertex count must be a multiple of 2.");

      pRenderContext->GetCommandEncoder()->UpdateBuffer(m_hVertexBuffer, 0, plMakeArrayPtr(pLineData, uiNumLineVerticesInBatch).ToByteArray());

      pRenderContext->BindMeshBuffer(m_hVertexBuffer, plGALBufferHandle(), &m_VertexDeclarationInfo, plGALPrimitiveTopology::Lines, uiNumLineVerticesInBatch / 2);
      pRenderContext->DrawMeshBuffer().IgnoreResult();

      uiNumLineVertices -= uiNumLineVerticesInBatch;
      pLineData += s_uiLineVerticesPerBatch;
    }
  }
}

float AdjustGridDensity(float fDensity, plUInt32 uiWindowWidth, float fOrthoDimX, plUInt32 uiMinPixelsDist)
{
  plInt32 iFactor = 1;
  float fNewDensity = fDensity;

  while (true)
  {
    const float stepsAtDensity = fOrthoDimX / fNewDensity;
    const float minPixelsAtDensity = stepsAtDensity * uiMinPixelsDist;

    if (minPixelsAtDensity < uiWindowWidth)
      break;

    iFactor *= 10;
    fNewDensity = fDensity * iFactor;
  }

  return fNewDensity;
}

void plEditorGridExtractor::Extract(const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& ref_extractedRenderData)
{
  if (m_pSceneContext == nullptr || m_pSceneContext->GetGridDensity() == 0.0f)
    return;

  const plCamera* cam = view.GetCamera();
  float fDensity = m_pSceneContext->GetGridDensity();

  plGridRenderData* pRenderData = plCreateRenderDataForThisFrame<plGridRenderData>(nullptr);
  pRenderData->m_GlobalBounds = plBoundingBoxSphere::MakeInvalid();
  pRenderData->m_bOrthoMode = cam->IsOrthographic();
  pRenderData->m_bGlobal = m_pSceneContext->IsGridInGlobalSpace();

  if (cam->IsOrthographic())
  {
    const float fAspectRatio = view.GetViewport().width / view.GetViewport().height;
    const float fDimX = cam->GetDimensionX(fAspectRatio) * 0.5f;
    const float fDimY = cam->GetDimensionY(fAspectRatio) * 0.5f;

    fDensity = AdjustGridDensity(fDensity, (plUInt32)view.GetViewport().width, fDimX, 10);
    pRenderData->m_fDensity = fDensity;

    pRenderData->m_GlobalTransform.SetIdentity();
    pRenderData->m_GlobalTransform.m_vPosition = cam->GetCenterDirForwards() * cam->GetFarPlane() * 0.9f;

    plMat3 mRot;
    mRot.SetColumn(0, cam->GetCenterDirRight());
    mRot.SetColumn(1, cam->GetCenterDirUp());
    mRot.SetColumn(2, cam->GetCenterDirForwards());
    pRenderData->m_GlobalTransform.m_qRotation = plQuat::MakeFromMat3(mRot);

    const plVec3 vBottomLeft = cam->GetCenterPosition() - cam->GetCenterDirRight() * fDimX - cam->GetCenterDirUp() * fDimY;
    const plVec3 vTopRight = cam->GetCenterPosition() + cam->GetCenterDirRight() * fDimX + cam->GetCenterDirUp() * fDimY;

    plPlane plane1, plane2;
    plane1 = plPlane::MakeFromNormalAndPoint(cam->GetCenterDirRight(), plVec3(0));
    plane2 = plPlane::MakeFromNormalAndPoint(cam->GetCenterDirUp(), plVec3(0));

    const float fFirstDist1 = plane1.GetDistanceTo(vBottomLeft) - fDensity;
    const float fLastDist1 = plane1.GetDistanceTo(vTopRight) + fDensity;

    const float fFirstDist2 = plane2.GetDistanceTo(vBottomLeft) - fDensity;
    const float fLastDist2 = plane2.GetDistanceTo(vTopRight) + fDensity;


    plVec3& val = pRenderData->m_GlobalTransform.m_vPosition;
    val.x = plMath::RoundToMultiple(val.x, pRenderData->m_fDensity);
    val.y = plMath::RoundToMultiple(val.y, pRenderData->m_fDensity);
    val.z = plMath::RoundToMultiple(val.z, pRenderData->m_fDensity);

    pRenderData->m_iFirstLine1 = (plInt32)plMath::Trunc(fFirstDist1 / fDensity);
    pRenderData->m_iLastLine1 = (plInt32)plMath::Trunc(fLastDist1 / fDensity);
    pRenderData->m_iFirstLine2 = (plInt32)plMath::Trunc(fFirstDist2 / fDensity);
    pRenderData->m_iLastLine2 = (plInt32)plMath::Trunc(fLastDist2 / fDensity);
  }
  else
  {
    pRenderData->m_GlobalTransform = m_pSceneContext->GetGridTransform();

    // grid is disabled
    if (pRenderData->m_GlobalTransform.m_vScale.IsZero(0.001f))
      return;

    pRenderData->m_fDensity = fDensity;

    const plInt32 iNumLines = 50;
    pRenderData->m_iFirstLine1 = -iNumLines;
    pRenderData->m_iLastLine1 = iNumLines;
    pRenderData->m_iFirstLine2 = -iNumLines;
    pRenderData->m_iLastLine2 = iNumLines;
  }

  ref_extractedRenderData.AddRenderData(pRenderData, plDefaultRenderDataCategories::SimpleTransparent);
}

plResult plEditorGridExtractor::Serialize(plStreamWriter& inout_stream) const
{
  PL_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return PL_SUCCESS;
}


plResult plEditorGridExtractor::Deserialize(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PL_IGNORE_UNUSED(uiVersion);
  return PL_SUCCESS;
}
