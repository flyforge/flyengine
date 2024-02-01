#include <EditorPluginScene/EditorPluginScenePCH.h>

#include "EditorFramework/GUI/RawDocumentTreeModel.moc.h"
#include <Core/World/GameObject.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/LayerDragDropHandler.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <QIODevice>
#include <QMimeData>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/DocumentManager.h>

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLayerDragDropHandler, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

const plRTTI* plLayerDragDropHandler::GetCommonBaseType(const plDragDropInfo* pInfo) const
{
  QByteArray encodedData = pInfo->m_pMimeData->data("application/plEditor.ObjectSelection");
  QDataStream stream(&encodedData, QIODevice::ReadOnly);
  plHybridArray<plDocumentObject*, 32> Dragged;
  stream >> Dragged;

  const plRTTI* pCommonBaseType = nullptr;
  for (const plDocumentObject* pItem : Dragged)
  {
    pCommonBaseType = pCommonBaseType == nullptr ? pItem->GetType() : plReflectionUtils::GetCommonBaseType(pCommonBaseType, pItem->GetType());
  }
  return pCommonBaseType;
}


//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLayerOnLayerDragDropHandler, 1, plRTTIDefaultAllocator<plLayerOnLayerDragDropHandler>)
PL_END_DYNAMIC_REFLECTED_TYPE;

float plLayerOnLayerDragDropHandler::CanHandle(const plDragDropInfo* pInfo) const
{
  if (pInfo->m_sTargetContext == "layertree" && pInfo->m_pMimeData->hasFormat("application/plEditor.ObjectSelection"))
  {
    if (plScene2Document* pDoc = plDynamicCast<plScene2Document*>(plDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument)))
    {
      const plDocumentObject* pTarget = pDoc->GetSceneObjectManager()->GetObject(pInfo->m_TargetObject);
      if (pTarget && pInfo->m_pAdapter && GetCommonBaseType(pInfo)->IsDerivedFrom(plGetStaticRTTI<plSceneLayerBase>()))
      {
        const plAbstractProperty* pTargetProp = pInfo->m_pAdapter->GetType()->FindPropertyByName(pInfo->m_pAdapter->GetChildProperty());
        if (pTargetProp && plGetStaticRTTI<plSceneLayerBase>()->IsDerivedFrom(pTargetProp->GetSpecificType()))
          return 1.0f;
      }
    }
  }
  return 0;
}

void plLayerOnLayerDragDropHandler::OnDrop(const plDragDropInfo* pInfo)
{
  if (plScene2Document* pDoc = plDynamicCast<plScene2Document*>(plDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument)))
  {
    const plUuid activeDoc = pDoc->GetActiveLayer();
    PL_VERIFY(pDoc->SetActiveLayer(pDoc->GetGuid()).Succeeded(), "Failed to set active document.");
    {
      // We need to make a copy of the info as the target document is actually the scene here, not the active document.
      plDragDropInfo info = *pInfo;
      info.m_TargetDocument = pDoc->GetGuid();
      plQtDocumentTreeModel::MoveObjects(info);
    }
    PL_VERIFY(pDoc->SetActiveLayer(activeDoc).Succeeded(), "Failed to set active document.");
  }
}

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plGameObjectOnLayerDragDropHandler, 1, plRTTIDefaultAllocator<plGameObjectOnLayerDragDropHandler>)
PL_END_DYNAMIC_REFLECTED_TYPE;

float plGameObjectOnLayerDragDropHandler::CanHandle(const plDragDropInfo* pInfo) const
{
  if (pInfo->m_sTargetContext == "layertree" && pInfo->m_pMimeData->hasFormat("application/plEditor.ObjectSelection"))
  {
    if (plScene2Document* pDoc = plDynamicCast<plScene2Document*>(plDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument)))
    {
      const plDocumentObject* pTarget = pDoc->GetSceneObjectManager()->GetObject(pInfo->m_TargetObject);
      if (pTarget && pTarget->GetType() == plGetStaticRTTI<plSceneLayer>() && pInfo->m_iTargetObjectInsertChildIndex == -1 && GetCommonBaseType(pInfo) == plGetStaticRTTI<plGameObject>())
      {
        plObjectAccessorBase* pAccessor = pDoc->GetSceneObjectAccessor();
        plUuid layerGuid = pAccessor->Get<plUuid>(pTarget, "Layer");
        if (pDoc->IsLayerLoaded(layerGuid))
          return 1.0f;
      }
    }
  }
  return 0;
}

void plGameObjectOnLayerDragDropHandler::OnDrop(const plDragDropInfo* pInfo)
{
  if (plScene2Document* pDoc = plDynamicCast<plScene2Document*>(plDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument)))
  {
    const plDocumentObject* pTarget = pDoc->GetSceneObjectManager()->GetObject(pInfo->m_TargetObject);

    QByteArray encodedData = pInfo->m_pMimeData->data("application/plEditor.ObjectSelection");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    plHybridArray<plDocumentObject*, 32> Dragged;
    stream >> Dragged;

    // We are dragging game objects on another layer => delete objects and recreate in target layer.
    plSceneDocument* pSourceDoc = plDynamicCast<plSceneDocument*>(Dragged[0]->GetDocumentObjectManager()->GetDocument());
    plObjectAccessorBase* pAccessor = pDoc->GetSceneObjectAccessor();
    plUuid layerGuid = pAccessor->Get<plUuid>(pTarget, "Layer");
    plSceneDocument* pTargetDoc = pDoc->GetLayerDocument(layerGuid);

    if (pSourceDoc != pTargetDoc && pTargetDoc)
    {
      const plUuid activeDoc = pDoc->GetActiveLayer();
      {
        // activeDoc should already match pSourceDoc, but just to be sure.
        PL_VERIFY(pDoc->SetActiveLayer(pSourceDoc->GetGuid()).Succeeded(), "Failed to set active document.");

        plResult res = plActionManager::ExecuteAction(nullptr, "Selection.Copy", pSourceDoc, plVariant());
        if (res.Failed())
        {
          plLog::Error("Failed to copy selection while moving objects between layers.");
          return;
        }
        res = plActionManager::ExecuteAction(nullptr, "Selection.Delete", pSourceDoc, plVariant());
        if (res.Failed())
        {
          plLog::Error("Failed to copy selection while moving objects between layers.");
          return;
        }
      }
      {
        PL_VERIFY(pDoc->SetActiveLayer(pTargetDoc->GetGuid()).Succeeded(), "Failed to set active document.");
        plResult res = plActionManager::ExecuteAction(nullptr, "Selection.PasteAtOriginalLocation", pTargetDoc, plVariant());
        if (res.Failed())
        {
          plLog::Error("Failed to paste selection while moving objects between layers.");
          return;
        }
      }
      PL_VERIFY(pDoc->SetActiveLayer(activeDoc).Succeeded(), "Failed to set active document.");
    }
  }
}
