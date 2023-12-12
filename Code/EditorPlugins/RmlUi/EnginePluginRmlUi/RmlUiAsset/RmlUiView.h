#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class plRmlUiDocumentContext;

class plRmlUiViewContext : public PlasmaEngineProcessViewContext
{
public:
  plRmlUiViewContext(plRmlUiDocumentContext* pRmlUiContext);
  ~plRmlUiViewContext();

  bool UpdateThumbnailCamera(const plBoundingBoxSphere& bounds);

protected:
  virtual plViewHandle CreateView() override;
  virtual void SetCamera(const plViewRedrawMsgToEngine* pMsg) override;

  plRmlUiDocumentContext* m_pRmlUiContext;
};
