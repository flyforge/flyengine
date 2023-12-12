#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/PickingRenderPass/PickingRenderPass.h>
#include <RendererCore/Lights/ClusteredDataProvider.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plPickingRenderPass, 1, plRTTIDefaultAllocator<plPickingRenderPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("PickSelected", m_bPickSelected),
    PLASMA_MEMBER_PROPERTY("PickTransparent", m_bPickTransparent),
    PLASMA_MEMBER_PROPERTY("PickingPosition", m_PickingPosition),
    PLASMA_MEMBER_PROPERTY("MarqueePickPos0", m_MarqueePickPosition0),
    PLASMA_MEMBER_PROPERTY("MarqueePickPos1", m_MarqueePickPosition1),
    PLASMA_MEMBER_PROPERTY("MarqueeActionID", m_uiMarqueeActionID),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plPickingRenderPass::plPickingRenderPass()
  : plRenderPipelinePass("EditorPickingRenderPass")
{
  m_PickingPosition.Set(-1);
  m_MarqueePickPosition0.Set(-1);
  m_MarqueePickPosition1.Set(-1);
}

plPickingRenderPass::~plPickingRenderPass()
{
  DestroyTarget();
}

plGALTextureHandle plPickingRenderPass::GetPickingIdRT() const
{
  return m_hPickingIdRT;
}

plGALTextureHandle plPickingRenderPass::GetPickingDepthRT() const
{
  return m_hPickingDepthRT;
}

bool plPickingRenderPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  m_TargetRect = view.GetViewport();

  return true;
}

void plPickingRenderPass::InitRenderPipelinePass(const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  DestroyTarget();
  CreateTarget();
}

void plPickingRenderPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const plRectFloat& viewPortRect = renderViewContext.m_pViewData->m_ViewPortRect;
  m_uiWindowWidth = (plUInt32)viewPortRect.width;
  m_uiWindowHeight = (plUInt32)viewPortRect.height;

  const plGALTexture* pDepthTexture = plGALDevice::GetDefaultDevice()->GetTexture(m_hPickingDepthRT);
  PLASMA_ASSERT_DEV(m_uiWindowWidth == pDepthTexture->GetDescription().m_uiWidth, "");
  PLASMA_ASSERT_DEV(m_uiWindowHeight == pDepthTexture->GetDescription().m_uiHeight, "");

  plGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup = m_RenderTargetSetup;
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_bClearDepth = true;
  renderingSetup.m_bClearStencil = true;

  auto pCommandEncoder = plRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName());

  plViewRenderMode::Enum viewRenderMode = renderViewContext.m_pViewData->m_ViewRenderMode;
  if (viewRenderMode == plViewRenderMode::WireframeColor || viewRenderMode == plViewRenderMode::WireframeMonochrome)
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_PICKING_WIREFRAME");
  else
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_PICKING");

  // Setup clustered data
  auto pClusteredData = GetPipeline()->GetFrameDataProvider<plClusteredDataProvider>()->GetData(renderViewContext);
  pClusteredData->BindResources(renderViewContext.m_pRenderContext);

  // copy selection to set for faster checks
  m_SelectionSet.Clear();

  auto batchList = GetPipeline()->GetRenderDataBatchesWithCategory(plDefaultRenderDataCategories::Selection);
  const plUInt32 uiBatchCount = batchList.GetBatchCount();
  for (plUInt32 i = 0; i < uiBatchCount; ++i)
  {
    const plRenderDataBatch& batch = batchList.GetBatch(i);
    for (auto it = batch.GetIterator<plRenderData>(); it.IsValid(); ++it)
    {
      m_SelectionSet.Insert(it->m_hOwner);
    }
  }

  // filter out all selected objects
  plRenderDataBatch::Filter filter([&](const plRenderData* pRenderData) { return m_SelectionSet.Contains(pRenderData->m_hOwner); });

  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::LitOpaque, filter);
  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::LitMasked, filter);

  if (m_bPickTransparent)
  {
    RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::LitTransparent, filter);

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
    RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::LitForeground);

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
    RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::LitForeground);
  }

  if (m_bPickSelected)
  {
    RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::Selection);
  }

  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::SimpleOpaque);

  if (m_bPickTransparent)
  {
    RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::SimpleTransparent, filter);
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::SimpleForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::SimpleForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_FORWARD");

  // download the picking information from the GPU
  {
    if (m_uiWindowWidth != 0 && m_uiWindowHeight != 0)
    {
      pCommandEncoder->ReadbackTexture(GetPickingDepthRT());

      plMat4 mProj;
      renderViewContext.m_pCamera->GetProjectionMatrix((float)m_uiWindowWidth / m_uiWindowHeight, mProj);
      plMat4 mView = renderViewContext.m_pCamera->GetViewMatrix();

      if (mProj.IsNaN())
        return;

      // Double precision version
      /*
      {
        plMat4d dView, dProj, dMVP;
        CopyMatD(dView, mView);
        CopyMatD(dProj, mProj);

        dMVP = dProj * dView;
        auto res = dMVP.Invert(0.00000001);

        if (res.Failed())
          plLog::Debug("Inversion of View-Projection-Matrix failed. Picking results will be wrong.");

        m_PickingInverseViewProjectionMatrix = dMVP;
      }
      */

      plMat4 inv = mProj * mView;
      if (inv.Invert(0).Failed())
      {
        plLog::Warning("Inversion of View-Projection-Matrix failed. Picking results will be wrong.");
        return;
      }

      m_mPickingInverseViewProjectionMatrix = inv;

      m_PickingResultsDepth.Clear();
      m_PickingResultsDepth.SetCountUninitialized(m_uiWindowWidth * m_uiWindowHeight);

      plGALSystemMemoryDescription MemDesc;
      MemDesc.m_uiRowPitch = 4 * m_uiWindowWidth;
      MemDesc.m_uiSlicePitch = 4 * m_uiWindowWidth * m_uiWindowHeight;

      MemDesc.m_pData = m_PickingResultsDepth.GetData();
      plArrayPtr<plGALSystemMemoryDescription> SysMemDescsDepth(&MemDesc, 1);
      plGALTextureSubresource sourceSubResource;
      plArrayPtr<plGALTextureSubresource> sourceSubResources(&sourceSubResource, 1);
      pCommandEncoder->CopyTextureReadbackResult(GetPickingDepthRT(), sourceSubResources, SysMemDescsDepth);
    }
  }

  {
    // download the picking information from the GPU
    if (m_uiWindowWidth != 0 && m_uiWindowHeight != 0)
    {
      pCommandEncoder->ReadbackTexture(GetPickingIdRT());

      plMat4 mProj;
      renderViewContext.m_pCamera->GetProjectionMatrix((float)m_uiWindowWidth / m_uiWindowHeight, mProj);
      plMat4 mView = renderViewContext.m_pCamera->GetViewMatrix();

      if (mProj.IsNaN())
        return;

      // Double precision version
      /*
      {
        plMat4d dView, dProj, dMVP;
        CopyMatD(dView, mView);
        CopyMatD(dProj, mProj);

        dMVP = dProj * dView;
        auto res = dMVP.Invert(0.00000001);

        if (res.Failed())
          plLog::Debug("Inversion of View-Projection-Matrix failed. Picking results will be wrong.");

        m_PickingInverseViewProjectionMatrix = dMVP;
      }
      */

      plMat4 inv = mProj * mView;
      if (inv.Invert(0).Failed())
      {
        plLog::Warning("Inversion of View-Projection-Matrix failed. Picking results will be wrong.");
        return;
      }

      m_mPickingInverseViewProjectionMatrix = inv;

      m_PickingResultsID.Clear();
      m_PickingResultsID.SetCountUninitialized(m_uiWindowWidth * m_uiWindowHeight);

      plGALSystemMemoryDescription MemDesc;
      MemDesc.m_uiRowPitch = 4 * m_uiWindowWidth;
      MemDesc.m_uiSlicePitch = 4 * m_uiWindowWidth * m_uiWindowHeight;

      MemDesc.m_pData = m_PickingResultsID.GetData();
      plArrayPtr<plGALSystemMemoryDescription> SysMemDescs(&MemDesc, 1);
      plGALTextureSubresource sourceSubResource;
      plArrayPtr<plGALTextureSubresource> sourceSubResources(&sourceSubResource, 1);
      pCommandEncoder->CopyTextureReadbackResult(GetPickingIdRT(), sourceSubResources, SysMemDescs);
    }
  }
}

void plPickingRenderPass::ReadBackProperties(plView* pView)
{
  ReadBackPropertiesSinglePick(pView);
  ReadBackPropertiesMarqueePick(pView);
}

