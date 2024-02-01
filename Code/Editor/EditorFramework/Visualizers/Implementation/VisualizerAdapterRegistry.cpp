#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>

PL_IMPLEMENT_SINGLETON(plVisualizerAdapterRegistry);

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, VisualizerAdapterRegistry)
 
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "VisualizerManager"
  END_SUBSYSTEM_DEPENDENCIES
 
  ON_CORESYSTEMS_STARTUP
  {
    PL_DEFAULT_NEW(plVisualizerAdapterRegistry);
  }
 
  ON_CORESYSTEMS_SHUTDOWN
  {
    auto ptr = plVisualizerAdapterRegistry::GetSingleton();
    PL_DEFAULT_DELETE(ptr);
  }
 
PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

plVisualizerAdapterRegistry::plVisualizerAdapterRegistry()
  : m_SingletonRegistrar(this)
{
  plVisualizerManager::GetSingleton()->m_Events.AddEventHandler(plMakeDelegate(&plVisualizerAdapterRegistry::VisualizerManagerEventHandler, this));
}

plVisualizerAdapterRegistry::~plVisualizerAdapterRegistry()
{
  plVisualizerManager::GetSingleton()->m_Events.RemoveEventHandler(plMakeDelegate(&plVisualizerAdapterRegistry::VisualizerManagerEventHandler, this));

  for (auto it = m_DocumentAdapters.GetIterator(); it.IsValid(); ++it)
  {
    ClearAdapters(it.Key());
  }
}


void plVisualizerAdapterRegistry::CreateAdapters(const plDocument* pDocument, const plDocumentObject* pObject)
{
  const auto& attributes = pObject->GetTypeAccessor().GetType()->GetAttributes();

  for (const auto pAttr : attributes)
  {
    if (pAttr->IsInstanceOf<plVisualizerAttribute>())
    {
      plVisualizerAdapter* pAdapter = m_Factory.CreateObject(pAttr->GetDynamicRTTI());

      if (pAdapter)
      {
        m_DocumentAdapters[pDocument].m_Adapters.PushBack(pAdapter);
        pAdapter->SetVisualizer(static_cast<const plVisualizerAttribute*>(pAttr), pObject);
      }
    }
  }

  for (const auto pChild : pObject->GetChildren())
  {
    CreateAdapters(pDocument, pChild);
  }
}

void plVisualizerAdapterRegistry::VisualizerManagerEventHandler(const plVisualizerManagerEvent& e)
{
  ClearAdapters(e.m_pDocument);

  for (const auto sel : *e.m_pSelection)
  {
    CreateAdapters(e.m_pDocument, sel);
  }
}

void plVisualizerAdapterRegistry::ClearAdapters(const plDocument* pDocument)
{
  for (auto& adapt : m_DocumentAdapters[pDocument].m_Adapters)
  {
    PL_DEFAULT_DELETE(adapt);
  }

  m_DocumentAdapters[pDocument].m_Adapters.Clear();
}
