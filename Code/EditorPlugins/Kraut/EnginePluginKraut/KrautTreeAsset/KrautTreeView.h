#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class plKrautTreeContext;

class plKrautTreeViewContext : public plEngineProcessViewContext
{
public:
  plKrautTreeViewContext(plKrautTreeContext* pKrautTreeContext);
  ~plKrautTreeViewContext();

  bool UpdateThumbnailCamera(const plBoundingBoxSphere& bounds);

protected:
  virtual plViewHandle CreateView() override;
  virtual void SetCamera(const plViewRedrawMsgToEngine* pMsg) override;

  plKrautTreeContext* m_pKrautTreeContext;
};
