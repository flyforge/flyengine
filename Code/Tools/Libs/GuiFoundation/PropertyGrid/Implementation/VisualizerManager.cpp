#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

PL_IMPLEMENT_SINGLETON(plVisualizerManager);

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, VisualizerManager)

  ON_CORESYSTEMS_STARTUP
  {
    PL_DEFAULT_NEW(plVisualizerManager);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (plVisualizerManager::GetSingleton())
    {
      auto ptr = plVisualizerManager::GetSingleton();
      PL_DEFAULT_DELETE(ptr);
    }
  }


PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

plVisualizerManager::plVisualizerManager()
  : m_SingletonRegistrar(this)
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plVisualizerManager::DocumentManagerEventHandler, this));
}

plVisualizerManager::~plVisualizerManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plVisualizerManager::DocumentManagerEventHandler, this));
}

void plVisualizerManager::SetVisualizersActive(const plDocument* pDoc, bool bActive)
{
  if (m_DocsSubscribed[pDoc].m_bActivated == bActive)
    return;

  m_DocsSubscribed[pDoc].m_bActivated = bActive;

  SendEventToRecreateVisualizers(pDoc);
}

bool plVisualizerManager::GetVisualizersActive(const plDocument* pDoc)
{
  return m_DocsSubscribed[pDoc].m_bActivated;
}

void plVisualizerManager::SelectionEventHandler(const plSelectionManagerEvent& event)
{
  if (!m_DocsSubscribed[event.m_pDocument].m_bActivated)
    return;

  SendEventToRecreateVisualizers(event.m_pDocument);
}

void plVisualizerManager::SendEventToRecreateVisualizers(const plDocument* pDoc)
{
  if (m_DocsSubscribed[pDoc].m_bActivated)
  {
    const auto& sel = pDoc->GetSelectionManager()->GetSelection();

    plVisualizerManagerEvent e;
    e.m_pSelection = &sel;
    e.m_pDocument = pDoc;
    m_Events.Broadcast(e);
  }
  else
  {
    plDeque<const plDocumentObject*> sel;

    plVisualizerManagerEvent e;
    e.m_pSelection = &sel;
    e.m_pDocument = pDoc;

    m_Events.Broadcast(e);
  }
}

void plVisualizerManager::DocumentManagerEventHandler(const plDocumentManager::Event& e)
{
  if (e.m_Type == plDocumentManager::Event::Type::DocumentOpened)
  {
    e.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(plMakeDelegate(&plVisualizerManager::SelectionEventHandler, this));
    e.m_pDocument->GetObjectManager()->m_StructureEvents.AddEventHandler(plMakeDelegate(&plVisualizerManager::StructureEventHandler, this));
  }

  if (e.m_Type == plDocumentManager::Event::Type::DocumentClosing)
  {
    e.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(plMakeDelegate(&plVisualizerManager::SelectionEventHandler, this));
    e.m_pDocument->GetObjectManager()->m_StructureEvents.RemoveEventHandler(plMakeDelegate(&plVisualizerManager::StructureEventHandler, this));

    SetVisualizersActive(e.m_pDocument, false);
  }
}

void plVisualizerManager::StructureEventHandler(const plDocumentObjectStructureEvent& event)
{
  if (!m_DocsSubscribed[event.m_pDocument].m_bActivated)
    return;

  if (!event.m_pDocument->GetSelectionManager()->IsSelectionEmpty() &&
      (event.m_EventType == plDocumentObjectStructureEvent::Type::AfterObjectAdded ||
        event.m_EventType == plDocumentObjectStructureEvent::Type::AfterObjectRemoved))
  {
    SendEventToRecreateVisualizers(event.m_pDocument);
  }
}
