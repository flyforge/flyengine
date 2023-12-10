#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <ToolsFoundation/Document/Document.h>

PLASMA_IMPLEMENT_SINGLETON(plManipulatorManager);

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, ManipulatorManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    PLASMA_DEFAULT_NEW(plManipulatorManager);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (plManipulatorManager::GetSingleton())
    {
      auto ptr = plManipulatorManager::GetSingleton();
      PLASMA_DEFAULT_DELETE(ptr);
    }
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

plManipulatorManager::plManipulatorManager()
  : m_SingletonRegistrar(this)
{
  plPhantomRttiManager::s_Events.AddEventHandler(plMakeDelegate(&plManipulatorManager::PhantomTypeManagerEventHandler, this));
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plManipulatorManager::DocumentManagerEventHandler, this));
}

plManipulatorManager::~plManipulatorManager()
{
  plPhantomRttiManager::s_Events.RemoveEventHandler(plMakeDelegate(&plManipulatorManager::PhantomTypeManagerEventHandler, this));
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plManipulatorManager::DocumentManagerEventHandler, this));
}

const plManipulatorAttribute* plManipulatorManager::GetActiveManipulator(
  const plDocument* pDoc, const plHybridArray<plPropertySelection, 8>*& out_pSelection) const
{
  out_pSelection = nullptr;
  auto it = m_ActiveManipulator.Find(pDoc);

  if (it.IsValid())
  {
    out_pSelection = &(it.Value().m_Selection);

    return it.Value().m_pAttribute;
  }

  return nullptr;
}

void plManipulatorManager::InternalSetActiveManipulator(
  const plDocument* pDoc, const plManipulatorAttribute* pManipulator, const plHybridArray<plPropertySelection, 8>& selection, bool bUnhide)
{
  bool existed = false;
  auto it = m_ActiveManipulator.FindOrAdd(pDoc, &existed);

  it.Value().m_pAttribute = pManipulator;
  it.Value().m_Selection = selection;

  if (!existed)
  {
    pDoc->GetObjectManager()->m_StructureEvents.AddEventHandler(plMakeDelegate(&plManipulatorManager::StructureEventHandler, this));
    pDoc->GetSelectionManager()->m_Events.AddEventHandler(plMakeDelegate(&plManipulatorManager::SelectionEventHandler, this));
  }

  auto& data = m_ActiveManipulator[pDoc];

  if (bUnhide)
  {
    data.m_bHideManipulators = false;
  }

  plManipulatorManagerEvent e;
  e.m_bHideManipulators = data.m_bHideManipulators;
  e.m_pDocument = pDoc;
  e.m_pManipulator = pManipulator;
  e.m_pSelection = &data.m_Selection;

  m_Events.Broadcast(e);
}


void plManipulatorManager::SetActiveManipulator(
  const plDocument* pDoc, const plManipulatorAttribute* pManipulator, const plHybridArray<plPropertySelection, 8>& selection)
{
  InternalSetActiveManipulator(pDoc, pManipulator, selection, true);
}

void plManipulatorManager::ClearActiveManipulator(const plDocument* pDoc)
{
  plHybridArray<plPropertySelection, 8> clearSel;

  InternalSetActiveManipulator(pDoc, nullptr, clearSel, false);
}

void plManipulatorManager::HideActiveManipulator(const plDocument* pDoc, bool bHide)
{
  auto it = m_ActiveManipulator.Find(pDoc);

  if (it.IsValid() && it.Value().m_bHideManipulators != bHide)
  {
    it.Value().m_bHideManipulators = bHide;

    if (bHide)
    {
      plHybridArray<plPropertySelection, 8> clearSel;
      InternalSetActiveManipulator(pDoc, it.Value().m_pAttribute, clearSel, false);
    }
    else
    {
      TransferToCurrentSelection(pDoc);
    }
  }
}

void plManipulatorManager::ToggleHideActiveManipulator(const plDocument* pDoc)
{
  auto it = m_ActiveManipulator.Find(pDoc);

  if (it.IsValid())
  {
    it.Value().m_bHideManipulators = !it.Value().m_bHideManipulators;

    if (it.Value().m_bHideManipulators)
    {
      plHybridArray<plPropertySelection, 8> clearSel;
      InternalSetActiveManipulator(pDoc, it.Value().m_pAttribute, clearSel, false);
    }
    else
    {
      TransferToCurrentSelection(pDoc);
    }
  }
}

void plManipulatorManager::StructureEventHandler(const plDocumentObjectStructureEvent& e)
{
  if (e.m_EventType == plDocumentObjectStructureEvent::Type::BeforeObjectRemoved)
  {
    auto pDoc = e.m_pObject->GetDocumentObjectManager()->GetDocument();
    auto it = m_ActiveManipulator.Find(pDoc);

    if (it.IsValid())
    {
      for (auto& sel : it.Value().m_Selection)
      {
        if (sel.m_pObject == e.m_pObject)
        {
          it.Value().m_Selection.RemoveAndCopy(sel);
          InternalSetActiveManipulator(pDoc, it.Value().m_pAttribute, it.Value().m_Selection, false);
          return;
        }
      }
    }
  }

  if (e.m_EventType == plDocumentObjectStructureEvent::Type::BeforeReset)
  {
    auto pDoc = e.m_pDocument;
    auto it = m_ActiveManipulator.Find(pDoc);

    if (it.IsValid())
    {
      for (auto& sel : it.Value().m_Selection)
      {
        it.Value().m_Selection.RemoveAndCopy(sel);
        InternalSetActiveManipulator(pDoc, it.Value().m_pAttribute, it.Value().m_Selection, false);
      }
    }
  }
}

