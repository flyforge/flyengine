#pragma once

#include <EditorFramework/DragDrop/AssetDragDropHandler.h>

class plDocument;
class plDragDropInfo;

class PL_EDITORFRAMEWORK_DLL plComponentDragDropHandler : public plAssetDragDropHandler
{
  PL_ADD_DYNAMIC_REFLECTION(plComponentDragDropHandler, plAssetDragDropHandler);

protected:
  void CreateDropObject(const plVec3& vPosition, const char* szType, const char* szProperty, const plVariant& value, plUuid parent, plInt32 iInsertChildIndex);

  void AttachComponentToObject(const char* szType, const char* szProperty, const plVariant& value, plUuid ObjectGuid);

  void MoveObjectToPosition(const plUuid& guid, const plVec3& vPosition, const plQuat& qRotation);

  void MoveDraggedObjectsToPosition(plVec3 vPosition, bool bAllowSnap, const plVec3& normal);

  void SelectCreatedObjects();

  void BeginTemporaryCommands();

  void EndTemporaryCommands();

  void CancelTemporaryCommands();

  plDocument* m_pDocument;
  plHybridArray<plUuid, 16> m_DraggedObjects;

  virtual void OnDragBegin(const plDragDropInfo* pInfo) override;

  virtual void OnDragUpdate(const plDragDropInfo* pInfo) override;

  virtual void OnDragCancel() override;

  virtual void OnDrop(const plDragDropInfo* pInfo) override;

  virtual float CanHandle(const plDragDropInfo* pInfo) const override;

  plVec3 m_vAlignAxisWithNormal = plVec3::MakeZero();
};
