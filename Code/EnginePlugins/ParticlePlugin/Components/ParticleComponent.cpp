#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/WorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <ParticlePlugin/Components/ParticleFinisherComponent.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

//////////////////////////////////////////////////////////////////////////

plParticleComponentManager::plParticleComponentManager(plWorld* pWorld)
  : SUPER(pWorld)
{
}

void plParticleComponentManager::Initialize()
{
  {
    auto desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plParticleComponentManager::Update, this);
    desc.m_bOnlyUpdateWhenSimulating = true;
    RegisterUpdateFunction(desc);
  }
}

void plParticleComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

void plParticleComponentManager::UpdatePfxTransformsAndBounds()
{
  for (auto it = this->m_ComponentStorage.GetIterator(); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->UpdatePfxTransform();

      // This function is called in the post-transform phase so the global bounds and transform have already been calculated at this point.
      // Therefore we need to manually update the global bounds again to ensure correct bounds for culling and rendering.
      pComponent->GetOwner()->UpdateLocalBounds();
      pComponent->GetOwner()->UpdateGlobalBounds();
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plParticleComponent, 5, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Effect", GetParticleEffectFile, SetParticleEffectFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Particle_Effect")),
    PLASMA_MEMBER_PROPERTY("SpawnAtStart", m_bSpawnAtStart)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_ENUM_MEMBER_PROPERTY("OnFinishedAction", plOnComponentFinishedAction2, m_OnFinishedAction),
    PLASMA_MEMBER_PROPERTY("MinRestartDelay", m_MinRestartDelay),
    PLASMA_MEMBER_PROPERTY("RestartDelayRange", m_RestartDelayRange),
    PLASMA_MEMBER_PROPERTY("RandomSeed", m_uiRandomSeed),
    PLASMA_ENUM_MEMBER_PROPERTY("SpawnDirection", plBasisAxis, m_SpawnDirection)->AddAttributes(new plDefaultValueAttribute((plInt32)plBasisAxis::PositiveZ)),
    PLASMA_MEMBER_PROPERTY("IgnoreOwnerRotation", m_bIgnoreOwnerRotation),
    PLASMA_MEMBER_PROPERTY("SharedInstanceName", m_sSharedInstanceName),
    PLASMA_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new plExposedParametersAttribute("Effect"), new plExposeColorAlphaAttribute),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Effects"),
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgSetPlaying, OnMsgSetPlaying),
    PLASMA_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
    PLASMA_MESSAGE_HANDLER(plMsgDeleteGameObject, OnMsgDeleteGameObject),
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(StartEffect),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(StopEffect),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(InterruptEffect),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(IsEffectActive),
  }
  PLASMA_END_FUNCTIONS;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plParticleComponent::plParticleComponent() = default;
plParticleComponent::~plParticleComponent() = default;

void plParticleComponent::OnDeactivated()
{
  m_EffectController.Invalidate();

  plRenderComponent::OnDeactivated();
}

