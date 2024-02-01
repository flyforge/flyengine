#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Components/FogComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/ClusteredDataExtractor.h>
#include <RendererCore/Lights/Implementation/ClusteredDataUtils.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
plCVarBool cvar_RenderingLightingVisClusterData("Rendering.Lighting.VisClusterData", false, plCVarFlags::Default, "Enables debug visualization of clustered light data");
plCVarInt cvar_RenderingLightingVisClusterDepthSlice("Rendering.Lighting.VisClusterDepthSlice", -1, plCVarFlags::Default, "Show the debug visualization only for the given depth slice");

namespace
{
  void VisualizeClusteredData(const plView& view, const plClusteredDataCPU* pData, plArrayPtr<plSimdBSphere> boundingSpheres)
  {
    if (!cvar_RenderingLightingVisClusterData)
      return;

    const plCamera* pCamera = view.GetCullingCamera();

    if (pCamera->IsOrthographic())
      return;

    float fAspectRatio = view.GetViewport().width / view.GetViewport().height;

    plMat4 mProj;
    pCamera->GetProjectionMatrix(fAspectRatio, mProj);

    plAngle fFovLeft;
    plAngle fFovRight;
    plAngle fFovBottom;
    plAngle fFovTop;
    plGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fFovLeft, fFovRight, fFovBottom, fFovTop);

    const float fTanLeft = plMath::Tan(fFovLeft);
    const float fTanRight = plMath::Tan(fFovRight);
    const float fTanBottom = plMath::Tan(fFovBottom);
    const float fTanTop = plMath::Tan(fFovTop);

    plColor lineColor = plColor(1.0f, 1.0f, 1.0f, 0.1f);

    plInt32 debugSlice = cvar_RenderingLightingVisClusterDepthSlice;
    plUInt32 maxSlice = debugSlice < 0 ? NUM_CLUSTERS_Z : debugSlice + 1;
    plUInt32 minSlice = debugSlice < 0 ? 0 : debugSlice;

    bool bDrawBoundingSphere = false;

