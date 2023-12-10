#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Time/Time.h>
#include <GameEngine/GameEngineDLL.h>

struct plTransformComponentFlags
{
  typedef plUInt16 StorageType;

  enum Enum
  {
    None = 0,
    Running = PLASMA_BIT(0),
    AutoReturnStart = PLASMA_BIT(1),
    AutoReturnEnd = PLASMA_BIT(2),
    AnimationReversed = PLASMA_BIT(5),
    Default = Running | AutoReturnStart | AutoReturnEnd
  };

  struct Bits
  {
    StorageType Running : 1;
    StorageType AutoReturnStart : 1;
    StorageType AutoReturnEnd : 1;
    StorageType Unused1 : 1;
    StorageType Unused2 : 1;
    StorageType AnimationReversed : 1;
  };
};

PLASMA_DECLARE_FLAGS_OPERATORS(plTransformComponentFlags);

class PLASMA_GAMEENGINE_DLL plTransformComponent : public plComponent
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTransformComponent, plComponent);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // plTransformComponent

public:
  plTransformComponent();
  ~plTransformComponent();

  void SetDirectionForwards(bool bForwards); // [ scriptable ]
  void ToggleDirection();                    // [ scriptable ]
  bool IsDirectionForwards() const;          // [ scriptable ]

  bool IsRunning(void) const;     // [ property ]
  void SetRunning(bool bRunning); // [ property ]

  bool GetReverseAtStart(void) const; // [ property ]
  void SetReverseAtStart(bool b);     // [ property ]

  bool GetReverseAtEnd(void) const; // [ property ]
  void SetReverseAtEnd(bool b);     // [ property ]

  float m_fAnimationSpeed = 1.0f; // [ property ]

protected:
  plBitflags<plTransformComponentFlags> m_Flags;
  plTime m_AnimationTime;
};
