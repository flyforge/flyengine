#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <ProcGenPlugin/Tasks/PlacementData.h>
#include <ProcGenPlugin/Tasks/PlacementTask.h>
#include <ProcGenPlugin/Tasks/Utils.h>

using namespace plProcGenInternal;

PLASMA_CHECK_AT_COMPILETIME(sizeof(PlacementPoint) == 32);
PLASMA_CHECK_AT_COMPILETIME(sizeof(PlacementTransform) == 64);

PlacementTask::PlacementTask(PlacementData* pData, const char* szName)
  : m_pData(pData)
{
  ConfigureTask(szName, plTaskNesting::Maybe);

  m_VM.RegisterFunction(plProcGenExpressionFunctions::s_ApplyVolumesFunc);
  m_VM.RegisterFunction(plProcGenExpressionFunctions::s_GetInstanceSeedFunc);
}

PlacementTask::~PlacementTask() = default;

void PlacementTask::Clear()
{
  m_InputPoints.Clear();
  m_OutputTransforms.Clear();
  m_Density.Clear();
  m_ValidPoints.Clear();
}

void PlacementTask::Execute()
{
  FindPlacementPoints();

  if (!m_InputPoints.IsEmpty())
  {
    ExecuteVM();
  }
}

void PlacementTask::FindPlacementPoints()
{
  PLASMA_PROFILE_SCOPE("FindPlacementPoints");

  auto pOutput = m_pData->m_pOutput;

  plSimdVec4u seed = plSimdVec4u(m_pData->m_uiTileSeed) + plSimdVec4u(0, 3, 7, 11);

  float fZRange = m_pData->m_TileBoundingBox.GetExtents().z;
  plSimdFloat fZStart = m_pData->m_TileBoundingBox.m_vMax.z;
  plSimdVec4f vXY = plSimdConversion::ToVec3(m_pData->m_TileBoundingBox.m_vMin);
  plSimdVec4f vMinOffset = plSimdConversion::ToVec3(pOutput->m_vMinOffset);
  plSimdVec4f vMaxOffset = plSimdConversion::ToVec3(pOutput->m_vMaxOffset);

  // use center for fixed plane placement
  vXY.SetZ(m_pData->m_TileBoundingBox.GetCenter().z);

  plVec3 rayDir = plVec3(0, 0, -1);
  plUInt32 uiCollisionLayer = pOutput->m_uiCollisionLayer;

  auto& patternPoints = pOutput->m_pPattern->m_Points;

  for (plUInt32 i = 0; i < patternPoints.GetCount(); ++i)
  {
    auto& patternPoint = patternPoints[i];
    plSimdVec4f patternCoords = plSimdVec4f(patternPoint.x, patternPoint.y, 0.0f);

    plPhysicsCastResult hitResult;

    if (m_pData->m_pPhysicsModule != nullptr && m_pData->m_pOutput->m_Mode == plProcPlacementMode::Raycast)
    {
      plSimdVec4f rayStart = (vXY + patternCoords * pOutput->m_fFootprint);
      rayStart += plSimdRandom::FloatMinMax(plSimdVec4i(i), vMinOffset, vMaxOffset, seed);
      rayStart.SetZ(fZStart);

      if (!m_pData->m_pPhysicsModule->Raycast(hitResult, plSimdConversion::ToVec3(rayStart), rayDir, fZRange, plPhysicsQueryParameters(uiCollisionLayer, plPhysicsShapeType::Static)))
        continue;

      if (pOutput->m_hSurface.IsValid())
      {
        if (!hitResult.m_hSurface.IsValid())
          continue;

        plResourceLock<plSurfaceResource> hitSurface(hitResult.m_hSurface, plResourceAcquireMode::BlockTillLoaded_NeverFail);
        if (hitSurface.GetAcquireResult() == plResourceAcquireResult::MissingFallback)
          continue;

        if (!hitSurface->IsBasedOn(pOutput->m_hSurface))
          continue;
      }
    }
    else if (m_pData->m_pOutput->m_Mode == plProcPlacementMode::Fixed)
    {
      plSimdVec4f rayStart = (vXY + patternCoords * pOutput->m_fFootprint);
      rayStart += plSimdRandom::FloatMinMax(plSimdVec4i(i), vMinOffset, vMaxOffset, seed);

      hitResult.m_vPosition = plSimdConversion::ToVec3(rayStart);
      hitResult.m_fDistance = 0;
      hitResult.m_vNormal.Set(0, 0, 1);
    }

    bool bInBoundingBox = false;
    plSimdVec4f hitPosition = plSimdConversion::ToVec3(hitResult.m_vPosition);
    plSimdVec4f allOne = plSimdVec4f(1.0f);
    for (auto& globalToLocalBox : m_pData->m_GlobalToLocalBoxTransforms)
    {
      plSimdVec4f localHitPosition = globalToLocalBox.TransformPosition(hitPosition).Abs();
      if ((localHitPosition <= allOne).AllSet<3>())
      {
        bInBoundingBox = true;
        break;
      }
    }

    if (bInBoundingBox)
    {
      PlacementPoint& placementPoint = m_InputPoints.ExpandAndGetRef();
      placementPoint.m_vPosition = hitResult.m_vPosition;
      placementPoint.m_fScale = 1.0f;
      placementPoint.m_vNormal = hitResult.m_vNormal;
      placementPoint.m_uiColorIndex = 0;
      placementPoint.m_uiObjectIndex = 0;
      placementPoint.m_uiPointIndex = static_cast<plUInt16>(i);
    }
  }
}

