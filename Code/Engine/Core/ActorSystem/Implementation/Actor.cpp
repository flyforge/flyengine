#include <Core/CorePCH.h>

#include <Core/ActorSystem/Actor.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plActor, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

struct plActorImpl
{
  plString m_sName;
  const void* m_pCreatedBy = nullptr;
  plHybridArray<plUniquePtr<plActorPlugin>, 4> m_AllPlugins;
  plMap<const plRTTI*, plActorPlugin*> m_PluginLookupCache;
};


plActor::plActor(plStringView szActorName, const void* pCreatedBy)
{
  m_pImpl = PLASMA_DEFAULT_NEW(plActorImpl);

  m_pImpl->m_sName = szActorName;
  m_pImpl->m_pCreatedBy = pCreatedBy;

  PLASMA_ASSERT_DEV(!m_pImpl->m_sName.IsEmpty(), "Actor name must not be empty");
}

plActor::~plActor() = default;

plStringView plActor::GetName() const
{
  return m_pImpl->m_sName;
}

const void* plActor::GetCreatedBy() const
{
  return m_pImpl->m_pCreatedBy;
}

void plActor::AddPlugin(plUniquePtr<plActorPlugin>&& pPlugin)
{
  PLASMA_ASSERT_DEV(pPlugin != nullptr, "Invalid actor plugin");
  PLASMA_ASSERT_DEV(pPlugin->m_pOwningActor == nullptr, "Actor plugin already in use");

  pPlugin->m_pOwningActor = this;

  // register this plugin under its type and all its base types
  for (const plRTTI* pRtti = pPlugin->GetDynamicRTTI(); pRtti != plGetStaticRTTI<plActorPlugin>(); pRtti = pRtti->GetParentType())
  {
    m_pImpl->m_PluginLookupCache[pRtti] = pPlugin.Borrow();
  }

  m_pImpl->m_AllPlugins.PushBack(std::move(pPlugin));
}

plActorPlugin* plActor::GetPlugin(const plRTTI* pPluginType) const
{
  PLASMA_ASSERT_DEV(pPluginType->IsDerivedFrom<plActorPlugin>(), "The queried type has to derive from plActorPlugin");

  return m_pImpl->m_PluginLookupCache.GetValueOrDefault(pPluginType, nullptr);
}

void plActor::DestroyPlugin(plActorPlugin* pPlugin)
{
  for (plUInt32 i = 0; i < m_pImpl->m_AllPlugins.GetCount(); ++i)
  {
    if (m_pImpl->m_AllPlugins[i] == pPlugin)
    {
      m_pImpl->m_AllPlugins.RemoveAtAndSwap(i);
      break;
    }
  }
}

void plActor::GetAllPlugins(plHybridArray<plActorPlugin*, 8>& out_AllPlugins)
{
  out_AllPlugins.Clear();

  for (auto& pPlugin : m_pImpl->m_AllPlugins)
  {
    out_AllPlugins.PushBack(pPlugin.Borrow());
  }
}

void plActor::UpdateAllPlugins()
{
  for (auto& pPlugin : m_pImpl->m_AllPlugins)
  {
    pPlugin->Update();
  }
}

void plActor::Activate() {}

void plActor::Update()
{
  UpdateAllPlugins();
}


PLASMA_STATICLINK_FILE(Core, Core_ActorSystem_Implementation_Actor);
