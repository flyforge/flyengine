#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/Physics/RopeSimulator.h>

//////////////////////////////////////////////////////////////////////////

class PL_GAMECOMPONENTS_DLL plFakeRopeComponentManager : public plComponentManager<class plFakeRopeComponent, plBlockStorageType::FreeList>
{
public:
  plFakeRopeComponentManager(plWorld* pWorld);
  ~plFakeRopeComponentManager();

  virtual void Initialize() override;

private:
  void Update(const plWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

/// \brief Simulates the behavior of a rope/cable without physical interaction with the world.
///
/// This component is mainly used for decorative purposes to add things like power lines, cables and ropes
/// that hang in a level and should swing in the wind. This component only simulates simple rope physics,
/// but doesn't take the surrounding geometry into account. Thus ropes will swing through other geometry,
/// and the only way to prevent that, is to place the rope such, that it usually doesn't come close to other objects.
///
/// The fake rope simulation is more lightweight than proper physical simulation and also stops simulating when
/// it isn't visible.
///
/// Prefer to use this over e.g. the plJoltRopeComponent, when the physical interaction isn't needed.
class PL_GAMECOMPONENTS_DLL plFakeRopeComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plFakeRopeComponent, plComponent, plFakeRopeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plFakeRopeComponent

public:
  plFakeRopeComponent();
  ~plFakeRopeComponent();

  /// Of how many pieces the rope is made up.
  plUInt16 m_uiPieces = 16; // [ property ]

  void SetAnchor1Reference(const char* szReference); // [ property ]
  void SetAnchor2Reference(const char* szReference); // [ property ]

  /// \brief The first game object to attach to.
  void SetAnchor1(plGameObjectHandle hActor);
  /// \brief The second game object to attach to.
  void SetAnchor2(plGameObjectHandle hActor);

  /// \brief How much the rope should sag. A value of 0 means it should be absolutely straight. In practice there is always slack, due to imprecision in the simulation.
  void SetSlack(float fVal);
  float GetSlack() const { return m_fSlack; }

  /// \brief If true, the rope will be fixed to the first anchor, otherwise it will start there but then fall down.
  void SetAttachToAnchor1(bool bVal);
  /// \brief If true, the rope will be fixed to the second anchor, otherwise it will start there but then fall down.
  void SetAttachToAnchor2(bool bVal);
  bool GetAttachToAnchor1() const;
  bool GetAttachToAnchor2() const;

  /// How quickly a swinging rope comes to a stop.
  float m_fDamping = 0.5f; // [ property ]

private:
  plResult ConfigureRopeSimulator();
  void SendCurrentPose();
  void SendPreviewPose();
  void RuntimeUpdate();

  plGameObjectHandle m_hAnchor1;
  plGameObjectHandle m_hAnchor2;

  float m_fSlack = 0.0f;
  plUInt32 m_uiPreviewHash = 0;

  // if the owner or the anchor object are flagged as 'dynamic', the rope must follow their movement
  // otherwise it can skip some update steps
  bool m_bIsDynamic = true;
  plUInt8 m_uiCheckEquilibriumCounter = 0;
  plUInt8 m_uiSleepCounter = 0;
  plRopeSimulator m_RopeSim;
  float m_fWindInfluence = 0.0f;

private:
  const char* DummyGetter() const { return nullptr; }
};
