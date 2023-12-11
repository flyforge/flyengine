#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Type/Quad/ParticleTypeQuad.h>

#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_LastPosition.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plQuadParticleOrientation, 2)
  PLASMA_ENUM_CONSTANTS(plQuadParticleOrientation::Billboard)
  PLASMA_ENUM_CONSTANTS(plQuadParticleOrientation::Rotating_OrthoEmitterDir, plQuadParticleOrientation::Rotating_EmitterDir)
  PLASMA_ENUM_CONSTANTS(plQuadParticleOrientation::Fixed_EmitterDir, plQuadParticleOrientation::Fixed_RandomDir, plQuadParticleOrientation::Fixed_WorldUp)
  PLASMA_ENUM_CONSTANTS(plQuadParticleOrientation::FixedAxis_EmitterDir, plQuadParticleOrientation::FixedAxis_ParticleDir)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleTypeQuadFactory, 2, plRTTIDefaultAllocator<plParticleTypeQuadFactory>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_MEMBER_PROPERTY("Orientation", plQuadParticleOrientation, m_Orientation),
    PLASMA_MEMBER_PROPERTY("Deviation", m_MaxDeviation)->AddAttributes(new plClampValueAttribute(plAngle::MakeFromDegree(0), plAngle::MakeFromDegree(90))),
    PLASMA_ENUM_MEMBER_PROPERTY("RenderMode", plParticleTypeRenderMode, m_RenderMode),
    PLASMA_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_2D"), new plDefaultValueAttribute(plStringView("{ e00262e8-58f5-42f5-880d-569257047201 }"))),// wrap in plStringView to prevent a memory leak report
    PLASMA_ENUM_MEMBER_PROPERTY("TextureAtlas", plParticleTextureAtlasType, m_TextureAtlasType),
    PLASMA_MEMBER_PROPERTY("NumSpritesX", m_uiNumSpritesX)->AddAttributes(new plDefaultValueAttribute(1), new plClampValueAttribute(1, 16)),
    PLASMA_MEMBER_PROPERTY("NumSpritesY", m_uiNumSpritesY)->AddAttributes(new plDefaultValueAttribute(1), new plClampValueAttribute(1, 16)),
    PLASMA_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
    PLASMA_MEMBER_PROPERTY("DistortionTexture", m_sDistortionTexture)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    PLASMA_MEMBER_PROPERTY("DistortionStrength", m_fDistortionStrength)->AddAttributes(new plDefaultValueAttribute(100.0f), new plClampValueAttribute(0.0f, 500.0f)),
    PLASMA_MEMBER_PROPERTY("ParticleStretch", m_fStretch)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(-100.0f, 100.0f)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleTypeQuad, 1, plRTTIDefaultAllocator<plParticleTypeQuad>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const plRTTI* plParticleTypeQuadFactory::GetTypeType() const
{
  return plGetStaticRTTI<plParticleTypeQuad>();
}

void plParticleTypeQuadFactory::CopyTypeProperties(plParticleType* pObject, bool bFirstTime) const
{
  plParticleTypeQuad* pType = static_cast<plParticleTypeQuad*>(pObject);

  pType->m_Orientation = m_Orientation;
  pType->m_MaxDeviation = m_MaxDeviation;
  pType->m_hTexture.Invalidate();
  pType->m_RenderMode = m_RenderMode;
  pType->m_uiNumSpritesX = m_uiNumSpritesX;
  pType->m_uiNumSpritesY = m_uiNumSpritesY;
  pType->m_sTintColorParameter = plTempHashedString(m_sTintColorParameter.GetData());
  pType->m_hDistortionTexture.Invalidate();
  pType->m_fDistortionStrength = m_fDistortionStrength;
  pType->m_TextureAtlasType = m_TextureAtlasType;
  pType->m_fStretch = m_fStretch;

  if (!m_sTexture.IsEmpty())
    pType->m_hTexture = plResourceManager::LoadResource<plTexture2DResource>(m_sTexture);
  if (!m_sDistortionTexture.IsEmpty())
    pType->m_hDistortionTexture = plResourceManager::LoadResource<plTexture2DResource>(m_sDistortionTexture);
}

