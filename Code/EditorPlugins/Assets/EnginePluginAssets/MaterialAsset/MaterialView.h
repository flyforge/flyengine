#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class plMaterialContext;

class plMaterialViewContext : public plEngineProcessViewContext
{
public:
  plMaterialViewContext(plMaterialContext* pMaterialContext);
  ~plMaterialViewContext();

  void PositionThumbnailCamera();

protected:
  virtual plViewHandle CreateView() override;

  plMaterialContext* m_pMaterialContext;
};
