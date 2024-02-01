#pragma once

#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/RangeView.h>

class plPrefabReferenceComponent;

class PL_CORE_DLL plPrefabReferenceComponentManager : public plComponentManager<plPrefabReferenceComponent, plBlockStorageType::Compact>
{
public:
  plPrefabReferenceComponentManager(plWorld* pWorld);
  ~plPrefabReferenceComponentManager();

  virtual void Initialize() override;

  void Update(const plWorldModule::UpdateContext& context);
  void AddToUpdateList(plPrefabReferenceComponent* pComponent);

private:
  void ResourceEventHandler(const plResourceEvent& e);

  plDeque<plComponentHandle> m_ComponentsToUpdate;
};

/// \brief The central component to instantiate prefabs.
///
/// This component instantiates a prefab and attaches the instantiated objects as children to this object.
/// The component is able to remove and recreate instantiated objects, which is needed at editing time.
/// Whenever the prefab resource changes, this component re-creates the instance.
///
/// It also holds prefab parameters, which are passed through during instantiation.
/// For that it also implements remapping of game object references, so that they can be passed into prefabs during instantiation.
class PL_CORE_DLL plPrefabReferenceComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plPrefabReferenceComponent, plComponent, plPrefabReferenceComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // plPrefabReferenceComponent

public:
  plPrefabReferenceComponent();
  ~plPrefabReferenceComponent();

  void SetPrefabFile(const char* szFile); // [ property ]
  const char* GetPrefabFile() const;      // [ property ]

  void SetPrefab(const plPrefabResourceHandle& hPrefab);                                 // [ property ]
  PL_ALWAYS_INLINE const plPrefabResourceHandle& GetPrefab() const { return m_hPrefab; } // [ property ]

  const plRangeView<const char*, plUInt32> GetParameters() const;   // [ property ] (exposed parameter)
  void SetParameter(const char* szKey, const plVariant& value);     // [ property ] (exposed parameter)
  void RemoveParameter(const char* szKey);                          // [ property ] (exposed parameter)
  bool GetParameter(const char* szKey, plVariant& out_value) const; // [ property ] (exposed parameter)

  static void SerializePrefabParameters(const plWorld& world, plWorldWriter& inout_stream, plArrayMap<plHashedString, plVariant> parameters);
  static void DeserializePrefabParameters(plArrayMap<plHashedString, plVariant>& out_parameters, plWorldReader& inout_stream);

private:
  void InstantiatePrefab();
  void ClearPreviousInstances();

  plPrefabResourceHandle m_hPrefab;
  plArrayMap<plHashedString, plVariant> m_Parameters;
  bool m_bInUpdateList = false;
};
