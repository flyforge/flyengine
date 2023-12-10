#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class plKrautTreeComponentDragDropHandler : public plComponentDragDropHandler
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plKrautTreeComponentDragDropHandler, plComponentDragDropHandler);

public:
  virtual float CanHandle(const plDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const plDragDropInfo* pInfo) override;
};
