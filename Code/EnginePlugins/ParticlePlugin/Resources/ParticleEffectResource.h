#pragma once

#include <Core/ResourceManager/Resource.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <RendererCore/Declarations.h>

using plParticleEffectResourceHandle = plTypedResourceHandle<class plParticleEffectResource>;

struct PL_PARTICLEPLUGIN_DLL plParticleEffectResourceDescriptor
{
  virtual void Save(plStreamWriter& inout_stream) const;
  virtual void Load(plStreamReader& inout_stream);

  plParticleEffectDescriptor m_Effect;
};

class PL_PARTICLEPLUGIN_DLL plParticleEffectResource final : public plResource
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleEffectResource, plResource);
  PL_RESOURCE_DECLARE_COMMON_CODE(plParticleEffectResource);
  PL_RESOURCE_DECLARE_CREATEABLE(plParticleEffectResource, plParticleEffectResourceDescriptor);

public:
  plParticleEffectResource();
  ~plParticleEffectResource();

  const plParticleEffectResourceDescriptor& GetDescriptor() { return m_Desc; }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  plParticleEffectResourceDescriptor m_Desc;
};
