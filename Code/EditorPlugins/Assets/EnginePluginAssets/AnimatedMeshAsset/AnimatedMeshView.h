#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class plAnimatedMeshContext;

class plAnimatedMeshViewContext : public plEngineProcessViewContext
{
public:
  plAnimatedMeshViewContext(plAnimatedMeshContext* pMeshContext);
  ~plAnimatedMeshViewContext();

  bool UpdateThumbnailCamera(const plBoundingBoxSphere& bounds);

protected:
  virtual plViewHandle CreateView() override;
  virtual void SetCamera(const plViewRedrawMsgToEngine* pMsg) override;

  plAnimatedMeshContext* m_pContext = nullptr;
};
