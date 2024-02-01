#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class plParticleComponentDragDropHandler : public plComponentDragDropHandler
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleComponentDragDropHandler, plComponentDragDropHandler);

public:
  virtual float CanHandle(const plDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const plDragDropInfo* pInfo) override;
};
