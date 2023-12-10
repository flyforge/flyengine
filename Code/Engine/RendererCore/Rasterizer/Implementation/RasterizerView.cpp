#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/Rasterizer/RasterizerView.h>
#include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#include <RendererCore/Rasterizer/Thirdparty/Rasterizer.h>

plCVarInt cvar_SpatialCullingOcclusionMaxResolution("Spatial.Occlusion.MaxResolution", 512, plCVarFlags::Default, "Max resolution for occlusion buffers.");
plCVarInt cvar_SpatialCullingOcclusionMaxOccluders("Spatial.Occlusion.MaxOccluders", 64, plCVarFlags::Default, "Max number of occluders to rasterize per frame.");

plRasterizerView::plRasterizerView() = default;
plRasterizerView::~plRasterizerView() = default;

void plRasterizerView::SetResolution(plUInt32 uiWidth, plUInt32 uiHeight, float fAspectRatio)
{
  if (m_uiResolutionX != uiWidth || m_uiResolutionY != uiHeight)
  {
    m_uiResolutionX = uiWidth;
    m_uiResolutionY = uiHeight;

    m_pRasterizer = PLASMA_DEFAULT_NEW(Rasterizer, uiWidth, uiHeight);
  }

  if (fAspectRatio == 0.0f)
    m_fAspectRation = float(m_uiResolutionX) / float(m_uiResolutionY);
  else
    m_fAspectRation = fAspectRatio;
}

void plRasterizerView::BeginScene()
{
  PLASMA_ASSERT_DEV(m_pRasterizer != nullptr, "Call SetResolution() first.");

  PLASMA_PROFILE_SCOPE("Occlusion::Clear");

  m_pRasterizer->clear();
  m_bAnyOccludersRasterized = false;
}

void plRasterizerView::ReadBackFrame(plArrayPtr<plColorLinearUB> targetBuffer) const
{
  PLASMA_PROFILE_SCOPE("Occlusion::ReadFrame");

  PLASMA_ASSERT_DEV(m_pRasterizer != nullptr, "Call SetResolution() first.");
  PLASMA_ASSERT_DEV(targetBuffer.GetCount() >= m_uiResolutionX * m_uiResolutionY, "Target buffer is too small.");

  m_pRasterizer->readBackDepth(targetBuffer.GetPtr());
}

void plRasterizerView::EndScene()
{
  if (m_Instances.IsEmpty())
    return;

  PLASMA_PROFILE_SCOPE("Occlusion::RasterizeScene");

  SortObjectsFrontToBack();

  UpdateViewProjectionMatrix();

  // only rasterize a limited number of the closest objects
  RasterizeObjects(cvar_SpatialCullingOcclusionMaxOccluders);

  m_Instances.Clear();

  m_pRasterizer->setModelViewProjection(m_mViewProjection.m_fElementsCM);
}

void plRasterizerView::RasterizeObjects(plUInt32 uiMaxObjects)
{
#if PLASMA_ENABLED(PLASMA_RASTERIZER_SUPPORTED)

  PLASMA_PROFILE_SCOPE("Occlusion::RasterizeObjects");

  for (const Instance& inst : m_Instances)
  {
    ApplyModelViewProjectionMatrix(inst.m_Transform);

    bool bNeedsClipping;
    const Occluder& occluder = inst.m_pObject->m_Occluder;

    if (m_pRasterizer->queryVisibility(occluder.m_boundsMin, occluder.m_boundsMax, bNeedsClipping))
    {
      m_bAnyOccludersRasterized = true;

      if (bNeedsClipping)
      {
        m_pRasterizer->rasterize<true>(occluder);
      }
      else
      {
        m_pRasterizer->rasterize<false>(occluder);
      }

      if (--uiMaxObjects == 0)
        return;
    }
  }
#endif
}

void plRasterizerView::UpdateViewProjectionMatrix()
{
  plMat4 mProjection;
  m_pCamera->GetProjectionMatrix(m_fAspectRation, mProjection, plCameraEye::Left, plClipSpaceDepthRange::ZeroToOne);

  m_mViewProjection = mProjection * m_pCamera->GetViewMatrix();
}

