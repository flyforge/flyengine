#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Trail/ParticleTypeTrail.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleTypeTrailFactory, 1, plRTTIDefaultAllocator<plParticleTypeTrailFactory>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ENUM_MEMBER_PROPERTY("RenderMode", plParticleTypeRenderMode, m_RenderMode),
    PL_MEMBER_PROPERTY("Texture", m_sTexture)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_2D"), new plDefaultValueAttribute(plStringView("{ e00262e8-58f5-42f5-880d-569257047201 }"))),// wrap in plStringView to prevent a memory leak report
    PL_MEMBER_PROPERTY("Segments", m_uiMaxPoints)->AddAttributes(new plDefaultValueAttribute(6), new plClampValueAttribute(3, 64)),
    PL_ENUM_MEMBER_PROPERTY("TextureAtlas", plParticleTextureAtlasType, m_TextureAtlasType),
    PL_MEMBER_PROPERTY("NumSpritesX", m_uiNumSpritesX)->AddAttributes(new plDefaultValueAttribute(1), new plClampValueAttribute(1, 16)),
    PL_MEMBER_PROPERTY("NumSpritesY", m_uiNumSpritesY)->AddAttributes(new plDefaultValueAttribute(1), new plClampValueAttribute(1, 16)),
    PL_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
    PL_MEMBER_PROPERTY("DistortionTexture", m_sDistortionTexture)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    PL_MEMBER_PROPERTY("DistortionStrength", m_fDistortionStrength)->AddAttributes(new plDefaultValueAttribute(100.0f), new plClampValueAttribute(0.0f, 500.0f)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleTypeTrail, 1, plRTTIDefaultAllocator<plParticleTypeTrail>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const plRTTI* plParticleTypeTrailFactory::GetTypeType() const
{
  return plGetStaticRTTI<plParticleTypeTrail>();
}

void plParticleTypeTrailFactory::CopyTypeProperties(plParticleType* pObject, bool bFirstTime) const
{
  plParticleTypeTrail* pType = static_cast<plParticleTypeTrail*>(pObject);

  pType->m_RenderMode = m_RenderMode;
  pType->m_uiMaxPoints = m_uiMaxPoints;
  pType->m_hTexture.Invalidate();
  pType->m_TextureAtlasType = m_TextureAtlasType;
  pType->m_uiNumSpritesX = m_uiNumSpritesX;
  pType->m_uiNumSpritesY = m_uiNumSpritesY;
  pType->m_sTintColorParameter = plTempHashedString(m_sTintColorParameter.GetData());
  pType->m_hDistortionTexture.Invalidate();
  pType->m_fDistortionStrength = m_fDistortionStrength;

  // fixed 25 FPS for the update rate
  pType->m_UpdateDiff = plTime::MakeFromSeconds(1.0 / 25.0); // m_UpdateDiff;

  if (!m_sTexture.IsEmpty())
    pType->m_hTexture = plResourceManager::LoadResource<plTexture2DResource>(m_sTexture);
  if (!m_sDistortionTexture.IsEmpty())
    pType->m_hDistortionTexture = plResourceManager::LoadResource<plTexture2DResource>(m_sDistortionTexture);

  if (bFirstTime)
  {
    pType->GetOwnerSystem()->AddParticleDeathEventHandler(plMakeDelegate(&plParticleTypeTrail::OnParticleDeath, pType));

    pType->m_LastSnapshot = pType->GetOwnerEffect()->GetTotalEffectLifeTime();
  }

  // m_uiMaxPoints = plMath::Min<plUInt16>(8, m_uiMaxPoints);

  // clamp the number of points to the maximum possible count
  pType->m_uiMaxPoints = plMath::Min<plUInt16>(pType->m_uiMaxPoints, pType->ComputeTrailPointBucketSize(pType->m_uiMaxPoints));

  pType->m_uiCurFirstIndex = 1;
}

enum class TypeTrailVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added render mode
  Version_3, // added texture atlas support
  Version_4, // added tint color
  Version_5, // added distortion mode

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void plParticleTypeTrailFactory::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = (int)TypeTrailVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_sTexture;
  inout_stream << m_uiMaxPoints;
  inout_stream << m_UpdateDiff;
  inout_stream << m_RenderMode;

  // version 3
  inout_stream << m_TextureAtlasType;
  inout_stream << m_uiNumSpritesX;
  inout_stream << m_uiNumSpritesY;

  // version 4
  inout_stream << m_sTintColorParameter;

  // version 5
  inout_stream << m_sDistortionTexture;
  inout_stream << m_fDistortionStrength;
}

