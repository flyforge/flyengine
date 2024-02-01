#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>
#include <Foundation/Types/RangeView.h>

struct plMsgComponentInternalTrigger;

struct plSpawnComponentFlags
{
  using StorageType = plUInt16;

  enum Enum
  {
    None = 0,
    SpawnAtStart = PL_BIT(0),      ///< The component will schedule a spawn once at creation time
    SpawnContinuously = PL_BIT(1), ///< Every time a scheduled spawn was done, a new one is scheduled
    AttachAsChild = PL_BIT(2),     ///< All objects spawned will be attached as children to this node
    SpawnInFlight = PL_BIT(3),     ///< [internal] A spawn trigger message has been posted.

    Default = None
  };

  struct Bits
  {
    StorageType SpawnAtStart : 1;
    StorageType SpawnContinuously : 1;
    StorageType AttachAsChild : 1;
    StorageType SpawnInFlight : 1;
  };
};

PL_DECLARE_FLAGS_OPERATORS(plSpawnComponentFlags);

using plSpawnComponentManager = plComponentManager<class plSpawnComponent, plBlockStorageType::Compact>;

/// \brief Spawns instances of prefabs dynamically at runtime.
///
/// The component may spawn prefabs automatically and also continuously, or it may only spawn objects on-demand
/// when triggered from code.
///
/// It keeps track of when it spawned an object and can ignore spawn requests that come in too early. Thus it can
/// also be used to take care of the logic that certain actions are only allowed every once in a while.
class PL_GAMEENGINE_DLL plSpawnComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plSpawnComponent, plComponent, plSpawnComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // plSpawnComponent

public:
  plSpawnComponent();
  ~plSpawnComponent();

  /// \brief Checks whether the last spawn time was long enough ago that a call to TriggerManualSpawn() would succeed.
  bool CanTriggerManualSpawn() const; // [ scriptable ]

  /// \brief Spawns a new object, unless the minimum spawn delay has not been reached between calls to this function.
  ///
  /// Manual spawns and continuous (scheduled) spawns are independent from each other regarding minimum spawn delays.
  /// If this function is called in too short intervals, it is ignored and false is returned.
  /// Returns true, if an object was spawned.
  bool TriggerManualSpawn(bool bIgnoreSpawnDelay = false, const plVec3& vLocalOffset = plVec3::MakeZero()); // [ scriptable ]

  /// \brief Unless a spawn is already scheduled, this will schedule one within the configured time frame.
  ///
  /// If continuous spawning is enabled, this will kick off the first spawn and then continue indefinitely.
  /// To stop continuously spawning, remove the continuous spawn flag.
  void ScheduleSpawn(); // [ scriptable ]

  /// \brief Sets the prefab resource to use for spawning.
  void SetPrefabFile(const char* szFile); // [ property ]
  const char* GetPrefabFile() const;      // [ property ]

  /// \brief Enables that the component spawns right at creation time. Otherwise it needs to be triggered manually.
  void SetSpawnAtStart(bool b); // [ property ]
  bool GetSpawnAtStart() const; // [ property ]

  /// \brief Enables that once an object was spawned, another spawn action will be scheduled right away.
  void SetSpawnContinuously(bool b); // [ property ]
  bool GetSpawnContinuously() const; // [ property ]

  /// \brief Sets that spawned objects will be attached as child objects to this game object.
  void SetAttachAsChild(bool b); // [ property ]
  bool GetAttachAsChild() const; // [ property ]

  /// \brief Sets the prefab resource to spawn.
  void SetPrefab(const plPrefabResourceHandle& hPrefab);
  PL_ALWAYS_INLINE const plPrefabResourceHandle& GetPrefab() const { return m_hPrefab; }

  /// The minimum delay between spawning objects. This is also enforced for manually spawning things.
  plTime m_MinDelay; // [ property ]

  /// For scheduled spawns (continuous / at start) this is an additional random range on top of the minimum spawn delay.
  plTime m_DelayRange; // [ property ]

  /// The spawned object's orientation may deviate by this amount around the X axis. 180° is completely random orientation.
  plAngle m_MaxDeviation; // [ property ]

  const plRangeView<const char*, plUInt32> GetParameters() const;   // [ property ] (exposed parameter)
  void SetParameter(const char* szKey, const plVariant& value);     // [ property ] (exposed parameter)
  void RemoveParameter(const char* szKey);                          // [ property ] (exposed parameter)
  bool GetParameter(const char* szKey, plVariant& out_value) const; // [ property ] (exposed parameter)

  /// Key/value pairs of parameters to pass to the prefab instantiation.
  plArrayMap<plHashedString, plVariant> m_Parameters;

protected:
  plBitflags<plSpawnComponentFlags> m_SpawnFlags;

  virtual void DoSpawn(const plTransform& tLocalSpawn);
  bool SpawnOnce(const plVec3& vLocalOffset);
  void OnTriggered(plMsgComponentInternalTrigger& msg);

  plTime m_LastManualSpawn;
  plPrefabResourceHandle m_hPrefab;
};
