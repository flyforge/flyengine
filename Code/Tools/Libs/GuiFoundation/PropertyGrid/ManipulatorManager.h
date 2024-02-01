#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <ToolsFoundation/Document/DocumentManager.h>

class plManipulatorAttribute;
struct plPhantomRttiManagerEvent;
struct plSelectionManagerEvent;

struct PL_GUIFOUNDATION_DLL plManipulatorManagerEvent
{
  const plDocument* m_pDocument;
  const plManipulatorAttribute* m_pManipulator;
  const plHybridArray<plPropertySelection, 8>* m_pSelection;
  bool m_bHideManipulators;
};

class PL_GUIFOUNDATION_DLL plManipulatorManager
{
  PL_DECLARE_SINGLETON(plManipulatorManager);

public:
  plManipulatorManager();
  ~plManipulatorManager();

  const plManipulatorAttribute* GetActiveManipulator(const plDocument* pDoc, const plHybridArray<plPropertySelection, 8>*& out_pSelection) const;

  void SetActiveManipulator(
    const plDocument* pDoc, const plManipulatorAttribute* pManipulator, const plHybridArray<plPropertySelection, 8>& selection);

  void ClearActiveManipulator(const plDocument* pDoc);

  plCopyOnBroadcastEvent<const plManipulatorManagerEvent&> m_Events;

  void HideActiveManipulator(const plDocument* pDoc, bool bHide);
  void ToggleHideActiveManipulator(const plDocument* pDoc);

private:
  struct Data
  {
    Data()
    {
      m_pAttribute = nullptr;
      m_bHideManipulators = false;
    }

    const plManipulatorAttribute* m_pAttribute;
    plHybridArray<plPropertySelection, 8> m_Selection;
    bool m_bHideManipulators;
  };

  void InternalSetActiveManipulator(
    const plDocument* pDoc, const plManipulatorAttribute* pManipulator, const plHybridArray<plPropertySelection, 8>& selection, bool bUnhide);

  void StructureEventHandler(const plDocumentObjectStructureEvent& e);
  void SelectionEventHandler(const plSelectionManagerEvent& e);

  void TransferToCurrentSelection(const plDocument* pDoc);

  void PhantomTypeManagerEventHandler(const plPhantomRttiManagerEvent& e);
  void DocumentManagerEventHandler(const plDocumentManager::Event& e);

  plMap<const plDocument*, Data> m_ActiveManipulator;
};