void plParticleTypeTrailFactory::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  PL_ASSERT_DEV(uiVersion <= (int)TypeTrailVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_sTexture;
  inout_stream >> m_uiMaxPoints;
  inout_stream >> m_UpdateDiff;

  if (uiVersion >= 2)
  {
    inout_stream >> m_RenderMode;
  }

  if (uiVersion >= 3)
  {
    inout_stream >> m_TextureAtlasType;
    inout_stream >> m_uiNumSpritesX;
    inout_stream >> m_uiNumSpritesY;

    if (m_TextureAtlasType == plParticleTextureAtlasType::None)
    {
      m_uiNumSpritesX = 1;
      m_uiNumSpritesY = 1;
    }
  }

  if (uiVersion >= 4)
  {
    inout_stream >> m_sTintColorParameter;
  }

  if (uiVersion >= 5)
  {
    inout_stream >> m_sDistortionTexture;
    inout_stream >> m_fDistortionStrength;
  }
}

//////////////////////////////////////////////////////////////////////////

plParticleTypeTrail::plParticleTypeTrail() = default;

plParticleTypeTrail::~plParticleTypeTrail()
{
  if (m_pStreamPosition != nullptr)
  {
    GetOwnerSystem()->RemoveParticleDeathEventHandler(plMakeDelegate(&plParticleTypeTrail::OnParticleDeath, this));
  }
}

