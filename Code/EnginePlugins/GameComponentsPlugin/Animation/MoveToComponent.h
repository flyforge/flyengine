#pragma once

#include <GameComponentsPlugin/GameComponentsDLL.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Time/Time.h>

struct plMoveToComponentFlags
{
  using StorageType = plUInt16;

  enum Enum
  {
    None = 0,
    Running = PL_BIT(0),
    Default = None
  };

  struct Bits
  {
    StorageType Running : 1;
  };
};

using plMoveToComponentManager = plComponentManagerSimple<class plMoveToComponent, plComponentUpdateType::WhenSimulating>;

PL_DECLARE_FLAGS_OPERATORS(plMoveToComponentFlags);

/// \brief A light-weight component that moves the owner object towards a single position.
///
/// The functionality of this component can only be controlled through (script) code.
/// The component is given a single point in global space. When it is set to 'running' it will move
/// the owning object towards this point. Optionally it may use acceleration or deceleration and a maximum speed
/// to reach that point.
///
/// Since the target position is given through code and can be modified at any time, this component can be used
/// for moving objects to a point that is decided dynamically. For example an elevator can be moved to a
/// specific height, depending on which floor was selected. Or an object could follow a character,
/// by updating the target position regularly.
///
/// The component sends the event 'plMsgAnimationReachedEnd' and resets its running state when it reaches the target position.
class PL_GAMECOMPONENTS_DLL plMoveToComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plMoveToComponent, plComponent, plMoveToComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // plTransformComponent

public:
  plMoveToComponent();
  ~plMoveToComponent();

  /// \brief If set to false, the animation stops immediately.
  void SetRunning(bool bRunning); // [ property ]
  bool IsRunning() const;         // [ property ]

  void SetTargetPosition(const plVec3& vPos); // [ scriptable ]

protected:
  void Update();

  plEventMessageSender<plMsgAnimationReachedEnd> m_ReachedEndMsgSender; // [ event ]

  float m_fCurTranslationSpeed = 0;
  float m_fMaxTranslationSpeed = 1;     // [ property ]
  float m_fTranslationAcceleration = 0; // [ property ]
  float m_fTranslationDeceleration = 0; // [ property ]

  plVec3 m_vTargetPosition;
  plBitflags<plMoveToComponentFlags> m_Flags;
};
