#pragma once

#include <BakingPlugin/Tracer/TracerInterface.h>
#include <Foundation/Types/UniquePtr.h>

class PLASMA_BAKINGPLUGIN_DLL plTracerEmbree : public plTracerInterface
{
public:
  plTracerEmbree();
  ~plTracerEmbree();

  virtual plResult BuildScene(const plBakingScene& scene) override;

  virtual void TraceRays(plArrayPtr<const Ray> rays, plArrayPtr<Hit> hits) override;

private:
  struct Data;

  plUniquePtr<Data> m_pData;
};
