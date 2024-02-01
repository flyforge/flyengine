#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Document/DocumentManager.h>

struct plSelectionManagerEvent;
class plDocumentObject;
class plVisualizerAttribute;

struct PL_GUIFOUNDATION_DLL plVisualizerManagerEvent
{
  const plDocument* m_pDocument;
  const plDeque<const plDocumentObject*>* m_pSelection;
};

class PL_GUIFOUNDATION_DLL plVisualizerManager
{
  PL_DECLARE_SINGLETON(plVisualizerManager);

public:
  plVisualizerManager();
  ~plVisualizerManager();

  void SetVisualizersActive(const plDocument* pDoc, bool bActive);
  bool GetVisualizersActive(const plDocument* pDoc);

  plEvent<const plVisualizerManagerEvent&> m_Events;

private:
  void SelectionEventHandler(const plSelectionManagerEvent& e);
  void DocumentManagerEventHandler(const plDocumentManager::Event& e);
  void StructureEventHandler(const plDocumentObjectStructureEvent& e);
  void SendEventToRecreateVisualizers(const plDocument* pDoc);

  struct DocData
  {
    bool m_bActivated;

    DocData() { m_bActivated = true; }
  };

  plMap<const plDocument*, DocData> m_DocsSubscribed;
};
