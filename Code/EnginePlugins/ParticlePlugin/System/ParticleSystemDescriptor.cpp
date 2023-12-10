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
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleSystemDescriptor, 2, plRTTIDefaultAllocator<plParticleSystemDescriptor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Name", m_sName),
    PLASMA_MEMBER_PROPERTY("Visible", m_bVisible)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("LifeTime", m_LifeTime)->AddAttributes(new plDefaultValueAttribute(plTime::Seconds(2)), new plClampValueAttribute(plTime::Seconds(0.0), plVariant())),
    PLASMA_MEMBER_PROPERTY("LifeScaleParam", m_sLifeScaleParameter),
    PLASMA_MEMBER_PROPERTY("OnDeathEvent", m_sOnDeathEvent),
    PLASMA_ARRAY_MEMBER_PROPERTY("Emitters", m_EmitterFactories)->AddFlags(plPropertyFlags::PointerOwner)->AddAttributes(new plMaxArraySizeAttribute(1)),
    PLASMA_SET_ACCESSOR_PROPERTY("Initializers", GetInitializerFactories, AddInitializerFactory, RemoveInitializerFactory)->AddFlags(plPropertyFlags::PointerOwner)->AddAttributes(new plPreventDuplicatesAttribute()),
    PLASMA_SET_ACCESSOR_PROPERTY("Behaviors", GetBehaviorFactories, AddBehaviorFactory, RemoveBehaviorFactory)->AddFlags(plPropertyFlags::PointerOwner)->AddAttributes(new plPreventDuplicatesAttribute()),
    PLASMA_SET_ACCESSOR_PROPERTY("Types", GetTypeFactories, AddTypeFactory, RemoveTypeFactory)->AddFlags(plPropertyFlags::PointerOwner),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
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
    PLASMA_ASSERT_DEBUG(
      pRtti->IsDerivedFrom<plParticleFinalizerFactory>(), "Invalid finalizer factory added as a dependency: '{0}'", pRtti->GetTypeName());
    PLASMA_ASSERT_DEBUG(pRtti->GetAllocator()->CanAllocate(), "Finalizer factory cannot be allocated: '{0}'", pRtti->GetTypeName());

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

void plParticleSystemDescriptor::Save(plStreamWriter& stream) const
{
  const plUInt8 uiVersion = (int)ParticleSystemVersion::Version_Current;

  stream << uiVersion;

  const plUInt32 uiNumEmitters = m_EmitterFactories.GetCount();
  const plUInt32 uiNumInitializers = m_InitializerFactories.GetCount();
  const plUInt32 uiNumBehaviors = m_BehaviorFactories.GetCount();
  const plUInt32 uiNumTypes = m_TypeFactories.GetCount();

  plUInt32 uiMaxParticles = 0;
  stream << m_bVisible;
  stream << uiMaxParticles;
  stream << m_LifeTime.m_Value;
  stream << m_LifeTime.m_fVariance;
  stream << m_sOnDeathEvent;
  stream << m_sLifeScaleParameter;
  stream << uiNumEmitters;
  stream << uiNumInitializers;
  stream << uiNumBehaviors;
  stream << uiNumTypes;

  for (auto pEmitter : m_EmitterFactories)
  {
    stream << pEmitter->GetDynamicRTTI()->GetTypeName();

    pEmitter->Save(stream);
  }

  for (auto pInitializer : m_InitializerFactories)
  {
    stream << pInitializer->GetDynamicRTTI()->GetTypeName();

    pInitializer->Save(stream);
  }

  for (auto pBehavior : m_BehaviorFactories)
  {
    stream << pBehavior->GetDynamicRTTI()->GetTypeName();

    pBehavior->Save(stream);
  }

  for (auto pType : m_TypeFactories)
  {
    stream << pType->GetDynamicRTTI()->GetTypeName();

    pType->Save(stream);
  }
}


void plParticleSystemDescriptor::Load(plStreamReader& stream)
{
  ClearEmitters();
  ClearInitializers();
  ClearBehaviors();
  ClearFinalizers();
  ClearTypes();

  plUInt8 uiVersion = 0;
  stream >> uiVersion;
  PLASMA_ASSERT_DEV(uiVersion <= (int)ParticleSystemVersion::Version_Current, "Unknown particle template version {0}", uiVersion);

  plUInt32 uiNumEmitters = 0;
  plUInt32 uiNumInitializers = 0;
  plUInt32 uiNumBehaviors = 0;
  plUInt32 uiNumTypes = 0;

  if (uiVersion >= 3)
  {
    stream >> m_bVisible;
  }

  if (uiVersion >= 2)
  {
    // now unused
    plUInt32 uiMaxParticles = 0;
    stream >> uiMaxParticles;
  }

  if (uiVersion >= 5)
  {
    stream >> m_LifeTime.m_Value;
    stream >> m_LifeTime.m_fVariance;
    stream >> m_sOnDeathEvent;
  }

  if (uiVersion >= 7)
  {
    stream >> m_sLifeScaleParameter;
  }

  stream >> uiNumEmitters;

  if (uiVersion >= 2)
  {
    stream >> uiNumInitializers;
  }

  stream >> uiNumBehaviors;

  if (uiVersion >= 4)
  {
    stream >> uiNumTypes;
  }

  m_EmitterFactories.SetCountUninitialized(uiNumEmitters);
  m_InitializerFactories.SetCountUninitialized(uiNumInitializers);
  m_BehaviorFactories.SetCountUninitialized(uiNumBehaviors);
  m_TypeFactories.SetCountUninitialized(uiNumTypes);

  plStringBuilder sType;

  for (auto& pEmitter : m_EmitterFactories)
  {
    stream >> sType;

    const plRTTI* pRtti = plRTTI::FindTypeByName(sType);
    PLASMA_ASSERT_DEBUG(pRtti != nullptr, "Unknown emitter factory type '{0}'", sType);

    pEmitter = pRtti->GetAllocator()->Allocate<plParticleEmitterFactory>();

    pEmitter->Load(stream);
  }

  if (uiVersion >= 2)
  {
    for (auto& pInitializer : m_InitializerFactories)
    {
      stream >> sType;

      const plRTTI* pRtti = plRTTI::FindTypeByName(sType);
      PLASMA_ASSERT_DEBUG(pRtti != nullptr, "Unknown initializer factory type '{0}'", sType);

      pInitializer = pRtti->GetAllocator()->Allocate<plParticleInitializerFactory>();

      pInitializer->Load(stream);
    }
  }

  for (auto& pBehavior : m_BehaviorFactories)
  {
    stream >> sType;

    const plRTTI* pRtti = plRTTI::FindTypeByName(sType);
    PLASMA_ASSERT_DEBUG(pRtti != nullptr, "Unknown behavior factory type '{0}'", sType);

    pBehavior = pRtti->GetAllocator()->Allocate<plParticleBehaviorFactory>();

    pBehavior->Load(stream);
  }

  if (uiVersion >= 4)
  {
    for (auto& pType : m_TypeFactories)
    {
      stream >> sType;

      const plRTTI* pRtti = plRTTI::FindTypeByName(sType);
      PLASMA_ASSERT_DEBUG(pRtti != nullptr, "Unknown type factory type '{0}'", sType);

      pType = pRtti->GetAllocator()->Allocate<plParticleTypeFactory>();

      pType->Load(stream);
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

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("LifeTime").IgnoreResult();
  }
};

plParticleSystemDescriptor_1_2 g_plParticleSystemDescriptor_1_2;

PLASMA_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_System_ParticleSystemDescriptor);
