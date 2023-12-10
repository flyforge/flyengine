#pragma once

#include <GameEngine/Animation/TransformComponent.h>
#include <GameEngine/GameEngineDLL.h>

typedef plComponentManagerSimple<class plHeadBoneComponent, plComponentUpdateType::WhenSimulating> plHeadBoneComponentManager;

class PLASMA_GAMEENGINE_DLL plHeadBoneComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plHeadBoneComponent, plComponent, plHeadBoneComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plHeadBoneComponent

public:
  plHeadBoneComponent();
  ~plHeadBoneComponent();


  void SetVerticalRotation(float radians);    // [ scriptable ]
  void ChangeVerticalRotation(float radians); // [ scriptable ]

  plAngle m_NewVerticalRotation;                       // [ property ]
  plAngle m_MaxVerticalRotation = plAngle::MakeFromDegree(80); // [ property ]

protected:
  void Update();

  plAngle m_CurVerticalRotation;
};
