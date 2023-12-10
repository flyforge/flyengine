#pragma once

#include <Core/Physics/SurfaceResource.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

struct plMsgComponentInternalTrigger;

typedef plComponentManagerSimple<class plProjectileComponent, plComponentUpdateType::WhenSimulating> plProjectileComponentManager;

/// \brief Defines what a projectile will do when it hits a surface
struct PLASMA_GAMEENGINE_DLL plProjectileReaction
{
  using StorageType = plInt8;

  enum Enum : StorageType
  {
    Absorb,      ///< The projectile simply stops and is deleted
    Reflect,     ///< Bounces away along the reflected direction
    Attach,      ///< Stops at the hit point, does not continue further and attaches itself as a child to the hit object
    PassThrough, ///< Continues flying through the geometry (but may spawn prefabs at the intersection points)

    Default = Absorb
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GAMEENGINE_DLL, plProjectileReaction);

/// \brief Holds the information about how a projectile interacts with a specific surface type
struct PLASMA_GAMEENGINE_DLL plProjectileSurfaceInteraction
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

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GAMEENGINE_DLL, plProjectileSurfaceInteraction);

class PLASMA_GAMEENGINE_DLL plProjectileComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plProjectileComponent, plComponent, plProjectileComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // plProjectileComponent

public:
  plProjectileComponent();
  ~plProjectileComponent();

  float m_fMetersPerSecond;                                                ///< [ property ] The speed at which the projectile flies
  float m_fGravityMultiplier;                                              ///< [ property ] If 0, the projectile is not affected by gravity.
  plUInt8 m_uiCollisionLayer;                                              ///< [ property ]
  plTime m_MaxLifetime;                                                    ///< [ property ] After this time the projectile is killed, if it didn't die already
  plSurfaceResourceHandle m_hFallbackSurface;                              ///< [ property ]
  plHybridArray<plProjectileSurfaceInteraction, 12> m_SurfaceInteractions; ///< [ property ]

  void SetTimeoutPrefab(const char* szPrefab); // [ property ]
  const char* GetTimeoutPrefab() const;        // [ property ]

  void SetFallbackSurfaceFile(const char* szFile); // [ property ]
  const char* GetFallbackSurfaceFile() const;      // [ property ]

private:
  void Update();
  void OnTriggered(plMsgComponentInternalTrigger& msg); // [ msg handler ]

  plPrefabResourceHandle m_hTimeoutPrefab; ///< Spawned when the projectile is killed due to m_MaxLifetime coming to an end

  /// \brief If an unknown surface type is hit, the projectile will just delete itself without further interaction
  plInt32 FindSurfaceInteraction(const plSurfaceResourceHandle& hSurface) const;

  void TriggerSurfaceInteraction(const plSurfaceResourceHandle& hSurface, plGameObjectHandle hObject, const plVec3& vPos, const plVec3& vNormal, const plVec3& vDirection, const char* szInteraction);

  plVec3 m_vVelocity;
};
