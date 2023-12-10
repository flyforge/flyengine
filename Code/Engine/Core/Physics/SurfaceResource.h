#pragma once

#include <Core/CoreDLL.h>

#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/World/Declarations.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>

class plWorld;
class plUuid;

struct plSurfaceResourceEvent
{
  enum class Type
  {
    Created,
    Destroyed
  };

  Type m_Type;
  plSurfaceResource* m_pSurface;
};

class PLASMA_CORE_DLL plSurfaceResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSurfaceResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plSurfaceResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plSurfaceResource, plSurfaceResourceDescriptor);

public:
  plSurfaceResource();
  ~plSurfaceResource();

  const plSurfaceResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

  static plEvent<const plSurfaceResourceEvent&, plMutex> s_Events;

  void* m_pPhysicsMaterialPhysX = nullptr;
  void* m_pPhysicsMaterialJolt = nullptr;

  /// \brief Spawns the prefab that was defined for the given interaction at the given position and using the configured orientation.
  /// Returns false, if the interaction type was not defined in this surface or any of its base surfaces
  bool InteractWithSurface(plWorld* pWorld, plGameObjectHandle hObject, const plVec3& vPosition, const plVec3& vSurfaceNormal, const plVec3& vIncomingDirection, const plTempHashedString& sInteraction, const plUInt16* pOverrideTeamID, float fImpulseSqr = 0.0f) const;

  bool IsBasedOn(const plSurfaceResource* pThisOrBaseSurface) const;

  bool IsBasedOn(const plSurfaceResourceHandle hThisOrBaseSurface) const;

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  static const plSurfaceInteraction* FindInteraction(const plSurfaceResource* pCurSurf, plUInt64 uiHash, float fImpulseSqr, float& out_fImpulseParamValue);

  plSurfaceResourceDescriptor m_Descriptor;

  struct SurfInt
  {
    plUInt64 m_uiInteractionTypeHash = 0;
    const plSurfaceInteraction* m_pInteraction;
  };

  plDynamicArray<SurfInt> m_Interactions;
};
