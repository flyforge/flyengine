#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class plTypeScriptComponentDragDropHandler : public plComponentDragDropHandler
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTypeScriptComponentDragDropHandler, plComponentDragDropHandler);

public:
  virtual float CanHandle(const plDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const plDragDropInfo* pInfo) override;
};
