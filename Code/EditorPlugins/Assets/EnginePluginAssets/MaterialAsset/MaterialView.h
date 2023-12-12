#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class plMaterialContext;

class plMaterialViewContext : public PlasmaEngineProcessViewContext
{
public:
  plMaterialViewContext(plMaterialContext* pMaterialContext);
  ~plMaterialViewContext();

  void PositionThumbnailCamera();

protected:
  virtual plViewHandle CreateView() override;

  plMaterialContext* m_pMaterialContext;
};
