#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Age.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Volume.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Point/ParticleTypePoint.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleSystemDescriptor, 2, plRTTIDefaultAllocator<plParticleSystemDescriptor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Name", m_sName),
    PL_MEMBER_PROPERTY("Visible", m_bVisible)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("LifeTime", m_LifeTime)->AddAttributes(new plDefaultValueAttribute(plTime::MakeFromSeconds(2)), new plClampValueAttribute(plTime::MakeFromSeconds(0.0), plVariant())),
    PL_MEMBER_PROPERTY("LifeScaleParam", m_sLifeScaleParameter),
    PL_MEMBER_PROPERTY("OnDeathEvent", m_sOnDeathEvent),
    PL_ARRAY_MEMBER_PROPERTY("Emitters", m_EmitterFactories)->AddFlags(plPropertyFlags::PointerOwner)->AddAttributes(new plMaxArraySizeAttribute(1)),
    PL_SET_ACCESSOR_PROPERTY("Initializers", GetInitializerFactories, AddInitializerFactory, RemoveInitializerFactory)->AddFlags(plPropertyFlags::PointerOwner)->AddAttributes(new plPreventDuplicatesAttribute()),
    PL_SET_ACCESSOR_PROPERTY("Behaviors", GetBehaviorFactories, AddBehaviorFactory, RemoveBehaviorFactory)->AddFlags(plPropertyFlags::PointerOwner)->AddAttributes(new plPreventDuplicatesAttribute()),
    PL_SET_ACCESSOR_PROPERTY("Types", GetTypeFactories, AddTypeFactory, RemoveTypeFactory)->AddFlags(plPropertyFlags::PointerOwner),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleSystemDescriptor::plParticleSystemDescriptor()
{
  m_bVisible = true;
}

plParticleSystemDescriptor::~plParticleSystemDescriptor()
{
  ClearEmitters();
  ClearInitializers();
  ClearBehaviors();
  ClearFinalizers();
  ClearTypes();
}

