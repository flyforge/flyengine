#pragma once

#include <Core/ResourceManager/Resource.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <RendererCore/Declarations.h>

using plParticleEffectResourceHandle = plTypedResourceHandle<class plParticleEffectResource>;

struct PLASMA_PARTICLEPLUGIN_DLL plParticleEffectResourceDescriptor
{
  virtual void Save(plStreamWriter& stream) const;
  virtual void Load(plStreamReader& stream);

  plParticleEffectDescriptor m_Effect;
};

class PLASMA_PARTICLEPLUGIN_DLL plParticleEffectResource final : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleEffectResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plParticleEffectResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plParticleEffectResource, plParticleEffectResourceDescriptor);

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
