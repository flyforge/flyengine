#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <Core/World/WorldModule.h>
#include <RendererCore/Declarations.h>

using plProbeTreeSectorResourceHandle = plTypedResourceHandle<class plProbeTreeSectorResource>;

class PLASMA_RENDERERCORE_DLL plBakedProbesWorldModule : public plWorldModule
{
  PLASMA_DECLARE_WORLD_MODULE();
  PLASMA_ADD_DYNAMIC_REFLECTION(plBakedProbesWorldModule, plWorldModule);

public:
  plBakedProbesWorldModule(plWorld* pWorld);
  ~plBakedProbesWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  bool HasProbeData() const;

  struct ProbeIndexData
  {
    static constexpr plUInt32 NumProbes = 8;
    plUInt32 m_probeIndices[NumProbes];
    float m_probeWeights[NumProbes];
  };

  plResult GetProbeIndexData(const plVec3& vGlobalPosition, const plVec3& vNormal, ProbeIndexData& out_probeIndexData) const;

  plAmbientCube<float> GetSkyVisibility(const ProbeIndexData& indexData) const;

private:
  friend class plBakedProbesComponent;

  void SetProbeTreeResourcePrefix(const plHashedString& prefix);

  plProbeTreeSectorResourceHandle m_hProbeTree;
};
