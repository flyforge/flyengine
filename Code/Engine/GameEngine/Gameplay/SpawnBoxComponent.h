#pragma once

#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>

struct plMsgComponentInternalTrigger;

struct plSpawnBoxComponentFlags
{
  using StorageType = plUInt16;

  enum Enum
  {
    None = 0,
    SpawnAtStart = PL_BIT(0),      ///< The component will schedule a spawn once at creation time
    SpawnContinuously = PL_BIT(1), ///< Every time a spawn duration has finished, a new one is started

    Default = None
  };

  struct Bits
  {
    StorageType SpawnAtStart : 1;
  };
};

PL_DECLARE_FLAGS_OPERATORS(plSpawnBoxComponentFlags);

using plSpawnBoxComponentManager = plComponentManager<class plSpawnBoxComponent, plBlockStorageType::Compact>;

/// \brief This component spawns prefabs inside a box.
///
/// The prefabs are spawned over a fixed duration.
/// The number of prefabs to spawn over the time duration is randomly chosen.
/// Each prefab may get rotated around the Z axis and tilted away from the Z axis.
/// If desired, the component can start spawning automatically, or it can be (re-)started from code.
/// If 'spawn continuously' is enabled, the component restarts itself after the spawn duration is over,
/// thus for every spawn duration the number of prefabs to spawn gets reevaluated.
class plSpawnBoxComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plSpawnBoxComponent, plComponent, plSpawnBoxComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // plSpawnBoxComponent

public:
  /// \brief When called, the component starts spawning the chosen number of prefabs over the set duration.
  ///
  /// If this is called while the component is already active, the internal state is reset and it starts over.
  void StartSpawning();                                           // [ scriptable ]

  void SetHalfExtents(const plVec3& value);                       // [ property ]
  const plVec3& GetHalfExtents() const { return m_vHalfExtents; } // [ property ]

  void SetPrefabFile(const char* szFile);                         // [ property ]
  const char* GetPrefabFile() const;                              // [ property ]

  bool GetSpawnAtStart() const;                                   // [ property ]
  void SetSpawnAtStart(bool b);                                   // [ property ]

  bool GetSpawnContinuously() const;                              // [ property ]
  void SetSpawnContinuously(bool b);                              // [ property ]

  plTime m_SpawnDuration;                                         // [ property ]
  plUInt16 m_uiMinSpawnCount = 5;                                 // [ property ]
  plUInt16 m_uiSpawnCountRange = 5;                               // [ property ]
  plPrefabResourceHandle m_hPrefab;                               // [ property ]

  /// The spawned object's forward direction may deviate this amount from the spawn box's forward rotation. This is accomplished by rotating around the Z axis.
  plAngle m_MaxRotationZ; // [ property ]

  /// The spawned object's Z (up) axis may deviate by this amount from the spawn box's Z axis.
  plAngle m_MaxTiltZ; // [ property ]


private:
  void OnTriggered(plMsgComponentInternalTrigger& msg);
  void Spawn(plUInt32 uiCount);
  void InternalStartSpawning(bool bFirstTime);

  plUInt16 m_uiSpawned = 0;
  plUInt16 m_uiTotalToSpawn = 0;
  plTime m_StartTime;
  plBitflags<plSpawnBoxComponentFlags> m_Flags;
  plVec3 m_vHalfExtents = plVec3(0.5f);
};
