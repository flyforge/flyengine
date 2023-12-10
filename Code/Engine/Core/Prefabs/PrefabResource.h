#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Reflection/PropertyPath.h>

using plPrefabResourceHandle = plTypedResourceHandle<class plPrefabResource>;

struct PLASMA_CORE_DLL plPrefabResourceDescriptor
{
};

struct PLASMA_CORE_DLL plExposedPrefabParameterDesc
{
  plHashedString m_sExposeName;
  plUInt32 m_uiWorldReaderChildObject : 1; // 0 -> use root object array, 1 -> use child object array
  plUInt32 m_uiWorldReaderObjectIndex : 31;
  plHashedString m_sComponentType;     // plRTTI type name to identify which component is meant, empty string -> affects game object
  plHashedString m_sProperty;          // which property to override
  plPropertyPath m_CachedPropertyPath; // cached plPropertyPath to apply a value to the specified property

  void Save(plStreamWriter& inout_stream) const;
  void Load(plStreamReader& inout_stream);
};

class PLASMA_CORE_DLL plPrefabResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPrefabResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plPrefabResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plPrefabResource, plPrefabResourceDescriptor);

public:
  plPrefabResource();

  enum class InstantiateResult : plUInt8
  {
    Success,
    NotYetLoaded,
    Error,
  };

  /// \brief Helper function to instantiate a prefab without having to deal with resource acquisition.
  static plPrefabResource::InstantiateResult InstantiatePrefab(const plPrefabResourceHandle& hPrefab, bool bBlockTillLoaded, plWorld& ref_world, const plTransform& rootTransform, plPrefabInstantiationOptions options = {}, const plArrayMap<plHashedString, plVariant>* pExposedParamValues = nullptr);

  /// \brief Creates an instance of this prefab in the given world.
  void InstantiatePrefab(plWorld& ref_world, const plTransform& rootTransform, plPrefabInstantiationOptions options, const plArrayMap<plHashedString, plVariant>* pExposedParamValues = nullptr);

  void ApplyExposedParameterValues(const plArrayMap<plHashedString, plVariant>* pExposedParamValues, const plDynamicArray<plGameObject*>& createdChildObjects, const plDynamicArray<plGameObject*>& createdRootObjects) const;

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  plUInt32 FindFirstParamWithName(plUInt64 uiNameHash) const;

  plWorldReader m_WorldReader;
  plDynamicArray<plExposedPrefabParameterDesc> m_PrefabParamDescs;
};
