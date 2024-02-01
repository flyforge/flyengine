#pragma once

#include <BakingPlugin/BakingPluginDLL.h>

class plBakingScene;

class PL_BAKINGPLUGIN_DLL plTracerInterface
{
public:
  virtual plResult BuildScene(const plBakingScene& scene) = 0;

  struct Ray
  {
    PL_DECLARE_POD_TYPE();

    plVec3 m_vStartPos;
    plVec3 m_vDir;
    float m_fDistance;
  };

  struct Hit
  {
    PL_DECLARE_POD_TYPE();

    plVec3 m_vPosition;
    plVec3 m_vNormal;
    float m_fDistance;
  };

  virtual void TraceRays(plArrayPtr<const Ray> rays, plArrayPtr<Hit> hits) = 0;
};