void plParticleComponent::SerializeComponent(plWorldWriter& stream) const
{
  auto& s = stream.GetStream();

  s << m_hEffectResource;
  s << m_bSpawnAtStart;

  // Version 1
  {
    bool bAutoRestart = false;
    s << bAutoRestart;
  }

  s << m_MinRestartDelay;
  s << m_RestartDelayRange;
  s << m_RestartTime;
  s << m_uiRandomSeed;
  s << m_sSharedInstanceName;

  // Version 2
  s << m_FloatParams.GetCount();
  for (plUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
  {
    s << m_FloatParams[i].m_sName;
    s << m_FloatParams[i].m_Value;
  }
  s << m_ColorParams.GetCount();
  for (plUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
  {
    s << m_ColorParams[i].m_sName;
    s << m_ColorParams[i].m_Value;
  }

  // Version 3
  s << m_OnFinishedAction;

  // version 4
  s << m_bIgnoreOwnerRotation;

  // version 5
  s << m_SpawnDirection;

  /// \todo store effect state
}

void plParticleComponent::DeserializeComponent(plWorldReader& stream)
{
  auto& s = stream.GetStream();
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  s >> m_hEffectResource;
  s >> m_bSpawnAtStart;

  // Version 1
  {
    bool bAutoRestart = false;
    s >> bAutoRestart;
  }

  s >> m_MinRestartDelay;
  s >> m_RestartDelayRange;
  s >> m_RestartTime;
  s >> m_uiRandomSeed;
  s >> m_sSharedInstanceName;

  if (uiVersion >= 2)
  {
    plUInt32 numFloats, numColors;

    s >> numFloats;
    m_FloatParams.SetCountUninitialized(numFloats);

    for (plUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
    {
      s >> m_FloatParams[i].m_sName;
      s >> m_FloatParams[i].m_Value;
    }

    m_bFloatParamsChanged = numFloats > 0;

    s >> numColors;
    m_ColorParams.SetCountUninitialized(numColors);

    for (plUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
    {
      s >> m_ColorParams[i].m_sName;
      s >> m_ColorParams[i].m_Value;
    }

    m_bColorParamsChanged = numColors > 0;
  }

  if (uiVersion >= 3)
  {
    s >> m_OnFinishedAction;
  }

  if (uiVersion >= 4)
  {
    s >> m_bIgnoreOwnerRotation;
  }

  if (uiVersion >= 5)
  {
    s >> m_SpawnDirection;
  }
}

bool plParticleComponent::StartEffect()
{
  // stop any previous effect
  m_EffectController.Invalidate();

  if (m_hEffectResource.IsValid())
  {
    plParticleWorldModule* pModule = GetWorld()->GetOrCreateModule<plParticleWorldModule>();

    m_EffectController.Create(m_hEffectResource, pModule, m_uiRandomSeed, m_sSharedInstanceName, this, m_FloatParams, m_ColorParams);

    UpdatePfxTransform();

    m_bFloatParamsChanged = false;
    m_bColorParamsChanged = false;

    return true;
  }

  return false;
}

void plParticleComponent::StopEffect()
{
  m_EffectController.Invalidate();
}

void plParticleComponent::InterruptEffect()
{
  m_EffectController.StopImmediate();
}

bool plParticleComponent::IsEffectActive() const
{
  return m_EffectController.IsAlive();
}


void plParticleComponent::OnMsgSetPlaying(plMsgSetPlaying& msg)
{
  if (msg.m_bPlay)
  {
    StartEffect();
  }
  else
  {
    StopEffect();
  }
}

void plParticleComponent::SetParticleEffect(const plParticleEffectResourceHandle& hEffect)
{
  m_EffectController.Invalidate();

  m_hEffectResource = hEffect;

  TriggerLocalBoundsUpdate();
}


void plParticleComponent::SetParticleEffectFile(const char* szFile)
{
  plParticleEffectResourceHandle hEffect;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hEffect = plResourceManager::LoadResource<plParticleEffectResource>(szFile);
  }

  SetParticleEffect(hEffect);
}


const char* plParticleComponent::GetParticleEffectFile() const
{
  if (!m_hEffectResource.IsValid())
    return "";

  return m_hEffectResource.GetResourceID();
}


plResult plParticleComponent::GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg)
{
  if (m_EffectController.IsAlive())
  {
    plBoundingBoxSphere volume;
    volume.SetInvalid();

    m_EffectController.GetBoundingVolume(volume);

    if (volume.IsValid())
    {
      if (m_SpawnDirection != plBasisAxis::PositiveZ)
      {
        const plQuat qRot = plBasisAxis::GetBasisRotation(plBasisAxis::PositiveZ, m_SpawnDirection);
        volume.Transform(qRot.GetAsMat4());
      }

      if (m_bIgnoreOwnerRotation)
      {
        volume.Transform((-GetOwner()->GetGlobalRotation()).GetAsMat4());
      }

      bounds = volume;
      return PLASMA_SUCCESS;
    }
  }

  return PLASMA_FAILURE;
}


void plParticleComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  // do not extract particles during shadow map rendering
  if (msg.m_pView->GetCameraUsageHint() == plCameraUsageHint::Shadow)
    return;

  m_EffectController.ExtractRenderData(msg, GetPfxTransform());
}

void plParticleComponent::OnMsgDeleteGameObject(plMsgDeleteGameObject& msg)
{
  plOnComponentFinishedAction2::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
}

void plParticleComponent::Update()
{
  if (!m_EffectController.IsAlive() && m_bSpawnAtStart)
  {
    if (StartEffect())
    {
      m_bSpawnAtStart = false;

      if (m_EffectController.IsContinuousEffect())
      {
        if (m_bIfContinuousStopRightAway)
        {
          StopEffect();
        }
        else
        {
          m_bSpawnAtStart = true;
        }
      }
    }
  }

  if (!m_EffectController.IsAlive() && (m_OnFinishedAction == plOnComponentFinishedAction2::Restart))
  {
    const plTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

    if (m_RestartTime == plTime())
    {
      const plTime tDiff = plTime::Seconds(GetWorld()->GetRandomNumberGenerator().DoubleInRange(m_MinRestartDelay.GetSeconds(), m_RestartDelayRange.GetSeconds()));

      m_RestartTime = tNow + tDiff;
    }
    else if (m_RestartTime <= tNow)
    {
      m_RestartTime.SetZero();
      StartEffect();
    }
  }

  if (m_EffectController.IsAlive())
  {
    if (m_bFloatParamsChanged)
    {
      m_bFloatParamsChanged = false;

      for (plUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
      {
        const auto& e = m_FloatParams[i];
        m_EffectController.SetParameter(e.m_sName, e.m_Value);
      }
    }

    if (m_bColorParamsChanged)
    {
      m_bColorParamsChanged = false;

      for (plUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
      {
        const auto& e = m_ColorParams[i];
        m_EffectController.SetParameter(e.m_sName, e.m_Value);
      }
    }

    m_EffectController.UpdateWindSamples();
  }
  else
  {
    plOnComponentFinishedAction2::HandleFinishedAction(this, m_OnFinishedAction);
  }
}

const plRangeView<const char*, plUInt32> plParticleComponent::GetParameters() const
{
  return plRangeView<const char*, plUInt32>([this]() -> plUInt32 { return 0; }, [this]() -> plUInt32 { return m_FloatParams.GetCount() + m_ColorParams.GetCount(); }, [this](plUInt32& it) { ++it; },
    [this](const plUInt32& it) -> const char* {
      if (it < m_FloatParams.GetCount())
        return m_FloatParams[it].m_sName.GetData();
      else
        return m_ColorParams[it - m_FloatParams.GetCount()].m_sName.GetData();
    });
}

void plParticleComponent::SetParameter(const char* szKey, const plVariant& var)
{
  const plTempHashedString th(szKey);
  if (var.CanConvertTo<float>())
  {
    float value = var.ConvertTo<float>();

    for (plUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
    {
      if (m_FloatParams[i].m_sName == th)
      {
        if (m_FloatParams[i].m_Value != value)
        {
          m_bFloatParamsChanged = true;
          m_FloatParams[i].m_Value = value;
        }
        return;
      }
    }

    m_bFloatParamsChanged = true;
    auto& e = m_FloatParams.ExpandAndGetRef();
    e.m_sName.Assign(szKey);
    e.m_Value = value;

    return;
  }

  if (var.CanConvertTo<plColor>())
  {
    plColor value = var.ConvertTo<plColor>();

    for (plUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
    {
      if (m_ColorParams[i].m_sName == th)
      {
        if (m_ColorParams[i].m_Value != value)
        {
          m_bColorParamsChanged = true;
          m_ColorParams[i].m_Value = value;
        }
        return;
      }
    }

    m_bColorParamsChanged = true;
    auto& e = m_ColorParams.ExpandAndGetRef();
    e.m_sName.Assign(szKey);
    e.m_Value = value;

    return;
  }
}

void plParticleComponent::RemoveParameter(const char* szKey)
{
  const plTempHashedString th(szKey);

  for (plUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
  {
    if (m_FloatParams[i].m_sName == th)
    {
      m_FloatParams.RemoveAtAndSwap(i);
      return;
    }
  }

  for (plUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
  {
    if (m_ColorParams[i].m_sName == th)
    {
      m_ColorParams.RemoveAtAndSwap(i);
      return;
    }
  }
}

bool plParticleComponent::GetParameter(const char* szKey, plVariant& out_value) const
{
  const plTempHashedString th(szKey);

  for (const auto& e : m_FloatParams)
  {
    if (e.m_sName == th)
    {
      out_value = e.m_Value;
      return true;
    }
  }
  for (const auto& e : m_ColorParams)
  {
    if (e.m_sName == th)
    {
      out_value = e.m_Value;
      return true;
    }
  }
  return false;
}

plTransform plParticleComponent::GetPfxTransform() const
{
  plTransform transform = GetOwner()->GetGlobalTransform();

  const plQuat qRot = plBasisAxis::GetBasisRotation(plBasisAxis::PositiveZ, m_SpawnDirection);

  if (m_bIgnoreOwnerRotation)
  {
    transform.m_qRotation = qRot;
  }
  else
  {
    transform.m_qRotation = transform.m_qRotation * qRot;
  }

  return transform;
}

void plParticleComponent::UpdatePfxTransform()
{
  m_EffectController.SetTransform(GetPfxTransform(), GetOwner()->GetLinearVelocity());
}

PLASMA_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Components_ParticleComponent);
