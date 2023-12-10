#pragma once

#include <BakingPlugin/Declarations.h>
#include <Foundation/Threading/TaskSystem.h>

struct plBakingSettings;

namespace plBakingInternal
{
  class PLASMA_BAKINGPLUGIN_DLL PlaceProbesTask : public plTask
  {
  public:
    PlaceProbesTask(const plBakingSettings& settings, const plBoundingBox& bounds, plArrayPtr<const Volume> volumes);
    ~PlaceProbesTask();

    virtual void Execute() override;

    plArrayPtr<const plVec3> GetProbePositions() const { return m_ProbePositions; }
    const plVec3& GetGridOrigin() const { return m_vGridOrigin; }
    const plVec3U32& GetProbeCount() const { return m_vProbeCount; }

  private:
    const plBakingSettings& m_Settings;

    plBoundingBox m_Bounds;
    plArrayPtr<const Volume> m_Volumes;

    plVec3 m_vGridOrigin = plVec3::MakeZero();
    plVec3U32 m_vProbeCount = plVec3U32::MakeZero();
    plDynamicArray<plVec3> m_ProbePositions;
  };
} // namespace plBakingInternal
