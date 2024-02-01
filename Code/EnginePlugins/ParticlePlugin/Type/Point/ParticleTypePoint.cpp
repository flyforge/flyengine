#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Math/Color16f.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Point/ParticleTypePoint.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleTypePointFactory, 1, plRTTIDefaultAllocator<plParticleTypePointFactory>)
{
  //PL_BEGIN_ATTRIBUTES
  //{
  //  new plHiddenAttribute()
  //}
  //PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleTypePoint, 1, plRTTIDefaultAllocator<plParticleTypePoint>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const plRTTI* plParticleTypePointFactory::GetTypeType() const
{
  return plGetStaticRTTI<plParticleTypePoint>();
}

void plParticleTypePointFactory::CopyTypeProperties(plParticleType* pObject, bool bFirstTime) const
{
  // plParticleTypePoint* pType = static_cast<plParticleTypePoint*>(pObject);
}

enum class TypePointVersion
{
  Version_0 = 0,


  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void plParticleTypePointFactory::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = (int)TypePointVersion::Version_Current;
  inout_stream << uiVersion;
}

void plParticleTypePointFactory::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  PL_ASSERT_DEV(uiVersion <= (int)TypePointVersion::Version_Current, "Invalid version {0}", uiVersion);
}

void plParticleTypePoint::CreateRequiredStreams()
{
  CreateStream("Position", plProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Color", plProcessingStream::DataType::Half4, &m_pStreamColor, false);
}

void plParticleTypePoint::ExtractTypeRenderData(plMsgExtractRenderData& ref_msg, const plTransform& instanceTransform) const
{
  PL_PROFILE_SCOPE("PFX: Point");

  const plUInt32 numParticles = (plUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numParticles == 0)
    return;

  // don't copy the data multiple times in the same frame, if the effect is instanced
  if (m_uiLastExtractedFrame != plRenderWorld::GetFrameCounter())
  {
    m_uiLastExtractedFrame = plRenderWorld::GetFrameCounter();

    const plVec4* pPosition = m_pStreamPosition->GetData<plVec4>();
    const plColorLinear16f* pColor = m_pStreamColor->GetData<plColorLinear16f>();

    // this will automatically be deallocated at the end of the frame
    m_BaseParticleData = PL_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plBaseParticleShaderData, numParticles);
    m_BillboardParticleData = PL_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plBillboardQuadParticleShaderData, numParticles);

    for (plUInt32 p = 0; p < numParticles; ++p)
    {
      m_BaseParticleData[p].Color = pColor[p].ToLinearFloat();
      m_BillboardParticleData[p].Position = pPosition[p].GetAsVec3();
    }
  }

  auto pRenderData = plCreateRenderDataForThisFrame<plParticlePointRenderData>(nullptr);

  pRenderData->m_bApplyObjectTransform = GetOwnerEffect()->NeedsToApplyTransform();
  pRenderData->m_GlobalTransform = instanceTransform;
  pRenderData->m_TotalEffectLifeTime = GetOwnerEffect()->GetTotalEffectLifeTime();
  pRenderData->m_BaseParticleData = m_BaseParticleData;
  pRenderData->m_BillboardParticleData = m_BillboardParticleData;

  ref_msg.AddRenderData(pRenderData, plDefaultRenderDataCategories::LitTransparent, plRenderData::Caching::Never);
}



PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Point_ParticleTypePoint);
