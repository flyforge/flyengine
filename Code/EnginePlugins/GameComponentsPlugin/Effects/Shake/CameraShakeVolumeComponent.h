#pragma once

#include <Core/World/Component.h>
#include <Core/World/Declarations.h>
#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>

struct plMsgUpdateLocalBounds;
struct plMsgComponentInternalTrigger;
struct plMsgDeleteGameObject;

/// \brief Base class for components that define volumes in which a camera shake effect shall be applied.
///
/// Derived classes implement different shape types and how the shake strength is calculated.
class PL_GAMECOMPONENTS_DLL plCameraShakeVolumeComponent : public plComponent
{
  PL_DECLARE_ABSTRACT_COMPONENT_TYPE(plCameraShakeVolumeComponent, plComponent);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // plCameraShakeVolumeComponent

public:
  plCameraShakeVolumeComponent();
  ~plCameraShakeVolumeComponent();

  /// \brief The spatial category used to find camera shake volume components through the spatial system.
  static plSpatialData::Category SpatialDataCategory;

  /// \brief How long a shake burst should last. Zero for constant shaking.
  plTime m_BurstDuration; // [ property ]

  /// \brief How strong the shake should be at the strongest point. Typically a value between one and zero.
  float m_fStrength;      // [ property ]

  /// \brief Calculates the shake strength at the given global position.
  float ComputeForceAtGlobalPosition(const plSimdVec4f& vGlobalPos) const;

  /// \brief Calculates the shake strength in local space of the component.
  virtual float ComputeForceAtLocalPosition(const plSimdVec4f& vLocalPos) const = 0;

  /// \brief In case of a burst shake, defines whether the component should delete itself afterwards.
  plEnum<plOnComponentFinishedAction> m_OnFinishedAction; // [ property ]

protected:
  void OnTriggered(plMsgComponentInternalTrigger& msg);
  void OnMsgDeleteGameObject(plMsgDeleteGameObject& msg);
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

using plCameraShakeVolumeSphereComponentManager = plComponentManager<class plCameraShakeVolumeSphereComponent, plBlockStorageType::Compact>;

/// \brief A spherical volume in which a camera shake will be applied.
///
/// The shake strength is strongest at the center of the sphere and gradually weaker towards the sphere radius.
///
/// \see plCameraShakeVolumeComponent
/// \see plCameraShakeComponent
class PL_GAMECOMPONENTS_DLL plCameraShakeVolumeSphereComponent : public plCameraShakeVolumeComponent
{
  PL_DECLARE_COMPONENT_TYPE(plCameraShakeVolumeSphereComponent, plCameraShakeVolumeComponent, plCameraShakeVolumeSphereComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plCameraShakeVolumeSphereComponent

public:
  plCameraShakeVolumeSphereComponent();
  ~plCameraShakeVolumeSphereComponent();

  virtual float ComputeForceAtLocalPosition(const plSimdVec4f& vLocalPos) const override;

  float GetRadius() const { return m_fRadius; } // [ property ]
  void SetRadius(float fVal);                   // [ property ]

private:
  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg);

  float m_fRadius = 1.0f;
  plSimdFloat m_fOneDivRadius;
};
