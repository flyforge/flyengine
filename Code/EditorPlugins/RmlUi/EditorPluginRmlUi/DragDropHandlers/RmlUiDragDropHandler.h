#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class plRmlUiComponentDragDropHandler : public plComponentDragDropHandler
{
  PL_ADD_DYNAMIC_REFLECTION(plRmlUiComponentDragDropHandler, plComponentDragDropHandler);

public:
  float CanHandle(const plDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const plDragDropInfo* pInfo) override;
};
