#pragma once

#include <Core/Graphics/AmbientCubeBasis.h>
#include <Core/ResourceManager/Resource.h>
#include <RendererCore/BakedProbes/BakingUtils.h>

using plProbeTreeSectorResourceHandle = plTypedResourceHandle<class plProbeTreeSectorResource>;

struct PLASMA_RENDERERCORE_DLL plProbeTreeSectorResourceDescriptor
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plProbeTreeSectorResourceDescriptor);

  plProbeTreeSectorResourceDescriptor();
  ~plProbeTreeSectorResourceDescriptor();
  plProbeTreeSectorResourceDescriptor& operator=(plProbeTreeSectorResourceDescriptor&& other);

  plVec3 m_vGridOrigin;
  plVec3 m_vProbeSpacing;
  plVec3U32 m_vProbeCount;

  plDynamicArray<plVec3> m_ProbePositions;
  plDynamicArray<plCompressedSkyVisibility> m_SkyVisibility;

  void Clear();
  plUInt64 GetHeapMemoryUsage() const;

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);
};

class PLASMA_RENDERERCORE_DLL plProbeTreeSectorResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProbeTreeSectorResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plProbeTreeSectorResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plProbeTreeSectorResource, plProbeTreeSectorResourceDescriptor);

public:
  plProbeTreeSectorResource();
  ~plProbeTreeSectorResource();

  const plVec3& GetGridOrigin() const { return m_Desc.m_vGridOrigin; }
  const plVec3& GetProbeSpacing() const { return m_Desc.m_vProbeSpacing; }
  const plVec3U32& GetProbeCount() const { return m_Desc.m_vProbeCount; }

  plArrayPtr<const plVec3> GetProbePositions() const { return m_Desc.m_ProbePositions; }
  plArrayPtr<const plCompressedSkyVisibility> GetSkyVisibility() const { return m_Desc.m_SkyVisibility; }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  plProbeTreeSectorResourceDescriptor m_Desc;
};