void plPickingRenderPass::CreateTarget()
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  // Create render target for picking
  plGALTextureCreationDescription tcd;
  tcd.m_bAllowDynamicMipGeneration = false;
  tcd.m_bAllowShaderResourceView = false;
  tcd.m_bAllowUAV = false;
  tcd.m_bCreateRenderTarget = true;
  tcd.m_Format = plGALResourceFormat::RGBAUByteNormalized;
  tcd.m_ResourceAccess.m_bReadBack = true;
  tcd.m_Type = plGALTextureType::Texture2D;
  tcd.m_uiWidth = (plUInt32)m_TargetRect.width;
  tcd.m_uiHeight = (plUInt32)m_TargetRect.height;

  m_hPickingIdRT = pDevice->CreateTexture(tcd);

  tcd.m_Format = plGALResourceFormat::DFloat;
  tcd.m_ResourceAccess.m_bReadBack = true;

  m_hPickingDepthRT = pDevice->CreateTexture(tcd);

  m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(m_hPickingIdRT)).SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(m_hPickingDepthRT));
}

void plPickingRenderPass::DestroyTarget()
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  m_RenderTargetSetup.DestroyAllAttachedViews();
  if (!m_hPickingIdRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hPickingIdRT);
    m_hPickingIdRT.Invalidate();
  }

  if (!m_hPickingDepthRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hPickingDepthRT);
    m_hPickingDepthRT.Invalidate();
  }
}

void plPickingRenderPass::ReadBackPropertiesSinglePick(plView* pView)
{
  const plUInt32 x = (plUInt32)m_PickingPosition.x;
  const plUInt32 y = (plUInt32)m_PickingPosition.y;
  const plUInt32 uiIndex = (y * m_uiWindowWidth) + x;

  if (uiIndex >= m_PickingResultsDepth.GetCount() || x >= m_uiWindowWidth || y >= m_uiWindowHeight)
  {
    // plLog::Error("Picking position {0}, {1} is outside the available picking area of {2} * {3}", x, y, m_uiWindowWidth,
    // m_uiWindowHeight);
    return;
  }

  m_PickingPosition.Set(-1);

  plVec3 vNormal(0);
  plVec3 vPickingRayStartPosition(0);
  plVec3 vPickedPosition(0);
  {
    const float fDepth = m_PickingResultsDepth[uiIndex];
    plGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, plVec3((float)x, (float)(m_uiWindowHeight - y), fDepth), vPickedPosition).IgnoreResult();
    plGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, plVec3((float)x, (float)(m_uiWindowHeight - y), 0), vPickingRayStartPosition).IgnoreResult();

    float fOtherDepths[4] = {fDepth, fDepth, fDepth, fDepth};
    plVec3 vOtherPos[4];
    plVec3 vNormals[4];

    if ((plUInt32)x + 1 < m_uiWindowWidth)
      fOtherDepths[0] = m_PickingResultsDepth[(y * m_uiWindowWidth) + x + 1];
    if (x > 0)
      fOtherDepths[1] = m_PickingResultsDepth[(y * m_uiWindowWidth) + x - 1];
    if ((plUInt32)y + 1 < m_uiWindowHeight)
      fOtherDepths[2] = m_PickingResultsDepth[((y + 1) * m_uiWindowWidth) + x];
    if (y > 0)
      fOtherDepths[3] = m_PickingResultsDepth[((y - 1) * m_uiWindowWidth) + x];

    plGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, plVec3((float)(x + 1), (float)(m_uiWindowHeight - y), fOtherDepths[0]), vOtherPos[0]).IgnoreResult();
    plGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, plVec3((float)(x - 1), (float)(m_uiWindowHeight - y), fOtherDepths[1]), vOtherPos[1]).IgnoreResult();
    plGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, plVec3((float)x, (float)(m_uiWindowHeight - (y + 1)), fOtherDepths[2]), vOtherPos[2]).IgnoreResult();
    plGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, plVec3((float)x, (float)(m_uiWindowHeight - (y - 1)), fOtherDepths[3]), vOtherPos[3]).IgnoreResult();

    vNormals[0] = plPlane(vPickedPosition, vOtherPos[0], vOtherPos[2]).m_vNormal;
    vNormals[1] = plPlane(vPickedPosition, vOtherPos[2], vOtherPos[1]).m_vNormal;
    vNormals[2] = plPlane(vPickedPosition, vOtherPos[1], vOtherPos[3]).m_vNormal;
    vNormals[3] = plPlane(vPickedPosition, vOtherPos[3], vOtherPos[0]).m_vNormal;

    vNormal = (vNormals[0] + vNormals[1] + vNormals[2] + vNormals[3]).GetNormalized();
  }

  plUInt32 uiPickID = m_PickingResultsID[uiIndex];
  if (uiPickID == 0)
  {
    for (plInt32 radius = 1; radius < 10; ++radius)
    {
      plInt32 left = plMath::Max<plInt32>(x - radius, 0);
      plInt32 right = plMath::Min<plInt32>(x + radius, m_uiWindowWidth - 1);
      plInt32 top = plMath::Max<plInt32>(y - radius, 0);
      plInt32 bottom = plMath::Min<plInt32>(y + radius, m_uiWindowHeight - 1);

      for (plInt32 xt = left; xt <= right; ++xt)
      {
        const plUInt32 idxt = (top * m_uiWindowWidth) + xt;

        uiPickID = m_PickingResultsID[idxt];

        if (uiPickID != 0)
          goto done;
      }

      for (plInt32 xt = left; xt <= right; ++xt)
      {
        const plUInt32 idxt = (bottom * m_uiWindowWidth) + xt;

        uiPickID = m_PickingResultsID[idxt];

        if (uiPickID != 0)
          goto done;
      }
    }

  done:;
  }

  pView->SetRenderPassReadBackProperty(GetName(), "PickingMatrix", m_mPickingInverseViewProjectionMatrix);
  pView->SetRenderPassReadBackProperty(GetName(), "PickingID", uiPickID);
  pView->SetRenderPassReadBackProperty(GetName(), "PickingDepth", m_PickingResultsDepth[uiIndex]);
  pView->SetRenderPassReadBackProperty(GetName(), "PickingNormal", vNormal);
  pView->SetRenderPassReadBackProperty(GetName(), "PickingRayStartPosition", vPickingRayStartPosition);
  pView->SetRenderPassReadBackProperty(GetName(), "PickingPosition", vPickedPosition);
}

