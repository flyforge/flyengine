#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <GameEngine/Animation/PathComponent.h>
#include <GameEngine/Animation/PropertyAnimResource.h>

struct plMsgAnimationReachedEnd;

//////////////////////////////////////////////////////////////////////////

using plFollowPathComponentManager = plComponentManagerSimple<class plFollowPathComponent, plComponentUpdateType::WhenSimulating>;

struct PLASMA_GAMEENGINE_DLL plFollowPathMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    OnlyPosition,
    AlignUpZ,
    FullRotation,

    Default = OnlyPosition
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GAMEENGINE_DLL, plFollowPathMode)

/// \brief This component makes the plGameObject, that it is attached to, move along a path defined by an plPathComponent.
///
/// Build a path using an plPathComponent and plPathNodeComponents.
/// Then attach an plFollowPathComponent to a free-standing plGameObject and reference the object with the plPathComponent in it.
///
/// During simulation the plFollowPathComponent will now move and rotate its owner object such that it moves along the path.
///
/// The start location of the 'hook' (the object with the plFollowPathComponent on it) may be anywhere. It will be teleported
/// onto the path. For many objects this is not a problem, but physically simulated objects may be very sensitive about this.
///
/// One option is to align the 'hook' perfectly with the start location.
/// You can achieve this, using the "Keep Simulation Changes" feature of the editor (simulate with zero speed, press K, stop simulation).
/// Another option is to instead delay the spawning of the object below the hook, by using an plSpawnComponent next to the plFollowPathComponent,
/// and thus have the payload spawn only after the hook has been placed properly.
class PLASMA_GAMEENGINE_DLL plFollowPathComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plFollowPathComponent, plComponent, plFollowPathComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& ref_stream) const override;
  virtual void DeserializeComponent(plWorldReader& ref_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // plFollowPathComponent

public:
  plFollowPathComponent();
  ~plFollowPathComponent();

  /// \brief Sets the reference to the game object on which an plPathComponent should be attached.
  void SetPathObject(const char* szReference); // [ property ]

  plEnum<plPropertyAnimMode> m_Mode;                    ///< [ property ] How the path should be traversed.
  plEnum<plFollowPathMode> m_FollowMode;                ///< [ property ] How the transform of the follower should be affected by the path.
  float m_fSpeed = 1.0f;                                ///< [ property ] How fast to move along the path.
  float m_fLookAhead = 1.0f;                            ///< [ property ] How far along the path to 'look ahead' to smooth the rotation. A small distance means rotations are very abrupt.
  float m_fSmoothing = 0.5f;                            ///< [ property ] How much to combine the current position with the new position. 0 to 1. At zero, the position follows the path perfectly, but therefore also has very abrupt changes. With a lot of smoothing, the path becomes very sluggish.
  float m_fTiltAmount = 5.0f;                           ///< [ property ] How much to tilt when turning.
  plAngle m_MaxTilt = plAngle::MakeFromDegree(30.0f);   ///< [ property ] The max tilt angle of the object.

  /// \brief Distance along the path at which the plFollowPathComponent should start off.
  void SetDistanceAlongPath(float fDistance); // [ property ]
  float GetDistanceAlongPath() const;         // [ property ]

  /// \brief Whether the component should move along the path 'forwards' or 'backwards'
  void SetDirectionForwards(bool bForwards); // [ scriptable ]

  /// \brief Toggles the direction that it travels along the path.
  void ToggleDirection(); // [ scriptable ]

  /// \brief Whether the component currently moves 'forwards' along the path.
  ///
  /// Note that if the 'speed' property is negative, moving 'forwards' along the path still means that it effectively moves backwards.
  bool IsDirectionForwards() const; // [ scriptable ]

  /// \brief Whether the component currently moves along the path, at all.
  bool IsRunning() const; // [ property ]

  /// \brief Whether to move along the path or not.
  void SetRunning(bool bRunning); // [ property ]

protected:
  void Update(bool bForce = false);

  plEventMessageSender<plMsgAnimationReachedEnd> m_ReachedEndEvent; // [ event ]
  plGameObjectHandle m_hPathObject;                                 // [ property ]
  plPathComponent::LinearSampler m_PathSampler;

  float m_fStartDistance = 0.0f; // [ property ]
  bool m_bLastStateValid = false;
  bool m_bIsRunning = true;
  bool m_bIsRunningForwards = true;
  plVec3 m_vLastPosition;
  plVec3 m_vLastTargetPosition;
  plVec3 m_vLastUpDir;
  plAngle m_LastTiltAngle;

  const char* DummyGetter() const { return nullptr; }
};
