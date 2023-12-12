#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class plAnimationClipContext;

class plAnimationClipViewContext : public PlasmaEngineProcessViewContext
{
public:
  plAnimationClipViewContext(plAnimationClipContext* pContext);
  ~plAnimationClipViewContext();

  bool UpdateThumbnailCamera(const plBoundingBoxSphere& bounds);

protected:
  virtual plViewHandle CreateView() override;
  virtual void SetCamera(const plViewRedrawMsgToEngine* pMsg) override;

  plAnimationClipContext* m_pContext = nullptr;
};