void plParticleSystemDescriptor::ClearEmitters()
{
  for (auto pFactory : m_EmitterFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_EmitterFactories.Clear();
}

void plParticleSystemDescriptor::ClearInitializers()
{
  for (auto pFactory : m_InitializerFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_InitializerFactories.Clear();
}

void plParticleSystemDescriptor::ClearBehaviors()
{
  for (auto pFactory : m_BehaviorFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_BehaviorFactories.Clear();
}

void plParticleSystemDescriptor::ClearTypes()
{
  for (auto pFactory : m_TypeFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_TypeFactories.Clear();
}

void plParticleSystemDescriptor::ClearFinalizers()
{
  for (auto pFactory : m_FinalizerFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_FinalizerFactories.Clear();
}

void plParticleSystemDescriptor::SetupDefaultProcessors()
{
  // Age Behavior
  {
    plParticleFinalizerFactory_Age* pFactory =
      plParticleFinalizerFactory_Age::GetStaticRTTI()->GetAllocator()->Allocate<plParticleFinalizerFactory_Age>();
    pFactory->m_LifeTime = m_LifeTime;
    pFactory->m_sOnDeathEvent = m_sOnDeathEvent;
    pFactory->m_sLifeScaleParameter = m_sLifeScaleParameter;
    m_FinalizerFactories.PushBack(pFactory);
  }

  // Bounding Volume Update Behavior
  {
    plParticleFinalizerFactory_Volume* pFactory =
      plParticleFinalizerFactory_Volume::GetStaticRTTI()->GetAllocator()->Allocate<plParticleFinalizerFactory_Volume>();
    m_FinalizerFactories.PushBack(pFactory);
  }

  if (m_TypeFactories.IsEmpty())
  {
    plParticleTypePointFactory* pFactory = plParticleTypePointFactory::GetStaticRTTI()->GetAllocator()->Allocate<plParticleTypePointFactory>();
    m_TypeFactories.PushBack(pFactory);
  }

  plSet<const plRTTI*> finalizers;
  for (const auto* pFactory : m_InitializerFactories)
  {
    pFactory->QueryFinalizerDependencies(finalizers);
  }

  for (const auto* pFactory : m_BehaviorFactories)
  {
    pFactory->QueryFinalizerDependencies(finalizers);
  }

  for (const auto* pFactory : m_TypeFactories)
  {
    pFactory->QueryFinalizerDependencies(finalizers);
  }

  for (const plRTTI* pRtti : finalizers)
  {
    PL_ASSERT_DEBUG(
      pRtti->IsDerivedFrom<plParticleFinalizerFactory>(), "Invalid finalizer factory added as a dependency: '{0}'", pRtti->GetTypeName());
    PL_ASSERT_DEBUG(pRtti->GetAllocator()->CanAllocate(), "Finalizer factory cannot be allocated: '{0}'", pRtti->GetTypeName());

    m_FinalizerFactories.PushBack(pRtti->GetAllocator()->Allocate<plParticleFinalizerFactory>());
  }
}

enum class ParticleSystemVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,
  Version_3,
  Version_4, // added Types
  Version_5, // added default processors
  Version_6, // changed lifetime variance
  Version_7, // added life scale param

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


plTime plParticleSystemDescriptor::GetAvgLifetime() const
{
  plTime time = m_LifeTime.m_Value + m_LifeTime.m_Value * (m_LifeTime.m_fVariance * 2.0f / 3.0f);

  // we actively prevent values outside the [0;2] range for the life-time scale parameter, when it is applied
  // so this is the accurate worst case value
  // effects should be authored with the maximum lifetime, and at runtime the lifetime should only be scaled down
  if (!m_sLifeScaleParameter.IsEmpty())
  {
    time = time * 2.0f;
  }

  return time;
}

void plParticleSystemDescriptor::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = (int)ParticleSystemVersion::Version_Current;

  inout_stream << uiVersion;

  const plUInt32 uiNumEmitters = m_EmitterFactories.GetCount();
  const plUInt32 uiNumInitializers = m_InitializerFactories.GetCount();
  const plUInt32 uiNumBehaviors = m_BehaviorFactories.GetCount();
  const plUInt32 uiNumTypes = m_TypeFactories.GetCount();

  plUInt32 uiMaxParticles = 0;
  inout_stream << m_bVisible;
  inout_stream << uiMaxParticles;
  inout_stream << m_LifeTime.m_Value;
  inout_stream << m_LifeTime.m_fVariance;
  inout_stream << m_sOnDeathEvent;
  inout_stream << m_sLifeScaleParameter;
  inout_stream << uiNumEmitters;
  inout_stream << uiNumInitializers;
  inout_stream << uiNumBehaviors;
  inout_stream << uiNumTypes;

  for (auto pEmitter : m_EmitterFactories)
  {
    inout_stream << pEmitter->GetDynamicRTTI()->GetTypeName();

    pEmitter->Save(inout_stream);
  }

  for (auto pInitializer : m_InitializerFactories)
  {
    inout_stream << pInitializer->GetDynamicRTTI()->GetTypeName();

    pInitializer->Save(inout_stream);
  }

  for (auto pBehavior : m_BehaviorFactories)
  {
    inout_stream << pBehavior->GetDynamicRTTI()->GetTypeName();

    pBehavior->Save(inout_stream);
  }

  for (auto pType : m_TypeFactories)
  {
    inout_stream << pType->GetDynamicRTTI()->GetTypeName();

    pType->Save(inout_stream);
  }
}


void plParticleSystemDescriptor::Load(plStreamReader& inout_stream)
{
  ClearEmitters();
  ClearInitializers();
  ClearBehaviors();
  ClearFinalizers();
  ClearTypes();

  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;
  PL_ASSERT_DEV(uiVersion <= (int)ParticleSystemVersion::Version_Current, "Unknown particle template version {0}", uiVersion);

  plUInt32 uiNumEmitters = 0;
  plUInt32 uiNumInitializers = 0;
  plUInt32 uiNumBehaviors = 0;
  plUInt32 uiNumTypes = 0;

  if (uiVersion >= 3)
  {
    inout_stream >> m_bVisible;
  }

  if (uiVersion >= 2)
  {
    // now unused
    plUInt32 uiMaxParticles = 0;
    inout_stream >> uiMaxParticles;
  }

  if (uiVersion >= 5)
  {
    inout_stream >> m_LifeTime.m_Value;
    inout_stream >> m_LifeTime.m_fVariance;
    inout_stream >> m_sOnDeathEvent;
  }

  if (uiVersion >= 7)
  {
    inout_stream >> m_sLifeScaleParameter;
  }

  inout_stream >> uiNumEmitters;

  if (uiVersion >= 2)
  {
    inout_stream >> uiNumInitializers;
  }

  inout_stream >> uiNumBehaviors;

  if (uiVersion >= 4)
  {
    inout_stream >> uiNumTypes;
  }

  m_EmitterFactories.SetCountUninitialized(uiNumEmitters);
  m_InitializerFactories.SetCountUninitialized(uiNumInitializers);
  m_BehaviorFactories.SetCountUninitialized(uiNumBehaviors);
  m_TypeFactories.SetCountUninitialized(uiNumTypes);

  plStringBuilder sType;

  for (auto& pEmitter : m_EmitterFactories)
  {
    inout_stream >> sType;

    const plRTTI* pRtti = plRTTI::FindTypeByName(sType);
    PL_ASSERT_DEBUG(pRtti != nullptr, "Unknown emitter factory type '{0}'", sType);

    pEmitter = pRtti->GetAllocator()->Allocate<plParticleEmitterFactory>();

    pEmitter->Load(inout_stream);
  }

  if (uiVersion >= 2)
  {
    for (auto& pInitializer : m_InitializerFactories)
    {
      inout_stream >> sType;

      const plRTTI* pRtti = plRTTI::FindTypeByName(sType);
      PL_ASSERT_DEBUG(pRtti != nullptr, "Unknown initializer factory type '{0}'", sType);

      pInitializer = pRtti->GetAllocator()->Allocate<plParticleInitializerFactory>();

      pInitializer->Load(inout_stream);
    }
  }

  for (auto& pBehavior : m_BehaviorFactories)
  {
    inout_stream >> sType;

    const plRTTI* pRtti = plRTTI::FindTypeByName(sType);
    PL_ASSERT_DEBUG(pRtti != nullptr, "Unknown behavior factory type '{0}'", sType);

    pBehavior = pRtti->GetAllocator()->Allocate<plParticleBehaviorFactory>();

    pBehavior->Load(inout_stream);
  }

  if (uiVersion >= 4)
  {
    for (auto& pType : m_TypeFactories)
    {
      inout_stream >> sType;

      const plRTTI* pRtti = plRTTI::FindTypeByName(sType);
      PL_ASSERT_DEBUG(pRtti != nullptr, "Unknown type factory type '{0}'", sType);

      pType = pRtti->GetAllocator()->Allocate<plParticleTypeFactory>();

      pType->Load(inout_stream);
    }
  }

  SetupDefaultProcessors();
}

//////////////////////////////////////////////////////////////////////////

class plParticleSystemDescriptor_1_2 : public plGraphPatch
{
public:
  plParticleSystemDescriptor_1_2()
    : plGraphPatch("plParticleSystemDescriptor", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("LifeTime").IgnoreResult();
  }
};

plParticleSystemDescriptor_1_2 g_plParticleSystemDescriptor_1_2;

PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_System_ParticleSystemDescriptor);