void plParticleTypeTrail::CreateRequiredStreams()
{
  CreateStream("LifeTime", plProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  CreateStream("Position", plProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Size", plProcessingStream::DataType::Half, &m_pStreamSize, false);
  CreateStream("Color", plProcessingStream::DataType::Half4, &m_pStreamColor, false);
  CreateStream("TrailData", plProcessingStream::DataType::Short2, &m_pStreamTrailData, true);

  m_pStreamVariation = nullptr;

  if (m_TextureAtlasType == plParticleTextureAtlasType::RandomVariations || m_TextureAtlasType == plParticleTextureAtlasType::RandomYAnimatedX)
  {
    CreateStream("Variation", plProcessingStream::DataType::Int, &m_pStreamVariation, false);
  }
}

void plParticleTypeTrail::ExtractTypeRenderData(plMsgExtractRenderData& ref_msg, const plTransform& instanceTransform) const
{
  PL_PROFILE_SCOPE("PFX: Trail");

  if (!m_hTexture.IsValid())
    return;

  const plUInt32 numActiveParticles = (plUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numActiveParticles == 0)
    return;

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if (m_uiLastExtractedFrame != plRenderWorld::GetFrameCounter())
  {
    m_uiLastExtractedFrame = plRenderWorld::GetFrameCounter();

    const plColor tintColor = GetOwnerEffect()->GetColorParameter(m_sTintColorParameter, plColor::White);

    const plFloat16* pSize = m_pStreamSize->GetData<plFloat16>();
    const plColorLinear16f* pColor = m_pStreamColor->GetData<plColorLinear16f>();
    const TrailData* pTrailData = m_pStreamTrailData->GetData<TrailData>();
    const plFloat16Vec2* pLifeTime = m_pStreamLifeTime->GetData<plFloat16Vec2>();
    const plUInt32* pVariation = m_pStreamVariation ? m_pStreamVariation->GetData<plUInt32>() : nullptr;

    const plUInt32 uiBucketSize = ComputeTrailPointBucketSize(m_uiMaxPoints);

    // this will automatically be deallocated at the end of the frame
    m_BaseParticleData =
      PL_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plBaseParticleShaderData, (plUInt32)GetOwnerSystem()->GetNumActiveParticles());
    m_TrailPointsShared =
      PL_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plVec4, (plUInt32)GetOwnerSystem()->GetNumActiveParticles() * uiBucketSize);
    m_TrailParticleData =
      PL_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plTrailParticleShaderData, (plUInt32)GetOwnerSystem()->GetNumActiveParticles());

    for (plUInt32 p = 0; p < numActiveParticles; ++p)
    {
      m_BaseParticleData[p].Size = pSize[p];
      m_BaseParticleData[p].Color = pColor[p].ToLinearFloat() * tintColor;
      m_BaseParticleData[p].Life = pLifeTime[p].x * pLifeTime[p].y;
      m_BaseParticleData[p].Variation = (pVariation != nullptr) ? pVariation[p] : 0;

      m_TrailParticleData[p].NumPoints = pTrailData[p].m_uiNumPoints;
    }

    for (plUInt32 p = 0; p < numActiveParticles; ++p)
    {
      const plVec4* pTrailPositions = GetTrailPointsPositions(pTrailData[p].m_uiIndexForTrailPoints);

      plVec4* pRenderPositions = &m_TrailPointsShared[p * uiBucketSize];

      /// \todo This loop could be done without a condition
      for (plUInt32 i = 0; i < m_uiMaxPoints; ++i)
      {
        if (i > m_uiCurFirstIndex)
        {
          pRenderPositions[i] = pTrailPositions[m_uiCurFirstIndex + m_uiMaxPoints - i];
        }
        else
        {
          pRenderPositions[i] = pTrailPositions[m_uiCurFirstIndex - i];
        }
      }
    }
  }

  auto pRenderData = plCreateRenderDataForThisFrame<plParticleTrailRenderData>(nullptr);

  pRenderData->m_uiBatchId = plHashingUtils::StringHashTo32(m_hTexture.GetResourceIDHash()) + m_uiMaxPoints;
  pRenderData->m_uiSortingKey = ComputeSortingKey(m_RenderMode, pRenderData->m_uiBatchId);

  pRenderData->m_bApplyObjectTransform = GetOwnerEffect()->NeedsToApplyTransform();
  pRenderData->m_TotalEffectLifeTime = GetOwnerEffect()->GetTotalEffectLifeTime();
  pRenderData->m_RenderMode = m_RenderMode;
  pRenderData->m_GlobalTransform = instanceTransform;
  pRenderData->m_uiMaxTrailPoints = m_uiMaxPoints;
  pRenderData->m_hTexture = m_hTexture;
  pRenderData->m_BaseParticleData = m_BaseParticleData;
  pRenderData->m_TrailParticleData = m_TrailParticleData;
  pRenderData->m_TrailPointsShared = m_TrailPointsShared;
  pRenderData->m_fSnapshotFraction = m_fSnapshotFraction;
  pRenderData->m_hDistortionTexture = m_hDistortionTexture;
  pRenderData->m_fDistortionStrength = m_fDistortionStrength;

  pRenderData->m_uiNumVariationsX = 1;
  pRenderData->m_uiNumVariationsY = 1;
  pRenderData->m_uiNumFlipbookAnimationsX = 1;
  pRenderData->m_uiNumFlipbookAnimationsY = 1;

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

  ref_msg.AddRenderData(pRenderData, plDefaultRenderDataCategories::LitTransparent, plRenderData::Caching::Never);
}

void plParticleTypeTrail::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  TrailData* pTrailData = m_pStreamTrailData->GetWritableData<TrailData>() + uiStartIndex;

  const plVec4* pPosData = m_pStreamPosition->GetData<plVec4>() + uiStartIndex;

  const plUInt32 uiPrevIndex = (m_uiCurFirstIndex > 0) ? (m_uiCurFirstIndex - 1) : (m_uiMaxPoints - 1);
  const plUInt32 uiPrevIndex2 = (uiPrevIndex > 0) ? (uiPrevIndex - 1) : (m_uiMaxPoints - 1);

  for (plUInt64 i = 0; i < uiNumElements; ++i)
  {
    const plVec4 vStartPos = pPosData[i];

    TrailData& td = pTrailData[i];
    td.m_uiNumPoints = 2;
    td.m_uiIndexForTrailPoints = GetIndexForTrailPoints();

    plVec4* pPos = GetTrailPointsPositions(td.m_uiIndexForTrailPoints);
    pPos[m_uiCurFirstIndex] = vStartPos;
    pPos[uiPrevIndex] = vStartPos;
    pPos[uiPrevIndex2] = vStartPos;
  }
}