    for (plUInt32 z = maxSlice; z-- > minSlice;)
    {
      float fZf = GetDepthFromSliceIndex(z);
      float fZn = (z > 0) ? GetDepthFromSliceIndex(z - 1) : 0.0f;
      for (plInt32 y = 0; y < NUM_CLUSTERS_Y; ++y)
      {
        for (plInt32 x = 0; x < NUM_CLUSTERS_X; ++x)
        {
          plUInt32 clusterIndex = GetClusterIndexFromCoord(x, y, z);
          auto& clusterData = pData->m_ClusterData[clusterIndex];

          if (clusterData.counts > 0)
          {
            if (bDrawBoundingSphere)
            {
              plBoundingSphere s = plSimdConversion::ToBSphere(boundingSpheres[clusterIndex]);
              plDebugRenderer::DrawLineSphere(view.GetHandle(), s, lineColor);
            }

            plVec3 cc[8];
            GetClusterCornerPoints(*pCamera, fZf, fZn, fTanLeft, fTanRight, fTanBottom, fTanTop, x, y, z, cc);

            float lightCount = (float)GET_LIGHT_INDEX(clusterData.counts);
            float decalCount = (float)GET_DECAL_INDEX(clusterData.counts);
            float r = plMath::Clamp(lightCount / 16.0f, 0.0f, 1.0f);
            float g = plMath::Clamp(decalCount / 16.0f, 0.0f, 1.0f);

            plDebugRenderer::Triangle tris[12];
            // back
            tris[0] = plDebugRenderer::Triangle(cc[0], cc[2], cc[1]);
            tris[1] = plDebugRenderer::Triangle(cc[2], cc[3], cc[1]);
            // front
            tris[2] = plDebugRenderer::Triangle(cc[4], cc[5], cc[6]);
            tris[3] = plDebugRenderer::Triangle(cc[6], cc[5], cc[7]);
            // top
            tris[4] = plDebugRenderer::Triangle(cc[4], cc[0], cc[5]);
            tris[5] = plDebugRenderer::Triangle(cc[0], cc[1], cc[5]);
            // bottom
            tris[6] = plDebugRenderer::Triangle(cc[6], cc[7], cc[2]);
            tris[7] = plDebugRenderer::Triangle(cc[2], cc[7], cc[3]);
            // left
            tris[8] = plDebugRenderer::Triangle(cc[4], cc[6], cc[0]);
            tris[9] = plDebugRenderer::Triangle(cc[0], cc[6], cc[2]);
            // right
            tris[10] = plDebugRenderer::Triangle(cc[5], cc[1], cc[7]);
            tris[11] = plDebugRenderer::Triangle(cc[1], cc[3], cc[7]);

            plDebugRenderer::DrawSolidTriangles(view.GetHandle(), tris, plColor(r, g, 0.0f, 0.1f));

            plDebugRenderer::Line lines[12];
            lines[0] = plDebugRenderer::Line(cc[4], cc[5]);
            lines[1] = plDebugRenderer::Line(cc[5], cc[7]);
            lines[2] = plDebugRenderer::Line(cc[7], cc[6]);
            lines[3] = plDebugRenderer::Line(cc[6], cc[4]);

            lines[4] = plDebugRenderer::Line(cc[0], cc[1]);
            lines[5] = plDebugRenderer::Line(cc[1], cc[3]);
            lines[6] = plDebugRenderer::Line(cc[3], cc[2]);
            lines[7] = plDebugRenderer::Line(cc[2], cc[0]);

            lines[8] = plDebugRenderer::Line(cc[4], cc[0]);
            lines[9] = plDebugRenderer::Line(cc[5], cc[1]);
            lines[10] = plDebugRenderer::Line(cc[7], cc[3]);
            lines[11] = plDebugRenderer::Line(cc[6], cc[2]);

            plDebugRenderer::DrawLines(view.GetHandle(), lines, plColor(r, g, 0.0f));
          }
        }
      }

      {
        plVec3 leftWidth = pCamera->GetDirRight() * fZf * fTanLeft;
        plVec3 rightWidth = pCamera->GetDirRight() * fZf * fTanRight;
        plVec3 bottomHeight = pCamera->GetDirUp() * fZf * fTanBottom;
        plVec3 topHeight = pCamera->GetDirUp() * fZf * fTanTop;

        plVec3 depthFar = pCamera->GetPosition() + pCamera->GetDirForwards() * fZf;
        plVec3 p0 = depthFar + rightWidth + topHeight;
        plVec3 p1 = depthFar + rightWidth + bottomHeight;
        plVec3 p2 = depthFar + leftWidth + bottomHeight;
        plVec3 p3 = depthFar + leftWidth + topHeight;

        plDebugRenderer::Line lines[4];
        lines[0] = plDebugRenderer::Line(p0, p1);
        lines[1] = plDebugRenderer::Line(p1, p2);
        lines[2] = plDebugRenderer::Line(p2, p3);
        lines[3] = plDebugRenderer::Line(p3, p0);

        plDebugRenderer::DrawLines(view.GetHandle(), lines, lineColor);
      }
    }
  }
} // namespace
#endif

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plClusteredDataCPU, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plClusteredDataCPU::plClusteredDataCPU() = default;
plClusteredDataCPU::~plClusteredDataCPU() = default;

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plClusteredDataExtractor, 1, plRTTIDefaultAllocator<plClusteredDataExtractor>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plClusteredDataExtractor::plClusteredDataExtractor(const char* szName)
  : plExtractor(szName)
{
  m_DependsOn.PushBack(plMakeHashedString("plVisibleObjectsExtractor"));

  m_TempLightsClusters.SetCountUninitialized(NUM_CLUSTERS);
  m_TempDecalsClusters.SetCountUninitialized(NUM_CLUSTERS);
  m_TempReflectionProbeClusters.SetCountUninitialized(NUM_CLUSTERS);
  m_ClusterBoundingSpheres.SetCountUninitialized(NUM_CLUSTERS);
}

plClusteredDataExtractor::~plClusteredDataExtractor() = default;

