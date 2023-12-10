#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorPluginScene/InputContexts/SceneSelectionContext.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>

plSceneSelectionContext::plSceneSelectionContext(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView, const plCamera* pCamera)
  : plSelectionContext(pOwnerWindow, pOwnerView, pCamera)
{
}

void plSceneSelectionContext::OpenDocumentForPickedObject(const plObjectPickingResult& res) const
{
  plSelectionContext::OpenDocumentForPickedObject(res);
}

void plSceneSelectionContext::SelectPickedObject(const plObjectPickingResult& res, bool bToggle, bool bDirect) const
{
  // If bToggle (ctrl-key) is held, we don't want to switch layers.
  // Same if we have a custom pick override set which usually means that the selection is hijacked to make an object modification on the current layer.
  if (res.m_PickedObject.IsValid() && !bToggle)
  {
    const plDocumentObject* pObject = nullptr;
    plUuid layerGuid = FindLayerByObject(res.m_PickedObject, pObject);
    if (layerGuid.IsValid())
    {
      plScene2Document* pSceneDocument = plDynamicCast<plScene2Document*>(GetOwnerWindow()->GetDocument());
      if (pSceneDocument->IsLayerLoaded(layerGuid))
      {
        if (m_PickObjectOverride.IsValid())
        {
          m_PickObjectOverride(pObject);
          return;
        }
        pSceneDocument->SetActiveLayer(layerGuid).IgnoreResult();
      }
    }
  }
  plSelectionContext::SelectPickedObject(res, bToggle, bDirect);
}

plUuid plSceneSelectionContext::FindLayerByObject(plUuid objectGuid, const plDocumentObject*& out_pObject) const
{
  plHybridArray<plSceneDocument*, 8> loadedLayers;
  const plScene2Document* pSceneDocument = plDynamicCast<const plScene2Document*>(GetOwnerWindow()->GetDocument());
  pSceneDocument->GetLoadedLayers(loadedLayers);
  for (plSceneDocument* pLayer : loadedLayers)
  {
    if (pLayer == pSceneDocument)
    {
      if ((out_pObject = pSceneDocument->GetSceneObjectManager()->GetObject(objectGuid)))
      {
        return pLayer->GetGuid();
      }
    }
    else if ((out_pObject = pLayer->GetObjectManager()->GetObject(objectGuid)))
    {
      return pLayer->GetGuid();
    }
  }
  out_pObject = nullptr;
  return plUuid();
}
