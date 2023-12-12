#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class plTextureCubeContext;

class plTextureCubeViewContext : public PlasmaEngineProcessViewContext
{
public:
  plTextureCubeViewContext(plTextureCubeContext* pMaterialContext);
  ~plTextureCubeViewContext();

protected:
  virtual plViewHandle CreateView() override;
  virtual void SetCamera(const plViewRedrawMsgToEngine* pMsg) override;

  plTextureCubeContext* m_pTextureContext;
};