enum class TypeQuadVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // sprite deviation
  Version_3, // distortion
  Version_4, // added texture atlas type
  Version_5, // added particle stretch

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void plParticleTypeQuadFactory::Save(plStreamWriter& stream) const
{
  const plUInt8 uiVersion = (int)TypeQuadVersion::Version_Current;
  stream << uiVersion;

  stream << m_Orientation;
  stream << m_RenderMode;
  stream << m_sTexture;
  stream << m_uiNumSpritesX;
  stream << m_uiNumSpritesY;
  stream << m_sTintColorParameter;
  stream << m_MaxDeviation;
  stream << m_sDistortionTexture;
  stream << m_fDistortionStrength;
  stream << m_TextureAtlasType;

  // Version 5
  stream << m_fStretch;
}

void plParticleTypeQuadFactory::Load(plStreamReader& stream)
{
  plUInt8 uiVersion = 0;
  stream >> uiVersion;

  PLASMA_ASSERT_DEV(uiVersion <= (int)TypeQuadVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_Orientation;
  stream >> m_RenderMode;
  stream >> m_sTexture;
  stream >> m_uiNumSpritesX;
  stream >> m_uiNumSpritesY;
  stream >> m_sTintColorParameter;

  if (uiVersion >= 2)
  {
    stream >> m_MaxDeviation;
  }

  if (uiVersion >= 3)
  {
    stream >> m_sDistortionTexture;
    stream >> m_fDistortionStrength;
  }

  if (uiVersion >= 4)
  {
    stream >> m_TextureAtlasType;

    if (m_TextureAtlasType == plParticleTextureAtlasType::None)
    {
      m_uiNumSpritesX = 1;
      m_uiNumSpritesY = 1;
    }
  }

  if (uiVersion >= 5)
  {
    stream >> m_fStretch;
  }
}

void plParticleTypeQuadFactory::QueryFinalizerDependencies(plSet<const plRTTI*>& inout_FinalizerDeps) const
{
  if (m_Orientation == plQuadParticleOrientation::FixedAxis_ParticleDir)
  {
    inout_FinalizerDeps.Insert(plGetStaticRTTI<plParticleFinalizerFactory_LastPosition>());
  }
}

plParticleTypeQuad::plParticleTypeQuad() = default;
plParticleTypeQuad::~plParticleTypeQuad() = default;

void plParticleTypeQuad::CreateRequiredStreams()
{
  CreateStream("LifeTime", plProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  CreateStream("Position", plProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Size", plProcessingStream::DataType::Half, &m_pStreamSize, false);
  CreateStream("Color", plProcessingStream::DataType::Half4, &m_pStreamColor, false);
  CreateStream("RotationSpeed", plProcessingStream::DataType::Half, &m_pStreamRotationSpeed, false);
  CreateStream("RotationOffset", plProcessingStream::DataType::Half, &m_pStreamRotationOffset, false);

  m_pStreamAxis = nullptr;
  m_pStreamVariation = nullptr;
  m_pStreamLastPosition = nullptr;

  if (m_Orientation == plQuadParticleOrientation::Fixed_RandomDir || m_Orientation == plQuadParticleOrientation::Fixed_EmitterDir || m_Orientation == plQuadParticleOrientation::Fixed_WorldUp)
  {
    CreateStream("Axis", plProcessingStream::DataType::Float3, &m_pStreamAxis, true);
  }

  if (m_TextureAtlasType == plParticleTextureAtlasType::RandomVariations || m_TextureAtlasType == plParticleTextureAtlasType::RandomYAnimatedX)
  {
    CreateStream("Variation", plProcessingStream::DataType::Int, &m_pStreamVariation, false);
  }

  if (m_Orientation == plQuadParticleOrientation::FixedAxis_ParticleDir)
  {
    CreateStream("LastPosition", plProcessingStream::DataType::Float3, &m_pStreamLastPosition, false);
  }
}

struct sodComparer
{
  // sort farther particles to the front, so that they get rendered first (back to front)
  PLASMA_ALWAYS_INLINE bool Less(const plParticleTypeQuad::sod& a, const plParticleTypeQuad::sod& b) const { return a.dist > b.dist; }
  PLASMA_ALWAYS_INLINE bool Equal(const plParticleTypeQuad::sod& a, const plParticleTypeQuad::sod& b) const { return a.dist == b.dist; }
};

void plParticleTypeQuad::ExtractTypeRenderData(plMsgExtractRenderData& msg, const plTransform& instanceTransform) const
{
  PLASMA_PROFILE_SCOPE("PFX: Quad");

  const plUInt32 numParticles = (plUInt32)GetOwnerSystem()->GetNumActiveParticles();
  if (!m_hTexture.IsValid() || numParticles == 0)
    return;

  const bool bNeedsSorting = (m_RenderMode == plParticleTypeRenderMode::Blended) || (m_RenderMode == plParticleTypeRenderMode::BlendedForeground) || (m_RenderMode == plParticleTypeRenderMode::BlendedBackground) || (m_RenderMode == plParticleTypeRenderMode::BlendAdd);

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if ((m_uiLastExtractedFrame != plRenderWorld::GetFrameCounter())
    /*&& !bNeedsSorting*/) // TODO: in theory every shared instance has to sort the Quads, in practice this maybe should be an option
  {
    m_uiLastExtractedFrame = plRenderWorld::GetFrameCounter();

    if (bNeedsSorting)
    {
      // TODO: Using the frame allocator this way results in memory corruptions.
      // Not sure, whether this is supposed to work.
      plHybridArray<sod, 64> sorted; // (plFrameAllocator::GetCurrentAllocator());
      sorted.SetCountUninitialized(numParticles);

      const plVec3 vCameraPos = msg.m_pView->GetCullingCamera()->GetCenterPosition();
      const plVec4* pPosition = m_pStreamPosition->GetData<plVec4>();

      for (plUInt32 p = 0; p < numParticles; ++p)
      {
        sorted[p].dist = (pPosition[p].GetAsVec3() - vCameraPos).GetLengthSquared();
        sorted[p].index = p;
      }

      sorted.Sort(sodComparer());

      CreateExtractedData(&sorted);
    }
    else
    {
      CreateExtractedData(nullptr);
    }
  }

  AddParticleRenderData(msg, instanceTransform);
}

PLASMA_ALWAYS_INLINE plUInt32 noRedirect(plUInt32 idx, const plHybridArray<plParticleTypeQuad::sod, 64>* pSorted)
{
  return idx;
}

PLASMA_ALWAYS_INLINE plUInt32 sortedRedirect(plUInt32 idx, const plHybridArray<plParticleTypeQuad::sod, 64>* pSorted)
{
  return (*pSorted)[idx].index;
}

void plParticleTypeQuad::CreateExtractedData(const plHybridArray<sod, 64>* pSorted) const
{
  auto redirect = (pSorted != nullptr) ? sortedRedirect : noRedirect;

  const plUInt32 numParticles = (plUInt32)GetOwnerSystem()->GetNumActiveParticles();

  const bool bNeedsBillboardData = m_Orientation == plQuadParticleOrientation::Billboard;
  const bool bNeedsTangentData = !bNeedsBillboardData;

  const plVec3 vEmitterPos = GetOwnerSystem()->GetTransform().m_vPosition;
  const plVec3 vEmitterDir = GetOwnerSystem()->GetTransform().m_qRotation * plVec3(0, 0, 1); // Z axis
  const plVec3 vEmitterDirOrtho = vEmitterDir.GetOrthogonalVector();

  const plTime tCur = GetOwnerEffect()->GetTotalEffectLifeTime();
  const plColor tintColor = GetOwnerEffect()->GetColorParameter(m_sTintColorParameter, plColor::White);

  const plFloat16Vec2* pLifeTime = m_pStreamLifeTime->GetData<plFloat16Vec2>();
  const plVec4* pPosition = m_pStreamPosition->GetData<plVec4>();
  const plFloat16* pSize = m_pStreamSize->GetData<plFloat16>();
  const plColorLinear16f* pColor = m_pStreamColor->GetData<plColorLinear16f>();
  const plFloat16* pRotationSpeed = m_pStreamRotationSpeed->GetData<plFloat16>();
  const plFloat16* pRotationOffset = m_pStreamRotationOffset->GetData<plFloat16>();
  const plVec3* pAxis = m_pStreamAxis ? m_pStreamAxis->GetData<plVec3>() : nullptr;
  const plUInt32* pVariation = m_pStreamVariation ? m_pStreamVariation->GetData<plUInt32>() : nullptr;
  const plVec3* pLastPosition = m_pStreamLastPosition ? m_pStreamLastPosition->GetData<plVec3>() : nullptr;

  // this will automatically be deallocated at the end of the frame
  m_BaseParticleData = PLASMA_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plBaseParticleShaderData, numParticles);

  AllocateParticleData(numParticles, bNeedsBillboardData, bNeedsTangentData);

  auto SetBaseData = [&](plUInt32 dstIdx, plUInt32 srcIdx) {
    m_BaseParticleData[dstIdx].Size = pSize[srcIdx];
    m_BaseParticleData[dstIdx].Color = pColor[srcIdx].ToLinearFloat() * tintColor;
    m_BaseParticleData[dstIdx].Life = pLifeTime[srcIdx].x * pLifeTime[srcIdx].y;
    m_BaseParticleData[dstIdx].Variation = (pVariation != nullptr) ? pVariation[srcIdx] : 0;
  };

  auto SetBillboardData = [&](plUInt32 dstIdx, plUInt32 srcIdx) {
    m_BillboardParticleData[dstIdx].Position = pPosition[srcIdx].GetAsVec3();
    m_BillboardParticleData[dstIdx].RotationOffset = pRotationOffset[srcIdx];
    m_BillboardParticleData[dstIdx].RotationSpeed = pRotationSpeed[srcIdx];
  };

  auto SetTangentDataEmitterDir = [&](plUInt32 dstIdx, plUInt32 srcIdx) {
    plMat3 mRotation = plMat3::MakeAxisRotation(vEmitterDir, plAngle::MakeFromRadian((float)(tCur.GetSeconds() * pRotationSpeed[srcIdx]) + pRotationOffset[srcIdx]));

    m_TangentParticleData[dstIdx].Position = pPosition[srcIdx].GetAsVec3();
    m_TangentParticleData[dstIdx].TangentX = mRotation * vEmitterDirOrtho;
    m_TangentParticleData[dstIdx].TangentZ = vEmitterDir;
  };

  auto SetTangentDataEmitterDirOrtho = [&](plUInt32 dstIdx, plUInt32 srcIdx) {
    const plVec3 vDirToParticle = (pPosition[srcIdx].GetAsVec3() - vEmitterPos);
    plVec3 vOrthoDir = vEmitterDir.CrossRH(vDirToParticle);
    vOrthoDir.NormalizeIfNotZero(plVec3(1, 0, 0)).IgnoreResult();

    plMat3 mRotation = plMat3::MakeAxisRotation(vOrthoDir, plAngle::MakeFromRadian((float)(tCur.GetSeconds() * pRotationSpeed[srcIdx]) + pRotationOffset[srcIdx]));

    m_TangentParticleData[dstIdx].Position = pPosition[srcIdx].GetAsVec3();
    m_TangentParticleData[dstIdx].TangentX = vOrthoDir;
    m_TangentParticleData[dstIdx].TangentZ = mRotation * vEmitterDir;
  };

  auto SetTangentDataFromAxis = [&](plUInt32 dstIdx, plUInt32 srcIdx) {
    plVec3 vNormal = pAxis[srcIdx];
    vNormal.Normalize();

    const plVec3 vTangentStart = vNormal.GetOrthogonalVector().GetNormalized();

    plMat3 mRotation = plMat3::MakeAxisRotation(vNormal, plAngle::MakeFromRadian((float)(tCur.GetSeconds() * pRotationSpeed[srcIdx]) + pRotationOffset[srcIdx]));

    const plVec3 vTangentX = mRotation * vTangentStart;

    m_TangentParticleData[dstIdx].Position = pPosition[srcIdx].GetAsVec3();
    m_TangentParticleData[dstIdx].TangentX = vTangentX;
    m_TangentParticleData[dstIdx].TangentZ = vTangentX.CrossRH(vNormal);
  };

  auto SetTangentDataAligned_Emitter = [&](plUInt32 dstIdx, plUInt32 srcIdx) {
    m_TangentParticleData[dstIdx].Position = pPosition[srcIdx].GetAsVec3();
    m_TangentParticleData[dstIdx].TangentX = vEmitterDir;
    m_TangentParticleData[dstIdx].TangentZ.x = m_fStretch;
  };

  auto SetTangentDataAligned_ParticleDir = [&](plUInt32 dstIdx, plUInt32 srcIdx) {
    const plVec3 vCurPos = pPosition[srcIdx].GetAsVec3();
    const plVec3 vLastPos = pLastPosition[srcIdx];
    const plVec3 vDir = vCurPos - vLastPos;
    m_TangentParticleData[dstIdx].Position = vCurPos;
    m_TangentParticleData[dstIdx].TangentX = vDir;
    m_TangentParticleData[dstIdx].TangentZ.x = m_fStretch;
  };

  for (plUInt32 p = 0; p < numParticles; ++p)
  {
    SetBaseData(p, redirect(p, pSorted));
  }

  if (bNeedsBillboardData)
  {
    for (plUInt32 p = 0; p < numParticles; ++p)
    {
      SetBillboardData(p, redirect(p, pSorted));
    }
  }

  if (bNeedsTangentData)
  {
    if (m_Orientation == plQuadParticleOrientation::Rotating_EmitterDir)
    {
      for (plUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataEmitterDir(p, redirect(p, pSorted));
      }
    }
    else if (m_Orientation == plQuadParticleOrientation::Rotating_OrthoEmitterDir)
    {
      for (plUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataEmitterDirOrtho(p, redirect(p, pSorted));
      }
    }
    else if (m_Orientation == plQuadParticleOrientation::Fixed_EmitterDir || m_Orientation == plQuadParticleOrientation::Fixed_RandomDir || m_Orientation == plQuadParticleOrientation::Fixed_WorldUp)
    {
      for (plUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataFromAxis(p, redirect(p, pSorted));
      }
    }
    else if (m_Orientation == plQuadParticleOrientation::FixedAxis_EmitterDir)
    {
      for (plUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataAligned_Emitter(p, redirect(p, pSorted));
      }
    }
    else if (m_Orientation == plQuadParticleOrientation::FixedAxis_ParticleDir)
    {
      for (plUInt32 p = 0; p < numParticles; ++p)
      {
        SetTangentDataAligned_ParticleDir(p, redirect(p, pSorted));
      }
    }
    else
    {
      PLASMA_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

void plParticleTypeQuad::AddParticleRenderData(plMsgExtractRenderData& msg, const plTransform& instanceTransform) const
{
  auto pRenderData = plCreateRenderDataForThisFrame<plParticleQuadRenderData>(nullptr);

  pRenderData->m_uiBatchId = plHashingUtils::StringHashTo32(m_hTexture.GetResourceIDHash());
  pRenderData->m_uiSortingKey = ComputeSortingKey(m_RenderMode, pRenderData->m_uiBatchId);

  pRenderData->m_bApplyObjectTransform = GetOwnerEffect()->NeedsToApplyTransform();
  pRenderData->m_GlobalTransform = instanceTransform;
  pRenderData->m_TotalEffectLifeTime = GetOwnerEffect()->GetTotalEffectLifeTime();
  pRenderData->m_RenderMode = m_RenderMode;
  pRenderData->m_hTexture = m_hTexture;
  pRenderData->m_BaseParticleData = m_BaseParticleData;
  pRenderData->m_BillboardParticleData = m_BillboardParticleData;
  pRenderData->m_TangentParticleData = m_TangentParticleData;
  pRenderData->m_uiNumVariationsX = 1;
  pRenderData->m_uiNumVariationsY = 1;
  pRenderData->m_uiNumFlipbookAnimationsX = 1;
  pRenderData->m_uiNumFlipbookAnimationsY = 1;
  pRenderData->m_hDistortionTexture = m_hDistortionTexture;
  pRenderData->m_fDistortionStrength = m_fDistortionStrength;

  switch (m_Orientation)
  {
    case plQuadParticleOrientation::Billboard:
      pRenderData->m_QuadModePermutation = "PARTICLE_QUAD_MODE_BILLBOARD";
      break;
    case plQuadParticleOrientation::Rotating_OrthoEmitterDir:
    case plQuadParticleOrientation::Rotating_EmitterDir:
    case plQuadParticleOrientation::Fixed_EmitterDir:
    case plQuadParticleOrientation::Fixed_WorldUp:
    case plQuadParticleOrientation::Fixed_RandomDir:
      pRenderData->m_QuadModePermutation = "PARTICLE_QUAD_MODE_TANGENTS";
      break;
    case plQuadParticleOrientation::FixedAxis_EmitterDir:
    case plQuadParticleOrientation::FixedAxis_ParticleDir:
      pRenderData->m_QuadModePermutation = "PARTICLE_QUAD_MODE_AXIS_ALIGNED";
      break;
  }

  switch (m_TextureAtlasType)
  {
    case plParticleTextureAtlasType::None:
      break;

    case plParticleTextureAtlasType::RandomVariations:
      pRenderData->m_uiNumVariationsX = m_uiNumSpritesX;
      pRenderData->m_uiNumVariationsY = m_uiNumSpritesY;
      break;

    case plParticleTextureAtlasType::FlipbookAnimation:
      pRenderData->m_uiNumFlipbookAnimationsX = m_uiNumSpritesX;
      pRenderData->m_uiNumFlipbookAnimationsY = m_uiNumSpritesY;
      break;

    case plParticleTextureAtlasType::RandomYAnimatedX:
      pRenderData->m_uiNumFlipbookAnimationsX = m_uiNumSpritesX;
      pRenderData->m_uiNumVariationsY = m_uiNumSpritesY;
      break;
  }

  msg.AddRenderData(pRenderData, plDefaultRenderDataCategories::LitTransparent, plRenderData::Caching::Never);
}

void plParticleTypeQuad::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  if (m_pStreamAxis != nullptr)
  {
    plVec3* pAxis = m_pStreamAxis->GetWritableData<plVec3>();
    plRandom& rng = GetRNG();

    if (m_Orientation == plQuadParticleOrientation::Fixed_RandomDir)
    {
      PLASMA_PROFILE_SCOPE("PFX: Init Quad Axis Random");

      for (plUInt32 i = 0; i < uiNumElements; ++i)
      {
        const plUInt64 uiElementIdx = uiStartIndex + i;

        pAxis[uiElementIdx] = plVec3::MakeRandomDirection(rng);
      }
    }
    else if (m_Orientation == plQuadParticleOrientation::Fixed_EmitterDir || m_Orientation == plQuadParticleOrientation::Fixed_WorldUp)
    {
      PLASMA_PROFILE_SCOPE("PFX: Init Quad Axis");

      plVec3 vNormal;

      if (m_Orientation == plQuadParticleOrientation::Fixed_EmitterDir)
      {
        vNormal = GetOwnerSystem()->GetTransform().m_qRotation * plVec3(0, 0, 1); // Z axis
      }
      else if (m_Orientation == plQuadParticleOrientation::Fixed_WorldUp)
      {
        plCoordinateSystem coord;
        GetOwnerSystem()->GetWorld()->GetCoordinateSystem(GetOwnerSystem()->GetTransform().m_vPosition, coord);

        vNormal = coord.m_vUpDir;
      }

      if (m_MaxDeviation > plAngle::MakeFromDegree(1.0f))
      {
        // how to get from the X axis to the desired normal
        plQuat qRotToDir = plQuat::MakeShortestRotation(plVec3(1, 0, 0), vNormal);

        for (plUInt32 i = 0; i < uiNumElements; ++i)
        {
          const plUInt64 uiElementIdx = uiStartIndex + i;
          const plVec3 vRandomX = plVec3::MakeRandomDeviationX(rng, m_MaxDeviation);

          pAxis[uiElementIdx] = qRotToDir * vRandomX;
        }
      }
      else
      {
        for (plUInt32 i = 0; i < uiNumElements; ++i)
        {
          const plUInt64 uiElementIdx = uiStartIndex + i;
          pAxis[uiElementIdx] = vNormal;
        }
      }
    }
  }
}

void plParticleTypeQuad::AllocateParticleData(const plUInt32 numParticles, const bool bNeedsBillboardData, const bool bNeedsTangentData) const
{
  m_BillboardParticleData = nullptr;
  if (bNeedsBillboardData)
  {
    m_BillboardParticleData = PLASMA_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plBillboardQuadParticleShaderData, numParticles);
  }

  m_TangentParticleData = nullptr;
  if (bNeedsTangentData)
  {
    m_TangentParticleData = PLASMA_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plTangentQuadParticleShaderData, (plUInt32)GetOwnerSystem()->GetNumActiveParticles());
  }
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class plQuadParticleOrientationPatch_1_2 final : public plGraphPatch
{
public:
  plQuadParticleOrientationPatch_1_2()
    : plGraphPatch("plQuadParticleOrientation", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    // TODO: this type of patch does not work

    pNode->RenameProperty("FragmentOrthogonalEmitterDirection", "Rotating_OrthoEmitterDir");
    pNode->RenameProperty("FragmentEmitterDirection", "Rotating_EmitterDir");

    pNode->RenameProperty("SpriteEmitterDirection", "Fixed_EmitterDir");
    pNode->RenameProperty("SpriteRandom", "Fixed_RandomDir");
    pNode->RenameProperty("SpriteWorldUp", "Fixed_WorldUp");

    pNode->RenameProperty("AxisAligned_Emitter", "FixedAxis_EmitterDir");
  }
};

plQuadParticleOrientationPatch_1_2 g_plQuadParticleOrientationPatch_1_2;

//////////////////////////////////////////////////////////////////////////

class plParticleTypeQuadFactory_1_2 final : public plGraphPatch
{
public:
  plParticleTypeQuadFactory_1_2()
    : plGraphPatch("plParticleTypeQuadFactory", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    plAbstractObjectNode::Property* pProp = pNode->FindProperty("Orientation");
    const plStringBuilder sOri = pProp->m_Value.Get<plString>();

    if (sOri == "plQuadParticleOrientation::FragmentOrthogonalEmitterDirection")
      pProp->m_Value = "plQuadParticleOrientation::Rotating_OrthoEmitterDir";

    if (sOri == "plQuadParticleOrientation::FragmentEmitterDirection")
      pProp->m_Value = "plQuadParticleOrientation::Rotating_EmitterDir";

    if (sOri == "plQuadParticleOrientation::SpriteEmitterDirection")
      pProp->m_Value = "plQuadParticleOrientation::Fixed_EmitterDir";

    if (sOri == "plQuadParticleOrientation::SpriteRandom")
      pProp->m_Value = "plQuadParticleOrientation::Fixed_RandomDir";

    if (sOri == "plQuadParticleOrientation::SpriteWorldUp")
      pProp->m_Value = "plQuadParticleOrientation::Fixed_WorldUp";

    if (sOri == "plQuadParticleOrientation::AxisAligned_Emitter")
      pProp->m_Value = "plQuadParticleOrientation::FixedAxis_EmitterDir";
  }
};

plParticleTypeQuadFactory_1_2 g_plParticleTypeQuadFactory_1_2;