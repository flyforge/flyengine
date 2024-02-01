#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class plMeshContext;

class plMeshViewContext : public plEngineProcessViewContext
{
public:
  plMeshViewContext(plMeshContext* pMeshContext);
  ~plMeshViewContext();

  bool UpdateThumbnailCamera(const plBoundingBoxSphere& bounds);

protected:
  virtual plViewHandle CreateView() override;
  virtual void SetCamera(const plViewRedrawMsgToEngine* pMsg) override;

  plMeshContext* m_pContext = nullptr;
};
