#pragma once

#include <BakingPlugin/BakingPluginDLL.h>
#include <Foundation/Threading/TaskSystem.h>
#include <RendererCore/BakedProbes/BakingUtils.h>

struct plBakingSettings;
class plTracerInterface;

namespace plBakingInternal
{
  class PLASMA_BAKINGPLUGIN_DLL SkyVisibilityTask : public plTask
  {
  public:
    SkyVisibilityTask(const plBakingSettings& settings, plTracerInterface& tracer, plArrayPtr<const plVec3> probePositions);
    ~SkyVisibilityTask();

    virtual void Execute() override;

    plArrayPtr<const plCompressedSkyVisibility> GetSkyVisibility() const { return m_SkyVisibility; }

  private:
    const plBakingSettings& m_Settings;

    plTracerInterface& m_Tracer;
    plArrayPtr<const plVec3> m_ProbePositions;

    plDynamicArray<plCompressedSkyVisibility> m_SkyVisibility;
  };
} // namespace plBakingInternal
