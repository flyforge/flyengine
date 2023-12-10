#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

struct plMsgExtractRenderData;

enum plParticleTypeSortingKey
{
  Distortion, // samples the back-buffer, so doing this later would overwrite their result
  Opaque,
  BlendedBackground,
  Additive,
  BlendAdd,
  Blended,
  BlendedForeground,
};

class PLASMA_PARTICLEPLUGIN_DLL plParticleTypeFactory : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleTypeFactory, plReflectedClass);

public:
  virtual const plRTTI* GetTypeType() const = 0;
  virtual void CopyTypeProperties(plParticleType* pObject, bool bFirstTime) const = 0;

  plParticleType* CreateType(plParticleSystemInstance* pOwner) const;

  virtual void QueryFinalizerDependencies(plSet<const plRTTI*>& inout_FinalizerDeps) const {}

  virtual void Save(plStreamWriter& stream) const = 0;
  virtual void Load(plStreamReader& stream) = 0;
};

class PLASMA_PARTICLEPLUGIN_DLL plParticleType : public plParticleModule
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleType, plParticleModule);

  friend class plParticleSystemInstance;

public:
  virtual float GetMaxParticleRadius(float fParticleSize) const { return fParticleSize * 0.5f; }

  virtual void ExtractTypeRenderData(plMsgExtractRenderData& msg, const plTransform& instanceTransform) const = 0;

protected:
  plParticleType();

  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override {}
  virtual void StepParticleSystem(const plTime& tDiff, plUInt32 uiNumNewParticles) { m_TimeDiff = tDiff; }

  static plUInt32 ComputeSortingKey(plParticleTypeRenderMode::Enum mode, plUInt32 uiTextureHash);

  plTime m_TimeDiff;
  mutable plUInt64 m_uiLastExtractedFrame;
};
