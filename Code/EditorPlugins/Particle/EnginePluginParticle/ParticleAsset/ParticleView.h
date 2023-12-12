#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class plParticleContext;

class plParticleViewContext : public PlasmaEngineProcessViewContext
{
public:
  plParticleViewContext(plParticleContext* pParticleContext);
  ~plParticleViewContext();

  void PositionThumbnailCamera(const plBoundingBoxSphere& bounds);

protected:
  virtual plViewHandle CreateView() override;

  plParticleContext* m_pParticleContext;
};
