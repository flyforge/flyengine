#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class plDecalContext;

class plDecalViewContext : public plEngineProcessViewContext
{
public:
  plDecalViewContext(plDecalContext* pDecalContext);
  ~plDecalViewContext();

protected:
  virtual plViewHandle CreateView() override;

  plDecalContext* m_pDecalContext;
};