void PlacementTask::ExecuteVM()
{
  auto pOutput = m_pData->m_pOutput;

  // Execute bytecode
  if (pOutput->m_pByteCode != nullptr)
  {
    PLASMA_PROFILE_SCOPE("ExecuteVM");

    plUInt32 uiNumInstances = m_InputPoints.GetCount();
    m_Density.SetCountUninitialized(uiNumInstances);

    plHybridArray<plProcessingStream, 8> inputs;
    {
      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sPositionX, offsetof(PlacementPoint, m_vPosition.x)));
      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sPositionY, offsetof(PlacementPoint, m_vPosition.y)));
      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sPositionZ, offsetof(PlacementPoint, m_vPosition.z)));

      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sNormalX, offsetof(PlacementPoint, m_vNormal.x)));
      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sNormalY, offsetof(PlacementPoint, m_vNormal.y)));
      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sNormalZ, offsetof(PlacementPoint, m_vNormal.z)));

      inputs.PushBack(MakeInputStream(ExpressionInputs::s_sPointIndex, offsetof(PlacementPoint, m_uiPointIndex), plProcessingStream::DataType::Short));
    }

    plHybridArray<plProcessingStream, 8> outputs;
    {
      outputs.PushBack(plProcessingStream(ExpressionOutputs::s_sOutDensity, m_Density.GetByteArrayPtr(), plProcessingStream::DataType::Float));
      outputs.PushBack(MakeOutputStream(ExpressionOutputs::s_sOutScale, offsetof(PlacementPoint, m_fScale)));
      outputs.PushBack(MakeOutputStream(ExpressionOutputs::s_sOutColorIndex, offsetof(PlacementPoint, m_uiColorIndex), plProcessingStream::DataType::Byte));
      outputs.PushBack(MakeOutputStream(ExpressionOutputs::s_sOutObjectIndex, offsetof(PlacementPoint, m_uiObjectIndex), plProcessingStream::DataType::Byte));
    }

    // Execute expression bytecode
    if (m_VM.Execute(*(pOutput->m_pByteCode), inputs, outputs, uiNumInstances, m_pData->m_GlobalData).Failed())
    {
      return;
    }

    // Test density against point threshold and fill remaining input point data from expression
    float fObjectCount = static_cast<float>(pOutput->m_ObjectsToPlace.GetCount());
    const Pattern* pPattern = pOutput->m_pPattern;
    for (plUInt32 i = 0; i < uiNumInstances; ++i)
    {
      auto& inputPoint = m_InputPoints[i];
      const plUInt32 uiPointIndex = inputPoint.m_uiPointIndex;
      const float fThreshold = pPattern->m_Points[uiPointIndex].threshold;

      if (m_Density[i] >= fThreshold)
      {
        m_ValidPoints.PushBack(i);
      }
    }
  }

  if (m_ValidPoints.IsEmpty())
  {
    return;
  }

  PLASMA_PROFILE_SCOPE("Construct final transforms");

  m_OutputTransforms.SetCountUninitialized(m_ValidPoints.GetCount());

  plSimdVec4u seed = plSimdVec4u(m_pData->m_uiTileSeed) + plSimdVec4u(13, 17, 31, 79);

  float fMinAngle = 0.0f;
  float fMaxAngle = plMath::Pi<float>() * 2.0f;

  plSimdVec4f vMinValue = plSimdVec4f(fMinAngle, pOutput->m_vMinOffset.z, 0.0f);
  plSimdVec4f vMaxValue = plSimdVec4f(fMaxAngle, pOutput->m_vMaxOffset.z, 0.0f);
  plSimdVec4f vYawRotationSnap = plSimdVec4f(pOutput->m_YawRotationSnap);
  plSimdVec4f vUp = plSimdVec4f(0, 0, 1);
  plSimdVec4f vHalf = plSimdVec4f(0.5f);
  plSimdVec4f vAlignToNormal = plSimdVec4f(pOutput->m_fAlignToNormal);
  plSimdVec4f vMinScale = plSimdConversion::ToVec3(pOutput->m_vMinScale);
  plSimdVec4f vMaxScale = plSimdConversion::ToVec3(pOutput->m_vMaxScale);

  const plColorGradient* pColorGradient = nullptr;
  if (pOutput->m_hColorGradient.IsValid())
  {
    plResourceLock<plColorGradientResource> pColorGradientResource(pOutput->m_hColorGradient, plResourceAcquireMode::BlockTillLoaded);
    pColorGradient = &(pColorGradientResource->GetDescriptor().m_Gradient);
  }

  for (plUInt32 i = 0; i < m_ValidPoints.GetCount(); ++i)
  {
    plUInt32 uiInputPointIndex = m_ValidPoints[i];
    auto& placementPoint = m_InputPoints[uiInputPointIndex];
    auto& placementTransform = m_OutputTransforms[i];

    plSimdVec4f random = plSimdRandom::FloatMinMax(plSimdVec4i(placementPoint.m_uiPointIndex), vMinValue, vMaxValue, seed);

    plSimdVec4f offset = plSimdVec4f::MakeZero();
    offset.SetZ(random.y());
    placementTransform.m_Transform.m_Position = plSimdConversion::ToVec3(placementPoint.m_vPosition) + offset;

    plSimdVec4f yaw = plSimdVec4f(random.x());
    plSimdVec4f roundedYaw = (yaw.CompDiv(vYawRotationSnap) + vHalf).Floor().CompMul(vYawRotationSnap);
    yaw = plSimdVec4f::Select(vYawRotationSnap == plSimdVec4f::MakeZero(), yaw, roundedYaw);

    plSimdQuat qYawRot = plSimdQuat::MakeFromAxisAndAngle(vUp, yaw.x());
    plSimdVec4f vNormal = plSimdConversion::ToVec3(placementPoint.m_vNormal);
    plSimdQuat qToNormalRot = plSimdQuat::MakeShortestRotation(vUp, plSimdVec4f::Lerp(vUp, vNormal, vAlignToNormal));
    placementTransform.m_Transform.m_Rotation = qToNormalRot * qYawRot;

    plSimdVec4f scale = plSimdVec4f(plMath::Clamp(placementPoint.m_fScale, 0.0f, 1.0f));
    placementTransform.m_Transform.m_Scale = plSimdVec4f::Lerp(vMinScale, vMaxScale, scale);

    placementTransform.m_ObjectColor = plColor::MakeZero();
    placementTransform.m_uiPointIndex = placementPoint.m_uiPointIndex;
    placementTransform.m_uiObjectIndex = placementPoint.m_uiObjectIndex;
    placementTransform.m_bHasValidColor = false;

    if (pColorGradient != nullptr)
    {
      float colorIndex = plMath::ColorByteToFloat(placementPoint.m_uiColorIndex);

      plColor objectColor;
      plUInt8 alpha;
      float intensity = 1.0f;
      pColorGradient->EvaluateColor(colorIndex, objectColor);
      pColorGradient->EvaluateIntensity(colorIndex, intensity);
      pColorGradient->EvaluateAlpha(colorIndex, alpha);
      objectColor.r *= intensity;
      objectColor.g *= intensity;
      objectColor.b *= intensity;
      objectColor.a = alpha;

      placementTransform.m_ObjectColor = objectColor;
      placementTransform.m_bHasValidColor = true;
    }
  }
}