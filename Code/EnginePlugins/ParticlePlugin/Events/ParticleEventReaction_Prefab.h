#pragma once

#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/SharedPtr.h>
#include <ParticlePlugin/Events/ParticleEventReaction.h>

using plPrefabResourceHandle = plTypedResourceHandle<class plPrefabResource>;

class PL_PARTICLEPLUGIN_DLL plParticleEventReactionFactory_Prefab final : public plParticleEventReactionFactory
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleEventReactionFactory_Prefab, plParticleEventReactionFactory);

public:
  plParticleEventReactionFactory_Prefab();

  virtual const plRTTI* GetEventReactionType() const override;
  virtual void CopyReactionProperties(plParticleEventReaction* pObject, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& inout_stream) const override;
  virtual void Load(plStreamReader& inout_stream) override;

  plString m_sPrefab;
  plEnum<plSurfaceInteractionAlignment> m_Alignment;

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
public:
  // const plRangeView<const char*, plUInt32> GetParameters() const;
  // void SetParameter(const char* szKey, const plVariant& value);
  // void RemoveParameter(const char* szKey);
  // bool GetParameter(const char* szKey, plVariant& out_value) const;

private:
  // plSharedPtr<plParticlePrefabParameters> m_Parameters;
};

class PL_PARTICLEPLUGIN_DLL plParticleEventReaction_Prefab final : public plParticleEventReaction
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleEventReaction_Prefab, plParticleEventReaction);

public:
  plParticleEventReaction_Prefab();
  ~plParticleEventReaction_Prefab();

  plPrefabResourceHandle m_hPrefab;
  plEnum<plSurfaceInteractionAlignment> m_Alignment;

  // plSharedPtr<plParticlePrefabParameters> m_Parameters;

protected:
  virtual void ProcessEvent(const plParticleEvent& e) override;
};