void plPickingRenderPass::ReadBackPropertiesMarqueePick(plView* pView)
{
  const plUInt32 x0 = (plUInt32)m_MarqueePickPosition0.x;
  const plUInt32 y0 = (plUInt32)m_MarqueePickPosition0.y;
  const plUInt32 x1 = (plUInt32)m_MarqueePickPosition1.x;
  const plUInt32 y1 = (plUInt32)m_MarqueePickPosition1.y;
  const plUInt32 uiIndex1 = (y0 * m_uiWindowWidth) + x0;
  const plUInt32 uiIndex2 = (y0 * m_uiWindowWidth) + x0;

  if ((uiIndex1 >= m_PickingResultsDepth.GetCount() || x0 >= m_uiWindowWidth || y0 >= m_uiWindowHeight) || (uiIndex2 >= m_PickingResultsDepth.GetCount() || x1 >= m_uiWindowWidth || y1 >= m_uiWindowHeight))
  {
    return;
  }

  m_MarqueePickPosition0.Set(-1);
  m_MarqueePickPosition1.Set(-1);
  pView->SetRenderPassReadBackProperty(GetName(), "MarqueeActionID", m_uiMarqueeActionID);

  plHybridArray<plUInt32, 32> IDs;
  plVariantArray resArray;

  const plUInt32 lowX = plMath::Min(x0, x1);
  const plUInt32 highX = plMath::Max(x0, x1);
  const plUInt32 lowY = plMath::Min(y0, y1);
  const plUInt32 highY = plMath::Max(y0, y1);

  plUInt32 offset = 0;

  for (plUInt32 y = lowY; y < highY; y += 1)
  {
    for (plUInt32 x = lowX + offset; x < highX; x += 2)
    {
      const plUInt32 uiIndex = (y * m_uiWindowWidth) + x;

      const plUInt32 id = m_PickingResultsID[uiIndex];

      // prevent duplicates
      if (IDs.Contains(id))
        continue;

      IDs.PushBack(id);
      resArray.PushBack(id);
    }

    // only evaluate every second pixel, in a checker board pattern
    offset = (offset + 1) % 2;
  }

  pView->SetRenderPassReadBackProperty(GetName(), "MarqueeResult", resArray);
}