void plParticleTypeTrail::Process(plUInt64 uiNumElements)
{
  const plTime tNow = GetOwnerEffect()->GetTotalEffectLifeTime();

  TrailData* pTrailData = m_pStreamTrailData->GetWritableData<TrailData>();
  const plVec4* pPosData = m_pStreamPosition->GetData<plVec4>();

  if (tNow - m_LastSnapshot >= m_UpdateDiff)
  {
    m_LastSnapshot = tNow;

    m_uiCurFirstIndex = (m_uiCurFirstIndex + 1) == m_uiMaxPoints ? 0 : (m_uiCurFirstIndex + 1);

    for (plUInt64 i = 0; i < uiNumElements; ++i)
    {
      pTrailData[i].m_uiNumPoints = plMath::Min<plUInt16>(pTrailData[i].m_uiNumPoints + 1, m_uiMaxPoints);
    }
  }

  m_fSnapshotFraction = 1.0f - (float)((tNow - m_LastSnapshot).GetSeconds() / m_UpdateDiff.GetSeconds());

  for (plUInt64 i = 0; i < uiNumElements; ++i)
  {
    plVec4* pPositions = GetTrailPointsPositions(pTrailData[i].m_uiIndexForTrailPoints);
    pPositions[m_uiCurFirstIndex] = pPosData[i];
  }
}

plUInt16 plParticleTypeTrail::GetIndexForTrailPoints()
{
  plUInt16 res = 0;

  if (!m_FreeTrailData.IsEmpty())
  {
    res = m_FreeTrailData.PeekBack();
    m_FreeTrailData.PopBack();
  }
  else
  {
    // expand the proper array

    // if (m_uiMaxPoints > 32)
    //{
    res = static_cast<plUInt16>(m_TrailPoints64.GetCount());
    m_TrailPoints64.ExpandAndGetRef();
    //}
    // else if (m_uiMaxPoints > 16)
    //{
    //  res = m_TrailData32.GetCount() & 0xFFFF;
    //  m_TrailData64.ExpandAndGetRef();
    //}
    // else if (m_uiMaxPoints > 8)
    //{
    //  res = m_TrailData16.GetCount() & 0xFFFF;
    //  m_TrailData64.ExpandAndGetRef();
    //}
    // else
    //{
    //  res = m_TrailData8.GetCount() & 0xFFFF;
    //  m_TrailData8.ExpandAndGetRef();
    //}
  }

  return res;
}

plVec4* plParticleTypeTrail::GetTrailPointsPositions(plUInt32 index)
{
  // if (m_uiMaxPoints > 32)
  {
    return &m_TrailPoints64[index].Positions[0];
  }
  // else if (m_uiMaxPoints > 16)
  //{
  //  return &m_TrailPoints32[index].Positions[0];
  //}
  // else if (m_uiMaxPoints > 8)
  //{
  //  return &m_TrailPoints16[index].Positions[0];
  //}
  // else
  //{
  //  return &m_TrailPoints8[index].Positions[0];
  //}
}

const plVec4* plParticleTypeTrail::GetTrailPointsPositions(plUInt32 index) const
{
  // if (m_uiMaxPoints > 32)
  {
    return &m_TrailPoints64[index].Positions[0];
  }
  // else if (m_uiMaxPoints > 16)
  //{
  //  return &m_TrailPoints32[index].Positions[0];
  //}
  // else if (m_uiMaxPoints > 8)
  //{
  //  return &m_TrailPoints16[index].Positions[0];
  //}
  // else
  //{
  //  return &m_TrailPoints8[index].Positions[0];
  //}
}


plUInt16 plParticleTypeTrail::ComputeTrailPointBucketSize(plUInt16 uiMaxTrailPoints)
{
  if (uiMaxTrailPoints > 32)
  {
    return 64;
  }
  else if (uiMaxTrailPoints > 16)
  {
    return 32;
  }
  else if (uiMaxTrailPoints > 8)
  {
    return 16;
  }
  else
  {
    return 8;
  }
}

void plParticleTypeTrail::OnParticleDeath(const plStreamGroupElementRemovedEvent& e)
{
  const TrailData* pTrailData = m_pStreamTrailData->GetData<TrailData>();

  // return the trail data to the list of free elements
  m_FreeTrailData.PushBack(pTrailData[e.m_uiElementIndex].m_uiIndexForTrailPoints);
}

PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Trail_ParticleTypeTrail);
