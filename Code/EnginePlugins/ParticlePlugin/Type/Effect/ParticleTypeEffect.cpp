#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/Type/Effect/ParticleTypeEffect.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleTypeEffectFactory, 1, plRTTIDefaultAllocator<plParticleTypeEffectFactory>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Effect", m_sEffect)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Particle_Effect")),
    // PLASMA_MEMBER_PROPERTY("Shared Instance Name", m_sSharedInstanceName), // there is currently no way (I can think of) to uniquely identify each sub-system for the 'shared owner'
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleTypeEffect, 1, plRTTIDefaultAllocator<plParticleTypeEffect>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleTypeEffectFactory::plParticleTypeEffectFactory() = default;
plParticleTypeEffectFactory::~plParticleTypeEffectFactory() = default;

const plRTTI* plParticleTypeEffectFactory::GetTypeType() const
{
  return plGetStaticRTTI<plParticleTypeEffect>();
}

void plParticleTypeEffectFactory::CopyTypeProperties(plParticleType* pObject, bool bFirstTime) const
{
  plParticleTypeEffect* pType = static_cast<plParticleTypeEffect*>(pObject);

  pType->m_hEffect.Invalidate();

  if (!m_sEffect.IsEmpty())
    pType->m_hEffect = plResourceManager::LoadResource<plParticleEffectResource>(m_sEffect);

  // pType->m_sSharedInstanceName = m_sSharedInstanceName;


  if (bFirstTime)
  {
    pType->GetOwnerSystem()->AddParticleDeathEventHandler(plMakeDelegate(&plParticleTypeEffect::OnParticleDeath, pType));
  }
}

enum class TypeEffectVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void plParticleTypeEffectFactory::Save(plStreamWriter& stream) const
{
  const plUInt8 uiVersion = (int)TypeEffectVersion::Version_Current;
  stream << uiVersion;

  plUInt64 m_uiRandomSeed = 0;

  stream << m_sEffect;
  stream << m_uiRandomSeed;
  stream << m_sSharedInstanceName;
}

void plParticleTypeEffectFactory::Load(plStreamReader& stream)
{
  plUInt8 uiVersion = 0;
  stream >> uiVersion;

  PLASMA_ASSERT_DEV(uiVersion <= (int)TypeEffectVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_sEffect;

  if (uiVersion >= 2)
  {
    plUInt64 m_uiRandomSeed = 0;

    stream >> m_uiRandomSeed;
    stream >> m_sSharedInstanceName;
  }
}


plParticleTypeEffect::plParticleTypeEffect() = default;

plParticleTypeEffect::~plParticleTypeEffect()
{
  if (m_pStreamPosition != nullptr)
  {
    GetOwnerSystem()->RemoveParticleDeathEventHandler(plMakeDelegate(&plParticleTypeEffect::OnParticleDeath, this));

    ClearEffects(true);
  }
}

void plParticleTypeEffect::CreateRequiredStreams()
{
  CreateStream("Position", plProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("EffectID", plProcessingStream::DataType::Int, &m_pStreamEffectID, false);
}

void plParticleTypeEffect::ExtractTypeRenderData(plMsgExtractRenderData& msg, const plTransform& instanceTransform) const
{
  PLASMA_PROFILE_SCOPE("PFX: Effect");

  const plUInt32 numParticles = (plUInt32)GetOwnerSystem()->GetNumActiveParticles();

  if (numParticles == 0)
    return;

  const plUInt32* pEffectID = m_pStreamEffectID->GetData<plUInt32>();

  const plParticleWorldModule* pWorldModule = GetOwnerEffect()->GetOwnerWorldModule();

  for (plUInt32 i = 0; i < numParticles; ++i)
  {
    plParticleEffectHandle hInstance = plParticleEffectHandle(plParticleEffectId(pEffectID[i]));

    const plParticleEffectInstance* pEffect = nullptr;
    if (pWorldModule->TryGetEffectInstance(hInstance, pEffect))
    {
      pWorldModule->ExtractEffectRenderData(pEffect, msg, pEffect->GetTransform());
    }
  }
}

void plParticleTypeEffect::OnReset()
{
  ClearEffects(true);
}

void plParticleTypeEffect::Process(plUInt64 uiNumElements)
{
  PLASMA_PROFILE_SCOPE("PFX: Effect");

  if (!m_hEffect.IsValid())
    return;

  const plVec4* pPosition = m_pStreamPosition->GetData<plVec4>();
  plUInt32* pEffectID = m_pStreamEffectID->GetWritableData<plUInt32>();

  plParticleWorldModule* pWorldModule = GetOwnerEffect()->GetOwnerWorldModule();

  m_fMaxEffectRadius = 0.0f;

  const plUInt64 uiRandomSeed = GetOwnerEffect()->GetRandomSeed();

  for (plUInt32 i = 0; i < uiNumElements; ++i)
  {
    if (pEffectID[i] == 0) // always an invalid ID
    {
      const void* pDummy = nullptr;
      plParticleEffectHandle hInstance = pWorldModule->CreateEffectInstance(m_hEffect, uiRandomSeed, /*m_sSharedInstanceName*/ nullptr, pDummy, plArrayPtr<plParticleEffectFloatParam>(), plArrayPtr<plParticleEffectColorParam>());

      pEffectID[i] = hInstance.GetInternalID().m_Data;
    }

    plParticleEffectHandle hInstance = plParticleEffectHandle(plParticleEffectId(pEffectID[i]));

    plParticleEffectInstance* pEffect = nullptr;
    if (pWorldModule->TryGetEffectInstance(hInstance, pEffect))
    {
      plTransform t;
      t.m_qRotation.SetIdentity();
      t.m_vScale.Set(1.0f);
      t.m_vPosition = pPosition[i].GetAsVec3();

      // TODO: pass through velocity
      pEffect->SetVisibleIf(GetOwnerEffect());
      pEffect->SetTransformForNextFrame(t, plVec3::MakeZero());

      plBoundingBoxSphere bounds;
      pEffect->GetBoundingVolume(bounds);

      m_fMaxEffectRadius = plMath::Max(m_fMaxEffectRadius, bounds.m_fSphereRadius);
    }
  }
}

void plParticleTypeEffect::OnParticleDeath(const plStreamGroupElementRemovedEvent& e)
{
  plParticleWorldModule* pWorldModule = GetOwnerEffect()->GetOwnerWorldModule();

  const plUInt32* pEffectID = m_pStreamEffectID->GetData<plUInt32>();

  plParticleEffectHandle hInstance = plParticleEffectHandle(plParticleEffectId(pEffectID[e.m_uiElementIndex]));

  pWorldModule->DestroyEffectInstance(hInstance, false, nullptr);
}

void plParticleTypeEffect::ClearEffects(bool bInterruptImmediately)
{
  // delete all effects that are still in the processing group

  plParticleWorldModule* pWorldModule = GetOwnerEffect()->GetOwnerWorldModule();
  const plUInt64 uiNumParticles = GetOwnerSystem()->GetNumActiveParticles();

  if (uiNumParticles == 0 || m_pStreamEffectID == nullptr)
    return;

  plUInt32* pEffectID = m_pStreamEffectID->GetWritableData<plUInt32>();

  for (plUInt32 elemIdx = 0; elemIdx < uiNumParticles; ++elemIdx)
  {
    plParticleEffectHandle hInstance = plParticleEffectHandle(plParticleEffectId(pEffectID[elemIdx]));
    pEffectID[elemIdx] = 0;

    pWorldModule->DestroyEffectInstance(hInstance, bInterruptImmediately, nullptr);
  }
}

PLASMA_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Effect_ParticleTypeEffect);
