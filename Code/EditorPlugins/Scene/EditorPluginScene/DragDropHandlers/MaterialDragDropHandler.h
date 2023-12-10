#pragma once

#include <EditorFramework/DragDrop/AssetDragDropHandler.h>

class plMaterialDragDropHandler : public plAssetDragDropHandler
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMaterialDragDropHandler, plAssetDragDropHandler);

public:
protected:
  virtual void RequestConfiguration(plDragDropConfig* pConfigToFillOut) override;
  virtual float CanHandle(const plDragDropInfo* pInfo) const override;
  virtual void OnDragBegin(const plDragDropInfo* pInfo) override;
  virtual void OnDragUpdate(const plDragDropInfo* pInfo) override;
  virtual void OnDragCancel() override;
  virtual void OnDrop(const plDragDropInfo* pInfo) override;

  plUuid m_AppliedToComponent;
  plInt32 m_iAppliedToSlot;
};
