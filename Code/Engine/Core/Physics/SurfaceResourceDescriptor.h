#pragma once

#include <Core/CoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/Variant.h>

using plSurfaceResourceHandle = plTypedResourceHandle<class plSurfaceResource>;
using plPrefabResourceHandle = plTypedResourceHandle<class plPrefabResource>;


struct plSurfaceInteractionAlignment
{
  using StorageType = plUInt8;

  enum Enum
  {
    SurfaceNormal,
    IncidentDirection,
    ReflectedDirection,
    ReverseSurfaceNormal,
    ReverseIncidentDirection,
    ReverseReflectedDirection,

    Default = SurfaceNormal
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plSurfaceInteractionAlignment);


struct PL_CORE_DLL plSurfaceInteraction
{
  void SetPrefab(const char* szPrefab);
  const char* GetPrefab() const;

  plString m_sInteractionType;

  plPrefabResourceHandle m_hPrefab;
  plEnum<plSurfaceInteractionAlignment> m_Alignment;
  plAngle m_Deviation;
  float m_fImpulseThreshold = 0.0f;
  float m_fImpulseScale = 1.0f;

  const plRangeView<const char*, plUInt32> GetParameters() const;   // [ property ] (exposed parameter)
  void SetParameter(const char* szKey, const plVariant& value);     // [ property ] (exposed parameter)
  void RemoveParameter(const char* szKey);                          // [ property ] (exposed parameter)
  bool GetParameter(const char* szKey, plVariant& out_value) const; // [ property ] (exposed parameter)

  plArrayMap<plHashedString, plVariant> m_Parameters;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plSurfaceInteraction);

struct PL_CORE_DLL plSurfaceResourceDescriptor : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plSurfaceResourceDescriptor, plReflectedClass);

public:
  void Load(plStreamReader& inout_stream);
  void Save(plStreamWriter& inout_stream) const;

  void SetBaseSurfaceFile(const char* szFile);
  const char* GetBaseSurfaceFile() const;

  void SetCollisionInteraction(const char* szName);
  const char* GetCollisionInteraction() const;

  void SetSlideReactionPrefabFile(const char* szFile);
  const char* GetSlideReactionPrefabFile() const;

  void SetRollReactionPrefabFile(const char* szFile);
  const char* GetRollReactionPrefabFile() const;

  plSurfaceResourceHandle m_hBaseSurface;
  float m_fPhysicsRestitution;
  float m_fPhysicsFrictionStatic;
  float m_fPhysicsFrictionDynamic;
  float m_fSoundObstruction;
  plHashedString m_sOnCollideInteraction;
  plHashedString m_sSlideInteractionPrefab;
  plHashedString m_sRollInteractionPrefab;
  plInt8 m_iGroundType = -1; ///< What kind of ground this is for navigation purposes. Ground type properties need to be specified elsewhere, this is just a number.

  plHybridArray<plSurfaceInteraction, 16> m_Interactions;
};