void plClusteredDataExtractor::PostSortAndBatch(
  const plView& view, const plDynamicArray<const plGameObject*>& visibleObjects, plExtractedRenderData& ref_extractedRenderData)
{
  PL_PROFILE_SCOPE("PostSortAndBatch");

  const plCamera* pCamera = view.GetCullingCamera();
  const float fAspectRatio = view.GetViewport().width / view.GetViewport().height;

  FillClusterBoundingSpheres(*pCamera, fAspectRatio, m_ClusterBoundingSpheres);
  plClusteredDataCPU* pData = PL_NEW(plFrameAllocator::GetCurrentAllocator(), plClusteredDataCPU);
  pData->m_ClusterData = PL_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plPerClusterData, NUM_CLUSTERS);

  plMat4 tmp = pCamera->GetViewMatrix();
  plSimdMat4f viewMatrix = plSimdConversion::ToMat4(tmp);

  pCamera->GetProjectionMatrix(fAspectRatio, tmp);
  plSimdMat4f projectionMatrix = plSimdConversion::ToMat4(tmp);

  plSimdMat4f viewProjectionMatrix = projectionMatrix * viewMatrix;

  // Lights
  {
    PL_PROFILE_SCOPE("Lights");
    m_TempLightData.Clear();
    plMemoryUtils::ZeroFill(m_TempLightsClusters.GetData(), NUM_CLUSTERS);

    auto batchList = ref_extractedRenderData.GetRenderDataBatchesWithCategory(plDefaultRenderDataCategories::Light);
    const plUInt32 uiBatchCount = batchList.GetBatchCount();
    for (plUInt32 i = 0; i < uiBatchCount; ++i)
    {
      const plRenderDataBatch& batch = batchList.GetBatch(i);

      for (auto it = batch.GetIterator<plRenderData>(); it.IsValid(); ++it)
      {
        const plUInt32 uiLightIndex = m_TempLightData.GetCount();

        if (uiLightIndex == plClusteredDataCPU::MAX_LIGHT_DATA)
        {
          plLog::Warning("Maximum number of lights reached ({0}). Further lights will be discarded.", plClusteredDataCPU::MAX_LIGHT_DATA);
          break;
        }

        if (auto pPointLightRenderData = plDynamicCast<const plPointLightRenderData*>(it))
        {
          FillPointLightData(m_TempLightData.ExpandAndGetRef(), pPointLightRenderData);

          plSimdBSphere pointLightSphere =
            plSimdBSphere(plSimdConversion::ToVec3(pPointLightRenderData->m_GlobalTransform.m_vPosition), pPointLightRenderData->m_fRange);
          RasterizeSphere(
            pointLightSphere, uiLightIndex, viewMatrix, projectionMatrix, m_TempLightsClusters.GetData(), m_ClusterBoundingSpheres.GetData());

          if (false)
          {
            plSimdBBox ssb = GetScreenSpaceBounds(pointLightSphere, viewMatrix, projectionMatrix);
            float minX = ((float)ssb.m_Min.x() * 0.5f + 0.5f) * view.GetViewport().width;
            float maxX = ((float)ssb.m_Max.x() * 0.5f + 0.5f) * view.GetViewport().width;
            float minY = ((float)ssb.m_Max.y() * -0.5f + 0.5f) * view.GetViewport().height;
            float maxY = ((float)ssb.m_Min.y() * -0.5f + 0.5f) * view.GetViewport().height;

            plRectFloat rect(minX, minY, maxX - minX, maxY - minY);
            plDebugRenderer::Draw2DRectangle(view.GetHandle(), rect, 0.0f, plColor::Blue.WithAlpha(0.3f));
          }
        }
        else if (auto pSpotLightRenderData = plDynamicCast<const plSpotLightRenderData*>(it))
        {
          FillSpotLightData(m_TempLightData.ExpandAndGetRef(), pSpotLightRenderData);

          plAngle halfAngle = pSpotLightRenderData->m_OuterSpotAngle / 2.0f;

          BoundingCone cone;
          cone.m_PositionAndRange = plSimdConversion::ToVec3(pSpotLightRenderData->m_GlobalTransform.m_vPosition);
          cone.m_PositionAndRange.SetW(pSpotLightRenderData->m_fRange);
          cone.m_ForwardDir = plSimdConversion::ToVec3(pSpotLightRenderData->m_GlobalTransform.m_qRotation * plVec3(1.0f, 0.0f, 0.0f));
          cone.m_SinCosAngle = plSimdVec4f(plMath::Sin(halfAngle), plMath::Cos(halfAngle), 0.0f);
          RasterizeSpotLight(cone, uiLightIndex, viewMatrix, projectionMatrix, m_TempLightsClusters.GetData(), m_ClusterBoundingSpheres.GetData());
        }
        else if (auto pDirLightRenderData = plDynamicCast<const plDirectionalLightRenderData*>(it))
        {
          FillDirLightData(m_TempLightData.ExpandAndGetRef(), pDirLightRenderData);

          RasterizeDirLight(pDirLightRenderData, uiLightIndex, m_TempLightsClusters.GetArrayPtr());
        }
        else if (auto pFogRenderData = plDynamicCast<const plFogRenderData*>(it))
        {
          float fogBaseHeight = pFogRenderData->m_GlobalTransform.m_vPosition.z;
          float fogHeightFalloff = pFogRenderData->m_fHeightFalloff > 0.0f ? plMath::Ln(0.0001f) / pFogRenderData->m_fHeightFalloff : 0.0f;

          float fogAtCameraPos = fogHeightFalloff * (pCamera->GetPosition().z - fogBaseHeight);
          if (fogAtCameraPos >= 80.0f) // Prevent infs
          {
            fogHeightFalloff = 0.0f;
          }

          pData->m_fFogHeight = -fogHeightFalloff * fogBaseHeight;
          pData->m_fFogHeightFalloff = fogHeightFalloff;
          pData->m_fFogDensityAtCameraPos = plMath::Exp(plMath::Clamp(fogAtCameraPos, -80.0f, 80.0f)); // Prevent infs
          pData->m_fFogDensity = pFogRenderData->m_fDensity;
          pData->m_fFogInvSkyDistance = pFogRenderData->m_fInvSkyDistance;

          pData->m_FogColor = pFogRenderData->m_Color;
        }
        else
        {
          plLog::Warning("Unhandled render data type '{}' in 'Light' category", it->GetDynamicRTTI()->GetTypeName());
        }
      }
    }

    pData->m_LightData = PL_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plPerLightData, m_TempLightData.GetCount());
    pData->m_LightData.CopyFrom(m_TempLightData);

    pData->m_uiSkyIrradianceIndex = view.GetWorld()->GetIndex();
    pData->m_cameraUsageHint = view.GetCameraUsageHint();
  }

  // Decals
  {
    PL_PROFILE_SCOPE("Decals");
    m_TempDecalData.Clear();
    plMemoryUtils::ZeroFill(m_TempDecalsClusters.GetData(), NUM_CLUSTERS);

    auto batchList = ref_extractedRenderData.GetRenderDataBatchesWithCategory(plDefaultRenderDataCategories::Decal);
    const plUInt32 uiBatchCount = batchList.GetBatchCount();
    for (plUInt32 i = 0; i < uiBatchCount; ++i)
    {
      const plRenderDataBatch& batch = batchList.GetBatch(i);

      for (auto it = batch.GetIterator<plRenderData>(); it.IsValid(); ++it)
      {
        const plUInt32 uiDecalIndex = m_TempDecalData.GetCount();

        if (uiDecalIndex == plClusteredDataCPU::MAX_DECAL_DATA)
        {
          plLog::Warning("Maximum number of decals reached ({0}). Further decals will be discarded.", plClusteredDataCPU::MAX_DECAL_DATA);
          break;
        }

        if (auto pDecalRenderData = plDynamicCast<const plDecalRenderData*>(it))
        {
          FillDecalData(m_TempDecalData.ExpandAndGetRef(), pDecalRenderData);

          RasterizeBox(pDecalRenderData->m_GlobalTransform, uiDecalIndex, viewProjectionMatrix, m_TempDecalsClusters.GetData(), m_ClusterBoundingSpheres.GetData());
        }
        else
        {
          plLog::Warning("Unhandled render data type '{}' in 'Decal' category", it->GetDynamicRTTI()->GetTypeName());
        }
      }
    }

    pData->m_DecalData = PL_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plPerDecalData, m_TempDecalData.GetCount());
    pData->m_DecalData.CopyFrom(m_TempDecalData);
  }

  // Reflection Probes
  {
    PL_PROFILE_SCOPE("Probes");
    m_TempReflectionProbeData.Clear();
    plMemoryUtils::ZeroFill(m_TempReflectionProbeClusters.GetData(), NUM_CLUSTERS);

    auto batchList = ref_extractedRenderData.GetRenderDataBatchesWithCategory(plDefaultRenderDataCategories::ReflectionProbe);
    const plUInt32 uiBatchCount = batchList.GetBatchCount();
    for (plUInt32 i = 0; i < uiBatchCount; ++i)
    {
      const plRenderDataBatch& batch = batchList.GetBatch(i);

      for (auto it = batch.GetIterator<plRenderData>(); it.IsValid(); ++it)
      {
        const plUInt32 uiProbeIndex = m_TempReflectionProbeData.GetCount();

        if (uiProbeIndex == plClusteredDataCPU::MAX_REFLECTION_PROBE_DATA)
        {
          plLog::Warning("Maximum number of reflection probes reached ({0}). Further reflection probes will be discarded.", plClusteredDataCPU::MAX_REFLECTION_PROBE_DATA);
          break;
        }

        if (auto pReflectionProbeRenderData = plDynamicCast<const plReflectionProbeRenderData*>(it))
        {
          auto& probeData = m_TempReflectionProbeData.ExpandAndGetRef();
          FillReflectionProbeData(probeData, pReflectionProbeRenderData);

          const plVec3 vFullScale = pReflectionProbeRenderData->m_vHalfExtents.CompMul(pReflectionProbeRenderData->m_GlobalTransform.m_vScale);

          bool bRasterizeSphere = false;
          float fMaxRadius = 0.0f;
          if (pReflectionProbeRenderData->m_uiIndex & REFLECTION_PROBE_IS_SPHERE)
          {
            constexpr float fSphereConstant = (4.0f / 3.0f) * plMath::Pi<float>();
            fMaxRadius = plMath::Max(plMath::Max(plMath::Abs(vFullScale.x), plMath::Abs(vFullScale.y)), plMath::Abs(vFullScale.z));
            const float fSphereVolume = fSphereConstant * plMath::Pow(fMaxRadius, 3.0f);
            const float fBoxVolume = plMath::Abs(vFullScale.x * vFullScale.y * vFullScale.z * 8);
            if (fSphereVolume < fBoxVolume)
            {
              bRasterizeSphere = true;
            }
          }

          if (bRasterizeSphere)
          {
            plSimdBSphere pointLightSphere =
              plSimdBSphere(plSimdConversion::ToVec3(pReflectionProbeRenderData->m_GlobalTransform.m_vPosition), fMaxRadius);
            RasterizeSphere(
              pointLightSphere, uiProbeIndex, viewMatrix, projectionMatrix, m_TempReflectionProbeClusters.GetData(), m_ClusterBoundingSpheres.GetData());
          }
          else
          {
            plTransform transform = pReflectionProbeRenderData->m_GlobalTransform;
            transform.m_vScale = vFullScale.CompMul(probeData.InfluenceScale.GetAsVec3());
            transform.m_vPosition += transform.m_qRotation * vFullScale.CompMul(probeData.InfluenceShift.GetAsVec3());

            //const plBoundingBox aabb(plVec3(-1.0f), plVec3(1.0f));
            //plDebugRenderer::DrawLineBox(view.GetHandle(), aabb, plColor::DarkBlue, transform);

            RasterizeBox(transform, uiProbeIndex, viewProjectionMatrix, m_TempReflectionProbeClusters.GetData(), m_ClusterBoundingSpheres.GetData());
          }
        }
        else
        {
          plLog::Warning("Unhandled render data type '{}' in 'ReflectionProbe' category", it->GetDynamicRTTI()->GetTypeName());
        }
      }
    }

    pData->m_ReflectionProbeData = PL_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plPerReflectionProbeData, m_TempReflectionProbeData.GetCount());
    pData->m_ReflectionProbeData.CopyFrom(m_TempReflectionProbeData);
  }

  FillItemListAndClusterData(pData);

  ref_extractedRenderData.AddFrameData(pData);

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  VisualizeClusteredData(view, pData, m_ClusterBoundingSpheres);
#endif
}