void plRasterizerView::ApplyModelViewProjectionMatrix(const plTransform& modelTransform)
{
  const plMat4 mModel = modelTransform.GetAsMat4();
  const plMat4 mMVP = m_mViewProjection * mModel;

  m_pRasterizer->setModelViewProjection(mMVP.m_fElementsCM);
}

void plRasterizerView::SortObjectsFrontToBack()
{
#if PLASMA_ENABLED(PLASMA_RASTERIZER_SUPPORTED)
  PLASMA_PROFILE_SCOPE("Occlusion::SortObjects");

  const plVec3 camPos = m_pCamera->GetCenterPosition();

  m_Instances.Sort([&](const Instance& i1, const Instance& i2) {
      const float d1 = (i1.m_Transform.m_vPosition - camPos).GetLengthSquared();
      const float d2 = (i2.m_Transform.m_vPosition - camPos).GetLengthSquared();

      return d1 < d2; });
#endif
}

bool plRasterizerView::IsVisible(const plSimdBBox& aabb) const
{
#if PLASMA_ENABLED(PLASMA_RASTERIZER_SUPPORTED)
  if (!m_bAnyOccludersRasterized)
    return true; // assume that people already do frustum culling anyway

  PLASMA_PROFILE_SCOPE("Occlusion::IsVisible");

  plSimdVec4f vmin = aabb.m_Min;
  plSimdVec4f vmax = aabb.m_Max;

  // plSimdBBox makes no guarantees what's in the W component
  // but the SW rasterizer requires them to be 1
  vmin.SetW(1);
  vmax.SetW(1);

  bool needsClipping = false;
  return m_pRasterizer->queryVisibility(vmin.m_v, vmax.m_v, needsClipping);
#else
  return true;
#endif
}

plRasterizerView* plRasterizerViewPool::GetRasterizerView(plUInt32 uiWidth, plUInt32 uiHeight, float fAspectRatio)
{
  PLASMA_PROFILE_SCOPE("Occlusion::GetViewFromPool");

  PLASMA_LOCK(m_Mutex);

  const float divX = (float)uiWidth / (float)cvar_SpatialCullingOcclusionMaxResolution;
  const float divY = (float)uiHeight / (float)cvar_SpatialCullingOcclusionMaxResolution;
  const float div = plMath::Max(divX, divY);

  if (div > 1.0)
  {
    uiWidth = (plUInt32)(uiWidth / div);
    uiHeight = (plUInt32)(uiHeight / div);
  }

  uiWidth = plMath::RoundDown(uiWidth, 8);
  uiHeight = plMath::RoundDown(uiHeight, 8);

  uiWidth = plMath::Clamp<plUInt32>(uiWidth, 32u, cvar_SpatialCullingOcclusionMaxResolution);
  uiHeight = plMath::Clamp<plUInt32>(uiHeight, 32u, cvar_SpatialCullingOcclusionMaxResolution);

  for (PoolEntry& entry : m_Entries)
  {
    if (entry.m_bInUse)
      continue;

    if (entry.m_RasterizerView.GetResolutionX() == uiWidth && entry.m_RasterizerView.GetResolutionY() == uiHeight)
    {
      entry.m_bInUse = true;
      entry.m_RasterizerView.SetResolution(uiWidth, uiHeight, fAspectRatio);
      return &entry.m_RasterizerView;
    }
  }

  auto& ne = m_Entries.ExpandAndGetRef();
  ne.m_RasterizerView.SetResolution(uiWidth, uiHeight, fAspectRatio);
  ne.m_bInUse = true;

  return &ne.m_RasterizerView;
}

void plRasterizerViewPool::ReturnRasterizerView(plRasterizerView* pView)
{
  if (pView == nullptr)
    return;

  PLASMA_PROFILE_SCOPE("Occlusion::ReturnViewToPool");

  pView->SetCamera(nullptr);

  PLASMA_LOCK(m_Mutex);

  for (PoolEntry& entry : m_Entries)
  {
    if (&entry.m_RasterizerView == pView)
    {
      entry.m_bInUse = false;
      return;
    }
  }

  PLASMA_ASSERT_NOT_IMPLEMENTED;
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Rasterizer_Implementation_RasterizerView);
