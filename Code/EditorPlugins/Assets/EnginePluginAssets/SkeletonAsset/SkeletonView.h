#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class plSkeletonContext;

class plSkeletonViewContext : public plEngineProcessViewContext
{
public:
  plSkeletonViewContext(plSkeletonContext* pContext);
  ~plSkeletonViewContext();

  bool UpdateThumbnailCamera(const plBoundingBoxSphere& bounds);

  virtual void Redraw(bool bRenderEditorGizmos) override;

protected:
  virtual plViewHandle CreateView() override;
  virtual void SetCamera(const plViewRedrawMsgToEngine* pMsg) override;

  virtual void HandleViewMessage(const plEditorEngineViewMsg* pMsg) override;

  void PickObjectAt(plUInt16 x, plUInt16 y);

  plSkeletonContext* m_pContext = nullptr;
};
