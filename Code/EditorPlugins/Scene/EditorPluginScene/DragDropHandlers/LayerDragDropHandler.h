#pragma once

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>

/// \brief Base class for drag and drop handler that drop on a plSceneLayer.
class plLayerDragDropHandler : public plDragDropHandler
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLayerDragDropHandler, plDragDropHandler);

public:
  virtual void OnDragBegin(const plDragDropInfo* pInfo) override {}
  virtual void OnDragUpdate(const plDragDropInfo* pInfo) override {}
  virtual void OnDragCancel() override {}

protected:
  const plRTTI* GetCommonBaseType(const plDragDropInfo* pInfo) const;
};

class plLayerOnLayerDragDropHandler : public plLayerDragDropHandler
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLayerOnLayerDragDropHandler, plLayerDragDropHandler);

public:
  virtual float CanHandle(const plDragDropInfo* pInfo) const override;
  virtual void OnDrop(const plDragDropInfo* pInfo) override;
};

class plGameObjectOnLayerDragDropHandler : public plLayerDragDropHandler
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGameObjectOnLayerDragDropHandler, plLayerDragDropHandler);

public:
  virtual float CanHandle(const plDragDropInfo* pInfo) const override;
  virtual void OnDrop(const plDragDropInfo* pInfo) override;
};
