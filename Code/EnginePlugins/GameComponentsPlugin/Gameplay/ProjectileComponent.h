#pragma once

#include <Core/Physics/SurfaceResource.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>

struct plMsgComponentInternalTrigger;

using plProjectileComponentManager = plComponentManagerSimple<class plProjectileComponent, plComponentUpdateType::WhenSimulating>;

/// \brief Defines what a projectile will do when it hits a surface
struct PL_GAMECOMPONENTS_DLL plProjectileReaction
{
  using StorageType = plInt8;

  enum Enum : StorageType
  {
    Absorb,      ///< The projectile simply stops and is deleted
    Reflect,     ///< Bounces away along the reflected direction. Maintains momentum.
    Bounce,      ///< Bounces away along the reflected direction. Loses momentum.
    Attach,      ///< Stops at the hit point, does not continue further and attaches itself as a child to the hit object
    PassThrough, ///< Continues flying through the geometry (but may spawn prefabs at the intersection points)

    Default = Absorb
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_GAMECOMPONENTS_DLL, plProjectileReaction);

/// \brief Holds the information about how a projectile interacts with a specific surface type
struct PL_GAMECOMPONENTS_DLL plProjectileSurfaceInteraction
{
  void SetSurface(const char* szSurface);
  const char* GetSurface() const;

  /// \brief The surface type (and derived ones) for which this interaction is used
  plSurfaceResourceHandle m_hSurface;

  /// \brief How the projectile itself will react when hitting the surface type
  plProjectileReaction::Enum m_Reaction;

  /// \brief Which interaction should be triggered. See plSurfaceResource.
  plString m_sInteraction;

  /// \brief The force (or rather impulse) that is applied on the object
  float m_fImpulse = 0.0f;

  /// \brief How much damage to do on this type of surface. Send via plMsgDamage
  float m_fDamage = 0.0f;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_GAMECOMPONENTS_DLL, plProjectileSurfaceInteraction);

/// \brief Shoots a game object in a straight line and uses physics raycasts to detect hits.
///
/// When a raycast detects a hit, the surface information is used to determine how the projectile should proceed
/// and which prefab it should spawn as an effect.
class PL_GAMECOMPONENTS_DLL plProjectileComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plProjectileComponent, plComponent, plProjectileComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // plProjectileComponent

public:
  plProjectileComponent();
  ~plProjectileComponent();

  /// The speed at which the projectile flies.
  float m_fMetersPerSecond; // [ property ]

  /// If 0, the projectile is not affected by gravity.
  float m_fGravityMultiplier; // [ property ]

  // If true the death prefab will be spawned when the velocity gones under the threshold to be considered static 
  bool m_bSpawnPrefabOnStatic; // [ property ]

  /// Defines which other physics objects the projectile will collide with.
  plUInt8 m_uiCollisionLayer; // [ property ]

  /// A broad filter to ignore certain types of colliders.
  plBitflags<plPhysicsShapeType> m_ShapeTypesToHit; // [ property ]

  /// After this time the projectile is removed, if it didn't hit anything yet.
  plTime m_MaxLifetime; // [ property ]

  /// If the projectile hits something that has no valid surface, this surface is used instead.
  plSurfaceResourceHandle m_hFallbackSurface; // [ property ]

  /// Specifies how the projectile interacts with different surface types.
  plHybridArray<plProjectileSurfaceInteraction, 12> m_SurfaceInteractions; // [ property ]

  /// \brief If the projectile reaches its maximum lifetime it can spawn this prefab.
  void SetDeathPrefab(const char* szPrefab); // [ property ]
  const char* GetDeathPrefab() const;             // [ property ]

  void SetFallbackSurfaceFile(const char* szFile); // [ property ]
  const char* GetFallbackSurfaceFile() const;      // [ property ]

private:
  void Update();
  void OnTriggered(plMsgComponentInternalTrigger& msg); // [ msg handler ]

  void SpawnDeathPrefab();

  plPrefabResourceHandle m_hDeathPrefab; ///< Spawned when the projectile is killed due to m_MaxLifetime coming to an end

  /// \brief If an unknown surface type is hit, the projectile will just delete itself without further interaction
  plInt32 FindSurfaceInteraction(const plSurfaceResourceHandle& hSurface) const;

  void TriggerSurfaceInteraction(const plSurfaceResourceHandle& hSurface, plGameObjectHandle hObject, const plVec3& vPos, const plVec3& vNormal, const plVec3& vDirection, const char* szInteraction);

  plVec3 m_vVelocity;
};
