#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class plTextureContext;

class plTextureViewContext : public plEngineProcessViewContext
{
public:
  plTextureViewContext(plTextureContext* pMaterialContext);
  ~plTextureViewContext();

protected:
  virtual plViewHandle CreateView() override;
  virtual void SetCamera(const plViewRedrawMsgToEngine* pMsg) override;

  plTextureContext* m_pTextureContext;
};
