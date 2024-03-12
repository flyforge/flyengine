#pragma once

#include <RendererCore/Decals/DecalComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>
PL_DEFINE_AS_POD_TYPE(plPerLightData);
PL_DEFINE_AS_POD_TYPE(plPerDecalData);
PL_DEFINE_AS_POD_TYPE(plPerReflectionProbeData);
PL_DEFINE_AS_POD_TYPE(plPerClusterData);

#include <Core/Graphics/Camera.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <Foundation/Utilities/GraphicsUtils.h>

namespace
{
  ///\todo Make this configurable.
  static float s_fMinLightDistance = 5.0f;
  static float s_fMaxLightDistance = 500.0f;

  static float s_fDepthSliceScale = (NUM_CLUSTERS_Z - 1) / (plMath::Log2(s_fMaxLightDistance) - plMath::Log2(s_fMinLightDistance));
  static float s_fDepthSliceBias = -s_fDepthSliceScale * plMath::Log2(s_fMinLightDistance) + 1.0f;

  PL_ALWAYS_INLINE float GetDepthFromSliceIndex(plUInt32 uiSliceIndex)
  {
    return plMath::Pow(2.0f, (uiSliceIndex - s_fDepthSliceBias + 1.0f) / s_fDepthSliceScale);
  }

  PL_ALWAYS_INLINE plUInt32 GetSliceIndexFromDepth(float fLinearDepth)
  {
    return plMath::Clamp((plInt32)(plMath::Log2(fLinearDepth) * s_fDepthSliceScale + s_fDepthSliceBias), 0, NUM_CLUSTERS_Z - 1);
  }

  PL_ALWAYS_INLINE plUInt32 GetClusterIndexFromCoord(plUInt32 x, plUInt32 y, plUInt32 z)
  {
    return z * NUM_CLUSTERS_XY + y * NUM_CLUSTERS_X + x;
  }

  // in order: tlf, trf, blf, brf, tln, trn, bln, brn
  PL_FORCE_INLINE void GetClusterCornerPoints(
    const plCamera& camera, float fZf, float fZn, float fTanLeft, float fTanRight, float fTanBottom, float fTanTop, plInt32 x, plInt32 y, plInt32 z, plVec3* out_pCorners)
  {
    const plVec3& pos = camera.GetPosition();
    const plVec3& dirForward = camera.GetDirForwards();
    const plVec3& dirRight = camera.GetDirRight();
    const plVec3& dirUp = camera.GetDirUp();

    const float fStartXf = fZf * fTanLeft;
    const float fStartYf = fZf * fTanBottom;
    const float fEndXf = fZf * fTanRight;
    const float fEndYf = fZf * fTanTop;

    float fStepXf = (fEndXf - fStartXf) / NUM_CLUSTERS_X;
    float fStepYf = (fEndYf - fStartYf) / NUM_CLUSTERS_Y;

    float fXf = fStartXf + x * fStepXf;
    float fYf = fStartYf + y * fStepYf;

    out_pCorners[0] = pos + dirForward * fZf + dirRight * fXf - dirUp * fYf;
    out_pCorners[1] = out_pCorners[0] + dirRight * fStepXf;
    out_pCorners[2] = out_pCorners[0] - dirUp * fStepYf;
    out_pCorners[3] = out_pCorners[2] + dirRight * fStepXf;

    const float fStartXn = fZn * fTanLeft;
    const float fStartYn = fZn * fTanBottom;
    const float fEndXn = fZn * fTanRight;
    const float fEndYn = fZn * fTanTop;

    float fStepXn = (fEndXn - fStartXn) / NUM_CLUSTERS_X;
    float fStepYn = (fEndYn - fStartYn) / NUM_CLUSTERS_Y;
    float fXn = fStartXn + x * fStepXn;
    float fYn = fStartYn + y * fStepYn;

    out_pCorners[4] = pos + dirForward * fZn + dirRight * fXn - dirUp * fYn;
    out_pCorners[5] = out_pCorners[4] + dirRight * fStepXn;
    out_pCorners[6] = out_pCorners[4] - dirUp * fStepYn;
    out_pCorners[7] = out_pCorners[6] + dirRight * fStepXn;
  }

