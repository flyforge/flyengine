#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

class plMeshComponentDragDropHandler : public plComponentDragDropHandler
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMeshComponentDragDropHandler, plComponentDragDropHandler);

public:
  virtual float CanHandle(const plDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const plDragDropInfo* pInfo) override;
};

//////////////////////////////////////////////////////////////////////////

class plAnimatedMeshComponentDragDropHandler : public plComponentDragDropHandler
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAnimatedMeshComponentDragDropHandler, plComponentDragDropHandler);

public:
  virtual float CanHandle(const plDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const plDragDropInfo* pInfo) override;
};
