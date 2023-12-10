#pragma once

#include <EditorFramework/InputContexts/SelectionContext.h>

/// \brief Custom selection context for the scene to allow switching the active layer if an object is clicked that is in a different layer then the active one.
class plSceneSelectionContext : public plSelectionContext
{
public:
  plSceneSelectionContext(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView, const plCamera* pCamera);

protected:
  virtual void OpenDocumentForPickedObject(const plObjectPickingResult& res) const override;
  virtual void SelectPickedObject(const plObjectPickingResult& res, bool bToggle, bool bDirect) const override;

  plUuid FindLayerByObject(plUuid objectGuid, const plDocumentObject*& out_pObject) const;
};