  void FillClusterBoundingSpheres(const plCamera& camera, float fAspectRatio, plArrayPtr<plSimdBSphere> clusterBoundingSpheres)
  {
    PL_PROFILE_SCOPE("FillClusterBoundingSpheres");

    ///\todo proper implementation for orthographic views
    if (camera.IsOrthographic())
      return;

    plMat4 mProj;
    camera.GetProjectionMatrix(fAspectRatio, mProj);

    plSimdVec4f stepScale;
    plSimdVec4f tanLBLB;
    {
      plAngle fFovLeft;
      plAngle fFovRight;
      plAngle fFovBottom;
      plAngle fFovTop;
      plGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fFovLeft, fFovRight, fFovBottom, fFovTop);

      const float fTanLeft = plMath::Tan(fFovLeft);
      const float fTanRight = plMath::Tan(fFovRight);
      const float fTanBottom = plMath::Tan(fFovBottom);
      const float fTanTop = plMath::Tan(fFovTop);

      float fStepXf = (fTanRight - fTanLeft) / NUM_CLUSTERS_X;
      float fStepYf = (fTanTop - fTanBottom) / NUM_CLUSTERS_Y;

      stepScale = plSimdVec4f(fStepXf, fStepYf, fStepXf, fStepYf);
      tanLBLB = plSimdVec4f(fTanLeft, fTanBottom, fTanLeft, fTanBottom);
    }

    plSimdVec4f pos = plSimdConversion::ToVec3(camera.GetPosition());
    plSimdVec4f dirForward = plSimdConversion::ToVec3(camera.GetDirForwards());
    plSimdVec4f dirRight = plSimdConversion::ToVec3(camera.GetDirRight());
    plSimdVec4f dirUp = plSimdConversion::ToVec3(camera.GetDirUp());


    plSimdVec4f fZn = plSimdVec4f::MakeZero();
    plSimdVec4f cc[8];