plResult plClusteredDataExtractor::Serialize(plStreamWriter& inout_stream) const
{
  PL_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return PL_SUCCESS;
}


plResult plClusteredDataExtractor::Deserialize(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PL_IGNORE_UNUSED(uiVersion);
  return PL_SUCCESS;
}

namespace
{
  plUInt32 PackIndex(plUInt32 uiLightIndex, plUInt32 uiDecalIndex) { return uiDecalIndex << 10 | uiLightIndex; }

  plUInt32 PackReflectionProbeIndex(plUInt32 uiData, plUInt32 uiReflectionProbeIndex) { return uiReflectionProbeIndex << 20 | uiData; }
} // namespace

void plClusteredDataExtractor::FillItemListAndClusterData(plClusteredDataCPU* pData)
{
  PL_PROFILE_SCOPE("FillItemListAndClusterData");
  m_TempClusterItemList.Clear();

  const plUInt32 uiNumLights = m_TempLightData.GetCount();
  const plUInt32 uiMaxLightBlockIndex = (uiNumLights + 31) / 32;

  const plUInt32 uiNumDecals = m_TempDecalData.GetCount();
  const plUInt32 uiMaxDecalBlockIndex = (uiNumDecals + 31) / 32;

  const plUInt32 uiNumReflectionProbes = m_TempReflectionProbeData.GetCount();
  const plUInt32 uiMaxReflectionProbeBlockIndex = (uiNumReflectionProbes + 31) / 32;

  const plUInt32 uiWorstCase = plMath::Max(uiNumLights, uiNumDecals, uiNumReflectionProbes);
  for (plUInt32 i = 0; i < NUM_CLUSTERS; ++i)
  {
    const plUInt32 uiOffset = m_TempClusterItemList.GetCount();
    plUInt32 uiLightCount = 0;

    // We expand m_TempClusterItemList by the worst case this loop can produce and then cut it down again to the actual size once we have filled the data. This makes sure we do not waste time on boundary checks or potential out of line calls like PushBack or PushBackUnchecked.
    m_TempClusterItemList.SetCountUninitialized(uiOffset + uiWorstCase);
    plUInt32* pTempClusterItemListRange = m_TempClusterItemList.GetData() + uiOffset;

    // Lights
    {
      auto& tempCluster = m_TempLightsClusters[i];
      for (plUInt32 uiBlockIndex = 0; uiBlockIndex < uiMaxLightBlockIndex; ++uiBlockIndex)
      {
        plUInt32 mask = tempCluster.m_BitMask[uiBlockIndex];

        while (mask > 0)
        {
          plUInt32 uiLightIndex = plMath::FirstBitLow(mask);
          mask &= ~(1 << uiLightIndex);

          uiLightIndex += uiBlockIndex * 32;
          pTempClusterItemListRange[uiLightCount] = uiLightIndex;
          ++uiLightCount;
        }
      }
    }

    plUInt32 uiDecalCount = 0;

    // Decals
    {
      auto& tempCluster = m_TempDecalsClusters[i];
      for (plUInt32 uiBlockIndex = 0; uiBlockIndex < uiMaxDecalBlockIndex; ++uiBlockIndex)
      {
        plUInt32 mask = tempCluster.m_BitMask[uiBlockIndex];

        while (mask > 0)
        {
          plUInt32 uiDecalIndex = plMath::FirstBitLow(mask);
          mask &= ~(1 << uiDecalIndex);

          uiDecalIndex += uiBlockIndex * 32;

          if (uiDecalCount < uiLightCount)
          {
            auto& item = pTempClusterItemListRange[uiDecalCount];
            item = PackIndex(item, uiDecalIndex);
          }
          else
          {
            pTempClusterItemListRange[uiDecalCount] = PackIndex(0, uiDecalIndex);
          }

          ++uiDecalCount;
        }
      }
    }

    plUInt32 uiReflectionProbeCount = 0;
    const plUInt32 uiMaxUsed = plMath::Max(uiLightCount, uiDecalCount);
    // Reflection Probes
    {
      auto& tempCluster = m_TempReflectionProbeClusters[i];
      for (plUInt32 uiBlockIndex = 0; uiBlockIndex < uiMaxReflectionProbeBlockIndex; ++uiBlockIndex)
      {
        plUInt32 mask = tempCluster.m_BitMask[uiBlockIndex];

        while (mask > 0)
        {
          plUInt32 uiReflectionProbeIndex = plMath::FirstBitLow(mask);
          mask &= ~(1 << uiReflectionProbeIndex);

          uiReflectionProbeIndex += uiBlockIndex * 32;

          if (uiReflectionProbeCount < uiMaxUsed)
          {
            auto& item = pTempClusterItemListRange[uiReflectionProbeCount];
            item = PackReflectionProbeIndex(item, uiReflectionProbeIndex);
          }
          else
          {
            pTempClusterItemListRange[uiReflectionProbeCount] = PackReflectionProbeIndex(0, uiReflectionProbeIndex);
          }

          ++uiReflectionProbeCount;
        }
      }
    }

    // Cut down the array to the actual number of elements we have written.
    const plUInt32 uiActualCase = plMath::Max(uiLightCount, uiDecalCount, uiReflectionProbeCount);
    m_TempClusterItemList.SetCountUninitialized(uiOffset + uiActualCase);

    auto& clusterData = pData->m_ClusterData[i];
    clusterData.offset = uiOffset;
    clusterData.counts = PackReflectionProbeIndex(PackIndex(uiLightCount, uiDecalCount), uiReflectionProbeCount);
  }

  pData->m_ClusterItemList = PL_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plUInt32, m_TempClusterItemList.GetCount());
  pData->m_ClusterItemList.CopyFrom(m_TempClusterItemList);
}



PL_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ClusteredDataExtractor);
