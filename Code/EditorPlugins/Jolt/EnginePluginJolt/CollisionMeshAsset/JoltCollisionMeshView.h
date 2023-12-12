#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class plJoltCollisionMeshContext;

class plJoltCollisionMeshViewContext : public PlasmaEngineProcessViewContext
{
public:
  plJoltCollisionMeshViewContext(plJoltCollisionMeshContext* pMeshContext);
  ~plJoltCollisionMeshViewContext();

  bool UpdateThumbnailCamera(const plBoundingBoxSphere& bounds);

protected:
  virtual plViewHandle CreateView() override;
  virtual void SetCamera(const plViewRedrawMsgToEngine* pMsg) override;

  plJoltCollisionMeshContext* m_pContext = nullptr;
};
