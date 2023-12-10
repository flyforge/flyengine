#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>
#include <EditorPluginProcGen/EditorPluginProcGenDLL.h>

class PLASMA_EDITORPLUGINPROCGEN_DLL plProcPlacementComponentDragDropHandler : public plComponentDragDropHandler
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProcPlacementComponentDragDropHandler, plComponentDragDropHandler);

public:
  virtual float CanHandle(const plDragDropInfo* pInfo) const override;

  virtual void OnDragBegin(const plDragDropInfo* pInfo) override;
};
