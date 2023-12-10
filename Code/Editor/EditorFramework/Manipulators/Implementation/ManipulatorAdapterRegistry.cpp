#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Manipulators/ManipulatorAdapterRegistry.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>

PLASMA_IMPLEMENT_SINGLETON(plManipulatorAdapterRegistry);

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, ManipulatorAdapterRegistry)
 
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ManipulatorManager"
  END_SUBSYSTEM_DEPENDENCIES
 
  ON_CORESYSTEMS_STARTUP
  {
    PLASMA_DEFAULT_NEW(plManipulatorAdapterRegistry);
  }
 
  ON_CORESYSTEMS_SHUTDOWN
  {
    auto ptr = plManipulatorAdapterRegistry::GetSingleton();
    PLASMA_DEFAULT_DELETE(ptr);
  }
 
PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

plManipulatorAdapterRegistry::plManipulatorAdapterRegistry()
  : m_SingletonRegistrar(this)
{
  plManipulatorManager::GetSingleton()->m_Events.AddEventHandler(plMakeDelegate(&plManipulatorAdapterRegistry::ManipulatorManagerEventHandler, this));
}

plManipulatorAdapterRegistry::~plManipulatorAdapterRegistry()
{
  plManipulatorManager::GetSingleton()->m_Events.RemoveEventHandler(
    plMakeDelegate(&plManipulatorAdapterRegistry::ManipulatorManagerEventHandler, this));

  for (auto it = m_DocumentAdapters.GetIterator(); it.IsValid(); ++it)
  {
    ClearAdapters(it.Key());
  }
}

void plManipulatorAdapterRegistry::QueryGridSettings(const plDocument* pDocument, plGridSettingsMsgToEngine& out_gridSettings)
{
  for (auto& adapt : m_DocumentAdapters[pDocument].m_Adapters)
  {
    adapt->QueryGridSettings(out_gridSettings);
  }
}

void plManipulatorAdapterRegistry::ManipulatorManagerEventHandler(const plManipulatorManagerEvent& e)
{
  ClearAdapters(e.m_pDocument);

  if (e.m_pManipulator == nullptr || e.m_bHideManipulators)
    return;

  for (const auto& sel : *e.m_pSelection)
  {
    plManipulatorAdapter* pAdapter = m_Factory.CreateObject(e.m_pManipulator->GetDynamicRTTI());

    if (pAdapter)
    {
      m_DocumentAdapters[e.m_pDocument].m_Adapters.PushBack(pAdapter);
      pAdapter->SetManipulator(e.m_pManipulator, sel.m_pObject);
    }
  }
}

void plManipulatorAdapterRegistry::ClearAdapters(const plDocument* pDocument)
{
  for (auto& adapt : m_DocumentAdapters[pDocument].m_Adapters)
  {
    PLASMA_DEFAULT_DELETE(adapt);
  }

  m_DocumentAdapters[pDocument].m_Adapters.Clear();
}