void plManipulatorManager::SelectionEventHandler(const plSelectionManagerEvent& e)
{
  TransferToCurrentSelection(e.m_pDocument->GetMainDocument());
}

void plManipulatorManager::TransferToCurrentSelection(const plDocument* pDoc)
{
  auto& data = m_ActiveManipulator[pDoc];
  auto pAttribute = data.m_pAttribute;

  if (pAttribute == nullptr)
    return;

  if (data.m_bHideManipulators)
    return;

  plHybridArray<plPropertySelection, 8> newSelection;

  const auto& selection = pDoc->GetSelectionManager()->GetSelection();

  PLASMA_ASSERT_DEV(pDoc->GetManipulatorSearchStrategy() != plManipulatorSearchStrategy::None, "The document type '{}' has to override the function 'GetManipulatorSearchStrategy()'", pDoc->GetDynamicRTTI()->GetTypeName());

  if (pDoc->GetManipulatorSearchStrategy() == plManipulatorSearchStrategy::SelectedObject)
  {
    for (plUInt32 i = 0; i < selection.GetCount(); ++i)
    {
      const auto& OtherAttributes = selection[i]->GetTypeAccessor().GetType()->GetAttributes();

      for (const auto pOtherAttr : OtherAttributes)
      {
        if (pOtherAttr->IsInstanceOf(pAttribute->GetDynamicRTTI()))
        {
          auto pOtherManip = static_cast<const plManipulatorAttribute*>(pOtherAttr);

          if (pOtherManip->m_sProperty1 == pAttribute->m_sProperty1 && pOtherManip->m_sProperty2 == pAttribute->m_sProperty2 &&
              pOtherManip->m_sProperty3 == pAttribute->m_sProperty3 && pOtherManip->m_sProperty4 == pAttribute->m_sProperty4 &&
              pOtherManip->m_sProperty5 == pAttribute->m_sProperty5 && pOtherManip->m_sProperty6 == pAttribute->m_sProperty6)
          {
            auto& newItem = newSelection.ExpandAndGetRef();
            newItem.m_pObject = selection[i];
          }
        }
      }
    }
  }

  if (pDoc->GetManipulatorSearchStrategy() == plManipulatorSearchStrategy::ChildrenOfSelectedObject)
  {
    for (plUInt32 i = 0; i < selection.GetCount(); ++i)
    {
      const auto& children = selection[i]->GetChildren();

      for (const auto& child : children)
      {
        const auto& OtherAttributes = child->GetTypeAccessor().GetType()->GetAttributes();

        for (const auto pOtherAttr : OtherAttributes)
        {
          if (pOtherAttr->IsInstanceOf(pAttribute->GetDynamicRTTI()))
          {
            auto pOtherManip = static_cast<const plManipulatorAttribute*>(pOtherAttr);

            if (pOtherManip->m_sProperty1 == pAttribute->m_sProperty1 && pOtherManip->m_sProperty2 == pAttribute->m_sProperty2 &&
                pOtherManip->m_sProperty3 == pAttribute->m_sProperty3 && pOtherManip->m_sProperty4 == pAttribute->m_sProperty4 &&
                pOtherManip->m_sProperty5 == pAttribute->m_sProperty5 && pOtherManip->m_sProperty6 == pAttribute->m_sProperty6)
            {
              auto& newItem = newSelection.ExpandAndGetRef();
              newItem.m_pObject = child;
            }
          }
        }
      }
    }
  }

  InternalSetActiveManipulator(pDoc, pAttribute, newSelection, false);
}

void plManipulatorManager::PhantomTypeManagerEventHandler(const plPhantomRttiManagerEvent& e)
{
  if (e.m_Type == plPhantomRttiManagerEvent::Type::TypeChanged || e.m_Type == plPhantomRttiManagerEvent::Type::TypeRemoved)
  {
    for (auto it = m_ActiveManipulator.GetIterator(); it.IsValid(); ++it)
    {
      ClearActiveManipulator(it.Key());
    }
  }
}

void plManipulatorManager::DocumentManagerEventHandler(const plDocumentManager::Event& e)
{
  if (e.m_Type == plDocumentManager::Event::Type::DocumentClosing)
  {
    ClearActiveManipulator(e.m_pDocument);

    e.m_pDocument->GetObjectManager()->m_StructureEvents.RemoveEventHandler(plMakeDelegate(&plManipulatorManager::StructureEventHandler, this));
    e.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(plMakeDelegate(&plManipulatorManager::SelectionEventHandler, this));

    m_ActiveManipulator.Remove(e.m_pDocument);
  }
}
