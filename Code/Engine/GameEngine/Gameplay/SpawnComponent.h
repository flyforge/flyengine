#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>
#include <Foundation/Types/RangeView.h>

struct plMsgComponentInternalTrigger;

struct plSpawnComponentFlags
{
  typedef plUInt16 StorageType;

  enum Enum
  {
    None = 0,
    SpawnAtStart = PLASMA_BIT(0),      ///< The component will schedule a spawn once at creation time
    SpawnContinuously = PLASMA_BIT(1), ///< Every time a scheduled spawn was done, a new one is scheduled
    AttachAsChild = PLASMA_BIT(2),     ///< All objects spawned will be attached as children to this node
    SpawnInFlight = PLASMA_BIT(3),     ///< [internal] A spawn trigger message has been posted.

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

PLASMA_DECLARE_FLAGS_OPERATORS(plSpawnComponentFlags);

typedef plComponentManager<class plSpawnComponent, plBlockStorageType::Compact> plSpawnComponentManager;

class PLASMA_GAMEENGINE_DLL plSpawnComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plSpawnComponent, plComponent, plSpawnComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

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
  bool TriggerManualSpawn(bool bIgnoreSpawnDelay = false, const plVec3& vLocalOffset = plVec3::ZeroVector()); // [ scriptable ]

  /// \brief Unless a spawn is already scheduled, this will schedule one within the configured time frame.
  ///
  /// If continuous spawning is enabled, this will kick off the first spawn and then continue indefinitely.
  /// To stop continuously spawning, remove the continuous spawn flag.
  void ScheduleSpawn(); // [ scriptable ]

  void SetPrefabFile(const char* szFile); // [ property ]
  const char* GetPrefabFile() const;      // [ property ]

  bool GetSpawnAtStart() const; // [ property ]
  void SetSpawnAtStart(bool b); // [ property ]

  bool GetSpawnContinuously() const; // [ property ]
  void SetSpawnContinuously(bool b); // [ property ]

  bool GetAttachAsChild() const; // [ property ]
  void SetAttachAsChild(bool b); // [ property ]

  void SetPrefab(const plPrefabResourceHandle& hPrefab);
  PLASMA_ALWAYS_INLINE const plPrefabResourceHandle& GetPrefab() const { return m_hPrefab; }

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

  plArrayMap<plHashedString, plVariant> m_Parameters;

protected:
  plBitflags<plSpawnComponentFlags> m_SpawnFlags;

  virtual void DoSpawn(const plTransform& tLocalSpawn);
  bool SpawnOnce(const plVec3& vLocalOffset);
  void OnTriggered(plMsgComponentInternalTrigger& msg);

  plTime m_LastManualSpawn;
  plPrefabResourceHandle m_hPrefab;
};
