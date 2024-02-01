#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class plJoltCollisionMeshComponentDragDropHandler : public plComponentDragDropHandler
{
  PL_ADD_DYNAMIC_REFLECTION(plJoltCollisionMeshComponentDragDropHandler, plComponentDragDropHandler);

public:
  virtual float CanHandle(const plDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const plDragDropInfo* pInfo) override;
};
