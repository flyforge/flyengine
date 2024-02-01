#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>

using plColorGradientResourceHandle = plTypedResourceHandle<class plColorGradientResource>;

class PL_PARTICLEPLUGIN_DLL plParticleInitializerFactory_RandomColor final : public plParticleInitializerFactory
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleInitializerFactory_RandomColor, plParticleInitializerFactory);

public:
  virtual const plRTTI* GetInitializerType() const override;
  virtual void CopyInitializerProperties(plParticleInitializer* pInitializer, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& inout_stream) const override;
  virtual void Load(plStreamReader& inout_stream) override;

  void SetColorGradient(const plColorGradientResourceHandle& hResource) { m_hGradient = hResource; }
  PL_ALWAYS_INLINE const plColorGradientResourceHandle& GetColorGradient() const { return m_hGradient; }

  void SetColorGradientFile(const char* szFile);
  const char* GetColorGradientFile() const;

  plColor m_Color1;
  plColor m_Color2;

private:
  plColorGradientResourceHandle m_hGradient;
};


class PL_PARTICLEPLUGIN_DLL plParticleInitializer_RandomColor final : public plParticleInitializer
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleInitializer_RandomColor, plParticleInitializer);

public:
  plColor m_Color1;
  plColor m_Color2;

  plColorGradientResourceHandle m_hGradient;


  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;

  plProcessingStream* m_pStreamColor;
};
