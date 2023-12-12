#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class plDecalContext;

class plDecalViewContext : public PlasmaEngineProcessViewContext
{
public:
  plDecalViewContext(plDecalContext* pDecalContext);
  ~plDecalViewContext();

protected:
  virtual plViewHandle CreateView() override;

  plDecalContext* m_pDecalContext;
};
