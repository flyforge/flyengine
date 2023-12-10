#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class plPrefabComponentDragDropHandler : public plComponentDragDropHandler
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPrefabComponentDragDropHandler, plComponentDragDropHandler);

protected:
  virtual float CanHandle(const plDragDropInfo* pInfo) const override;
  virtual void OnDragBegin(const plDragDropInfo* pInfo) override;
  virtual void OnDragUpdate(const plDragDropInfo* pInfo) override;

private:
  void CreatePrefab(const plVec3& vPosition, const plUuid& AssetGuid, plUuid parent, plInt32 iInsertChildIndex);
};