    for (plInt32 z = 0; z < NUM_CLUSTERS_Z; z++)
    {
      plSimdVec4f fZf = plSimdVec4f(GetDepthFromSliceIndex(z));
      plSimdVec4f zff_znn = fZf.GetCombined<plSwizzle::XXXX>(fZn);
      plSimdVec4f steps = zff_znn.CompMul(stepScale);

      plSimdVec4f depthF = pos + dirForward * fZf.x();
      plSimdVec4f depthN = pos + dirForward * fZn.x();

      plSimdVec4f startLBLB = zff_znn.CompMul(tanLBLB);

      for (plInt32 y = 0; y < NUM_CLUSTERS_Y; y++)
      {
        for (plInt32 x = 0; x < NUM_CLUSTERS_X; x++)
        {
          plSimdVec4f xyxy = plSimdVec4i(x, y, x, y).ToFloat();
          plSimdVec4f xfyf = startLBLB + (xyxy).CompMul(steps);

          cc[0] = depthF + dirRight * xfyf.x() - dirUp * xfyf.y();
          cc[1] = cc[0] + dirRight * steps.x();
          cc[2] = cc[0] - dirUp * steps.y();
          cc[3] = cc[2] + dirRight * steps.x();

          cc[4] = depthN + dirRight * xfyf.z() - dirUp * xfyf.w();
          cc[5] = cc[4] + dirRight * steps.z();
          cc[6] = cc[4] - dirUp * steps.w();
          cc[7] = cc[6] + dirRight * steps.z();

          clusterBoundingSpheres[GetClusterIndexFromCoord(x, y, z)] = plSimdBSphere::MakeFromPoints(cc, 8);
        }
      }

      fZn = fZf;
    }
  }

  PL_ALWAYS_INLINE void FillLightData(plPerLightData& ref_perLightData, const plLightRenderData* pLightRenderData, plUInt8 uiType)
  {
    plMemoryUtils::ZeroFill(&ref_perLightData, 1);

    plColorLinearUB lightColor = pLightRenderData->m_LightColor;
    lightColor.a = uiType;

    ref_perLightData.colorAndType = *reinterpret_cast<plUInt32*>(&lightColor.r);
    ref_perLightData.intensity = pLightRenderData->m_fIntensity;
    ref_perLightData.specularMultiplier = pLightRenderData->m_fSpecularMultiplier;
    ref_perLightData.width = pLightRenderData->m_fWidth;
    ref_perLightData.length = pLightRenderData->m_fLength;
    ref_perLightData.shadowDataOffset = pLightRenderData->m_uiShadowDataOffset;
    ref_perLightData.upDirection = plShaderUtils::Float3ToRGB10(pLightRenderData->m_GlobalTransform.m_qRotation * plVec3(0, 1, 0));
  }

  void FillPointLightData(plPerLightData& ref_perLightData, const plPointLightRenderData* pPointLightRenderData)
  {
    FillLightData(ref_perLightData, pPointLightRenderData, LIGHT_TYPE_POINT);

    ref_perLightData.position = pPointLightRenderData->m_GlobalTransform.m_vPosition;
    ref_perLightData.invSqrAttRadius = 1.0f / (pPointLightRenderData->m_fRange * pPointLightRenderData->m_fRange);
  }

  void FillSpotLightData(plPerLightData& ref_perLightData, const plSpotLightRenderData* pSpotLightRenderData)
  {
    FillLightData(ref_perLightData, pSpotLightRenderData, LIGHT_TYPE_SPOT);

    ref_perLightData.direction = plShaderUtils::Float3ToRGB10(pSpotLightRenderData->m_GlobalTransform.m_qRotation * plVec3(-1, 0, 0));
    ref_perLightData.position = pSpotLightRenderData->m_GlobalTransform.m_vPosition;
    ref_perLightData.invSqrAttRadius = 1.0f / (pSpotLightRenderData->m_fRange * pSpotLightRenderData->m_fRange);

    const float fCosInner = plMath::Cos(pSpotLightRenderData->m_InnerSpotAngle * 0.5f);
    const float fCosOuter = plMath::Cos(pSpotLightRenderData->m_OuterSpotAngle * 0.5f);
    const float fSpotParamScale = 1.0f / plMath::Max(0.001f, (fCosInner - fCosOuter));
    const float fSpotParamOffset = -fCosOuter * fSpotParamScale;
    ref_perLightData.spotParams = plShaderUtils::Float2ToRG16F(plVec2(fSpotParamScale, fSpotParamOffset));
  }

  void FillDirLightData(plPerLightData& ref_perLightData, const plDirectionalLightRenderData* pDirLightRenderData)
  {
    FillLightData(ref_perLightData, pDirLightRenderData, LIGHT_TYPE_DIR);

    ref_perLightData.direction = plShaderUtils::Float3ToRGB10(pDirLightRenderData->m_GlobalTransform.m_qRotation * plVec3(-1, 0, 0));
  }

  void FillDecalData(plPerDecalData& ref_perDecalData, const plDecalRenderData* pDecalRenderData)
  {
    plVec3 position = pDecalRenderData->m_GlobalTransform.m_vPosition;
    plVec3 dirForwards = pDecalRenderData->m_GlobalTransform.m_qRotation * plVec3(1.0f, 0.0, 0.0f);
    plVec3 dirUp = pDecalRenderData->m_GlobalTransform.m_qRotation * plVec3(0.0f, 0.0, 1.0f);
    plVec3 scale = pDecalRenderData->m_GlobalTransform.m_vScale;

    // the CompMax prevents division by zero (thus inf, thus NaN later, then crash)
    // if negative scaling should be allowed, this would need to be changed
    scale = plVec3(1.0f).CompDiv(scale.CompMax(plVec3(0.00001f)));

    const plMat4 lookAt = plGraphicsUtils::CreateLookAtViewMatrix(position, position + dirForwards, dirUp);
    plMat4 scaleMat = plMat4::MakeScaling(plVec3(scale.y, -scale.z, scale.x));

    ref_perDecalData.worldToDecalMatrix = scaleMat * lookAt;
    ref_perDecalData.applyOnlyToId = pDecalRenderData->m_uiApplyOnlyToId;
    ref_perDecalData.decalFlags = pDecalRenderData->m_uiFlags;
    ref_perDecalData.angleFadeParams = pDecalRenderData->m_uiAngleFadeParams;
    ref_perDecalData.baseColor = *reinterpret_cast<const plUInt32*>(&pDecalRenderData->m_BaseColor.r);
    ref_perDecalData.emissiveColorRG = plShaderUtils::PackFloat16intoUint(pDecalRenderData->m_EmissiveColor.r, pDecalRenderData->m_EmissiveColor.g);
    ref_perDecalData.emissiveColorBA = plShaderUtils::PackFloat16intoUint(pDecalRenderData->m_EmissiveColor.b, pDecalRenderData->m_EmissiveColor.a);
    ref_perDecalData.baseColorAtlasScale = pDecalRenderData->m_uiBaseColorAtlasScale;
    ref_perDecalData.baseColorAtlasOffset = pDecalRenderData->m_uiBaseColorAtlasOffset;
    ref_perDecalData.normalAtlasScale = pDecalRenderData->m_uiNormalAtlasScale;
    ref_perDecalData.normalAtlasOffset = pDecalRenderData->m_uiNormalAtlasOffset;
    ref_perDecalData.ormAtlasScale = pDecalRenderData->m_uiORMAtlasScale;
    ref_perDecalData.ormAtlasOffset = pDecalRenderData->m_uiORMAtlasOffset;
  }

  void FillReflectionProbeData(plPerReflectionProbeData& ref_perReflectionProbeData, const plReflectionProbeRenderData* pReflectionProbeRenderData)
  {
    plVec3 position = pReflectionProbeRenderData->m_GlobalTransform.m_vPosition;
    plVec3 scale = pReflectionProbeRenderData->m_GlobalTransform.m_vScale.CompMul(pReflectionProbeRenderData->m_vHalfExtents);

    // We store scale separately so we easily transform into probe projection space (with scale), influence space (scale + offset) and cube map space (no scale).
    auto trans = pReflectionProbeRenderData->m_GlobalTransform;
    trans.m_vScale = plVec3(1.0f, 1.0f, 1.0f);
    auto inverse = trans.GetAsMat4().GetInverse();

    // the CompMax prevents division by zero (thus inf, thus NaN later, then crash)
    // if negative scaling should be allowed, this would need to be changed
    scale = plVec3(1.0f).CompDiv(scale.CompMax(plVec3(0.00001f)));
    ref_perReflectionProbeData.WorldToProbeProjectionMatrix = inverse;

    ref_perReflectionProbeData.ProbePosition = pReflectionProbeRenderData->m_vProbePosition.GetAsVec4(1.0f); // W isn't used.
    ref_perReflectionProbeData.Scale = scale.GetAsVec4(0.0f);                                                // W isn't used.

    ref_perReflectionProbeData.InfluenceScale = pReflectionProbeRenderData->m_vInfluenceScale.GetAsVec4(0.0f);
    ref_perReflectionProbeData.InfluenceShift = pReflectionProbeRenderData->m_vInfluenceShift.CompMul(plVec3(1.0f) - pReflectionProbeRenderData->m_vInfluenceScale).GetAsVec4(0.0f);

    ref_perReflectionProbeData.PositiveFalloff = pReflectionProbeRenderData->m_vPositiveFalloff.GetAsVec4(0.0f);
    ref_perReflectionProbeData.NegativeFalloff = pReflectionProbeRenderData->m_vNegativeFalloff.GetAsVec4(0.0f);
    ref_perReflectionProbeData.Index = pReflectionProbeRenderData->m_uiIndex;
  }


  PL_FORCE_INLINE plSimdBBox GetScreenSpaceBounds(const plSimdBSphere& sphere, const plSimdMat4f& mViewMatrix, const plSimdMat4f& mProjectionMatrix)
  {
    plSimdVec4f viewSpaceCenter = mViewMatrix.TransformPosition(sphere.GetCenter());
    plSimdFloat depth = viewSpaceCenter.z();
    plSimdFloat radius = sphere.GetRadius();

    plSimdVec4f mi;
    plSimdVec4f ma;

    if (viewSpaceCenter.GetLength<3>() > radius && depth > radius)
    {
      plSimdVec4f one = plSimdVec4f(1.0f);
      plSimdVec4f oneNegOne = plSimdVec4f(1.0f, -1.0f, 1.0f, -1.0f);

      plSimdVec4f pRadius = plSimdVec4f(radius / depth);
      plSimdVec4f pRadius2 = pRadius.CompMul(pRadius);

      plSimdVec4f xy = viewSpaceCenter / depth;
      plSimdVec4f xxyy = xy.Get<plSwizzle::XXYY>();
      plSimdVec4f nom = (pRadius2.CompMul(xxyy.CompMul(xxyy) - pRadius2 + one)).GetSqrt() - xxyy.CompMul(oneNegOne);
      plSimdVec4f denom = pRadius2 - one;

      plSimdVec4f projection = mProjectionMatrix.m_col0.GetCombined<plSwizzle::XXYY>(mProjectionMatrix.m_col1);
      plSimdVec4f minXmaxX_minYmaxY = nom.CompDiv(denom).CompMul(oneNegOne).CompMul(projection);

      mi = minXmaxX_minYmaxY.Get<plSwizzle::XZXX>();
      ma = minXmaxX_minYmaxY.Get<plSwizzle::YWYY>();
    }
    else
    {
      mi = plSimdVec4f(-1.0f);
      ma = plSimdVec4f(1.0f);
    }

    mi.SetZ(depth - radius);
    ma.SetZ(depth + radius);

    return plSimdBBox(mi, ma);
  }

  template <typename Cluster, typename IntersectionFunc>
  PL_FORCE_INLINE void FillCluster(const plSimdBBox& screenSpaceBounds, plUInt32 uiBlockIndex, plUInt32 uiMask, Cluster* pClusters, IntersectionFunc func)
  {
    plSimdVec4f scale = plSimdVec4f(0.5f * NUM_CLUSTERS_X, -0.5f * NUM_CLUSTERS_Y, 1.0f, 1.0f);
    plSimdVec4f bias = plSimdVec4f(0.5f * NUM_CLUSTERS_X, 0.5f * NUM_CLUSTERS_Y, 0.0f, 0.0f);

    plSimdVec4f mi = plSimdVec4f::MulAdd(screenSpaceBounds.m_Min, scale, bias);
    plSimdVec4f ma = plSimdVec4f::MulAdd(screenSpaceBounds.m_Max, scale, bias);

    plSimdVec4i minXY_maxXY = plSimdVec4i::Truncate(mi.GetCombined<plSwizzle::XYXY>(ma));

    plSimdVec4i maxClusterIndex = plSimdVec4i(NUM_CLUSTERS_X, NUM_CLUSTERS_Y, NUM_CLUSTERS_X, NUM_CLUSTERS_Y);
    minXY_maxXY = minXY_maxXY.CompMin(maxClusterIndex - plSimdVec4i(1));
    minXY_maxXY = minXY_maxXY.CompMax(plSimdVec4i::MakeZero());

    plUInt32 xMin = minXY_maxXY.x();
    plUInt32 yMin = minXY_maxXY.w();

    plUInt32 xMax = minXY_maxXY.z();
    plUInt32 yMax = minXY_maxXY.y();

    plUInt32 zMin = GetSliceIndexFromDepth(screenSpaceBounds.m_Min.z());
    plUInt32 zMax = GetSliceIndexFromDepth(screenSpaceBounds.m_Max.z());

    for (plUInt32 z = zMin; z <= zMax; ++z)
    {
      for (plUInt32 y = yMin; y <= yMax; ++y)
      {
        for (plUInt32 x = xMin; x <= xMax; ++x)
        {
          plUInt32 uiClusterIndex = GetClusterIndexFromCoord(x, y, z);
          if (func(uiClusterIndex))
          {
            pClusters[uiClusterIndex].m_BitMask[uiBlockIndex] |= uiMask;
          }
        }
      }
    }
  }

  template <typename Cluster>
  void RasterizeSphere(const plSimdBSphere& pointLightSphere, plUInt32 uiLightIndex, const plSimdMat4f& mViewMatrix,
    const plSimdMat4f& mProjectionMatrix, Cluster* pClusters, plSimdBSphere* pClusterBoundingSpheres)
  {
    plSimdBBox screenSpaceBounds = GetScreenSpaceBounds(pointLightSphere, mViewMatrix, mProjectionMatrix);

    const plUInt32 uiBlockIndex = uiLightIndex / 32;
    const plUInt32 uiMask = 1 << (uiLightIndex - uiBlockIndex * 32);

    FillCluster(screenSpaceBounds, uiBlockIndex, uiMask, pClusters,
      [&](plUInt32 uiClusterIndex) { return pointLightSphere.Overlaps(pClusterBoundingSpheres[uiClusterIndex]); });
  }

  struct BoundingCone
  {
    plSimdBSphere m_BoundingSphere;
    plSimdVec4f m_PositionAndRange;
    plSimdVec4f m_ForwardDir;
    plSimdVec4f m_SinCosAngle;
  };

  template <typename Cluster>
  void RasterizeSpotLight(const BoundingCone& spotLightCone, plUInt32 uiLightIndex, const plSimdMat4f& mViewMatrix,
    const plSimdMat4f& mProjectionMatrix, Cluster* pClusters, plSimdBSphere* pClusterBoundingSpheres)
  {
    plSimdVec4f position = spotLightCone.m_PositionAndRange;
    plSimdFloat range = spotLightCone.m_PositionAndRange.w();
    plSimdVec4f forwardDir = spotLightCone.m_ForwardDir;
    plSimdFloat sinAngle = spotLightCone.m_SinCosAngle.x();
    plSimdFloat cosAngle = spotLightCone.m_SinCosAngle.y();

    // First calculate a bounding sphere around the cone to get min and max bounds
    plSimdVec4f bSphereCenter;
    plSimdFloat bSphereRadius;
    if (sinAngle > 0.707107f) // sin(45)
    {
      bSphereCenter = position + forwardDir * cosAngle * range;
      bSphereRadius = sinAngle * range;
    }
    else
    {
      bSphereRadius = range / (cosAngle + cosAngle);
      bSphereCenter = position + forwardDir * bSphereRadius;
    }

    plSimdBSphere spotLightSphere(bSphereCenter, bSphereRadius);
    plSimdBBox screenSpaceBounds = GetScreenSpaceBounds(spotLightSphere, mViewMatrix, mProjectionMatrix);

    const plUInt32 uiBlockIndex = uiLightIndex / 32;
    const plUInt32 uiMask = 1 << (uiLightIndex - uiBlockIndex * 32);

    FillCluster(screenSpaceBounds, uiBlockIndex, uiMask, pClusters, [&](plUInt32 uiClusterIndex) {
      plSimdBSphere clusterSphere = pClusterBoundingSpheres[uiClusterIndex];
      plSimdFloat clusterRadius = clusterSphere.GetRadius();

      plSimdVec4f toConePos = clusterSphere.m_CenterAndRadius - position;
      plSimdFloat projected = forwardDir.Dot<3>(toConePos);
      plSimdFloat distToConeSq = toConePos.Dot<3>(toConePos);
      plSimdFloat distClosestP = cosAngle * (distToConeSq - projected * projected).GetSqrt() - projected * sinAngle;

      bool angleCull = distClosestP > clusterRadius;
      bool frontCull = projected > clusterRadius + range;
      bool backCull = projected < -clusterRadius;

      return !(angleCull || frontCull || backCull); });
  }

  template <typename Cluster>
  void RasterizeDirLight(const plDirectionalLightRenderData* pDirLightRenderData, plUInt32 uiLightIndex, plArrayPtr<Cluster> clusters)
  {
    const plUInt32 uiBlockIndex = uiLightIndex / 32;
    const plUInt32 uiMask = 1 << (uiLightIndex - uiBlockIndex * 32);

    for (plUInt32 i = 0; i < clusters.GetCount(); ++i)
    {
      clusters[i].m_BitMask[uiBlockIndex] |= uiMask;
    }
  }

  template <typename Cluster>
  void RasterizeBox(const plTransform& transform, plUInt32 uiDecalIndex, const plSimdMat4f& mViewProjectionMatrix, Cluster* pClusters,
    plSimdBSphere* pClusterBoundingSpheres)
  {
    plSimdMat4f decalToWorld = plSimdConversion::ToTransform(transform).GetAsMat4();
    plSimdMat4f worldToDecal = decalToWorld.GetInverse();

    plVec3 corners[8];
    plBoundingBox::MakeFromMinMax(plVec3(-1), plVec3(1)).GetCorners(corners);

    plSimdMat4f decalToScreen = mViewProjectionMatrix * decalToWorld;
    plSimdBBox screenSpaceBounds = plSimdBBox::MakeInvalid();
    bool bInsideBox = false;
    for (plUInt32 i = 0; i < 8; ++i)
    {
      plSimdVec4f corner = plSimdConversion::ToVec3(corners[i]);
      plSimdVec4f screenSpaceCorner = decalToScreen.TransformPosition(corner);
      plSimdFloat depth = screenSpaceCorner.w();
      bInsideBox |= depth < plSimdFloat::MakeZero();

      screenSpaceCorner /= depth;
      screenSpaceCorner = screenSpaceCorner.GetCombined<plSwizzle::XYZW>(plSimdVec4f(depth));

      screenSpaceBounds.m_Min = screenSpaceBounds.m_Min.CompMin(screenSpaceCorner);
      screenSpaceBounds.m_Max = screenSpaceBounds.m_Max.CompMax(screenSpaceCorner);
    }

    if (bInsideBox)
    {
      screenSpaceBounds.m_Min = plSimdVec4f(-1.0f).GetCombined<plSwizzle::XYZW>(screenSpaceBounds.m_Min);
      screenSpaceBounds.m_Max = plSimdVec4f(1.0f).GetCombined<plSwizzle::XYZW>(screenSpaceBounds.m_Max);
    }

    plSimdVec4f decalHalfExtents = plSimdVec4f(1.0f);
    plSimdBBox localDecalBounds = plSimdBBox(-decalHalfExtents, decalHalfExtents);

    const plUInt32 uiBlockIndex = uiDecalIndex / 32;
    const plUInt32 uiMask = 1 << (uiDecalIndex - uiBlockIndex * 32);

    FillCluster(screenSpaceBounds, uiBlockIndex, uiMask, pClusters, [&](plUInt32 uiClusterIndex) {
      plSimdBSphere clusterSphere = pClusterBoundingSpheres[uiClusterIndex];
      clusterSphere.Transform(worldToDecal);

      return localDecalBounds.Overlaps(clusterSphere); });
  }
} // namespace
