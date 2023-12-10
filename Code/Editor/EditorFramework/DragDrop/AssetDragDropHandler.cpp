#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DragDrop/AssetDragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>


PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAssetDragDropHandler, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

bool plAssetDragDropHandler::IsAssetType(const plDragDropInfo* pInfo) const
{
  return pInfo->m_pMimeData->hasFormat("application/plEditor.AssetGuid");
}

plString plAssetDragDropHandler::GetAssetGuidString(const plDragDropInfo* pInfo) const
{
  QByteArray ba = pInfo->m_pMimeData->data("application/plEditor.AssetGuid");
  QDataStream stream(&ba, QIODevice::ReadOnly);

  plHybridArray<QString, 1> guids;
  stream >> guids;

  if (guids.GetCount() > 1)
  {
    plLog::Warning("Dragging more than one asset type is currently not supported");
  }

  return guids[0].toUtf8().data();
}

plString plAssetDragDropHandler::GetAssetsDocumentTypeName(const plUuid& assetTypeGuid) const
{
  return plAssetCurator::GetSingleton()->GetSubAsset(assetTypeGuid)->m_Data.m_sSubAssetsDocumentTypeName.GetData();
}

bool plAssetDragDropHandler::IsSpecificAssetType(const plDragDropInfo* pInfo, const char* szType) const
{
  if (!IsAssetType(pInfo))
    return false;

  const plUuid guid = GetAssetGuid(pInfo);

  return GetAssetsDocumentTypeName(guid) == szType;
}
