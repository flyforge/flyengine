#pragma once

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/Component.h>
#include <Core/World/Declarations.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

struct plMsgUpdateLocalBounds;
struct plMsgComponentInternalTrigger;
struct plMsgDeleteGameObject;

/// \brief Base class for components that define wind volumes.
///
/// These components define the shape in which to apply wind to objects that support this functionality.
class PL_GAMEENGINE_DLL plWindVolumeComponent : public plComponent
{
  PL_DECLARE_ABSTRACT_COMPONENT_TYPE(plWindVolumeComponent, plComponent);

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
  // plWindVolumeComponent

public:
  plWindVolumeComponent();
  ~plWindVolumeComponent();

  /// \brief The spatial category to use to find all wind volume components through the spatial system.
  static plSpatialData::Category SpatialDataCategory;

  /// \brief If non-zero, the wind will only last for a limited amount of time.
  plTime m_BurstDuration; // [ property ]

  /// \brief How strong the wind shall blow at the strongest point of the volume.
  plEnum<plWindStrength> m_Strength; // [ property ]

  /// \brief Factor to scale the wind strength. Negative values can be used to reverse the wind direction.
  float m_fStrengthFactor = 1.0f;

  /// \brief Computes the wind force at a global position.
  ///
  /// Only the x,y,z components are used, they are a wind direction vector scaled to the wind speed.
  plSimdVec4f ComputeForceAtGlobalPosition(const plSimdVec4f& vGlobalPos) const;

  virtual plSimdVec4f ComputeForceAtLocalPosition(const plSimdVec4f& vLocalPos) const = 0;

  /// \brief What happens after the wind burst is over.
  plEnum<plOnComponentFinishedAction> m_OnFinishedAction; // [ property ]

protected:
  void OnTriggered(plMsgComponentInternalTrigger& msg);
  void OnMsgDeleteGameObject(plMsgDeleteGameObject& msg);

  float GetWindInMetersPerSecond() const;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

using plWindVolumeSphereComponentManager = plComponentManager<class plWindVolumeSphereComponent, plBlockStorageType::Compact>;

/// \brief A spherical shape in which wind shall be applied to objects.
///
/// The wind blows outwards from the center of the sphere. If the wind direction is reversed, it pulls objects inwards.
class PL_GAMEENGINE_DLL plWindVolumeSphereComponent : public plWindVolumeComponent
{
  PL_DECLARE_COMPONENT_TYPE(plWindVolumeSphereComponent, plWindVolumeComponent, plWindVolumeSphereComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plWindVolumeSphereComponent

public:
  plWindVolumeSphereComponent();
  ~plWindVolumeSphereComponent();

  virtual plSimdVec4f ComputeForceAtLocalPosition(const plSimdVec4f& vLocalPos) const override;

  float GetRadius() const { return m_fRadius; } // [ property ]
  void SetRadius(float fVal);                   // [ property ]

private:
  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg);

  float m_fRadius = 1.0f;
  plSimdFloat m_fOneDivRadius;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief How the wind direction shall be calculated in a cylindrical wind volume.
struct plWindVolumeCylinderMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    Directional, ///< The wind direction is outwards from the cylinder.
    Vortex,      ///< The wind direction is tangential, moving in a circular fashion around the cylinder like in a tornado.

    Default = Directional
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_GAMEENGINE_DLL, plWindVolumeCylinderMode);

using plWindVolumeCylinderComponentManager = plComponentManager<class plWindVolumeCylinderComponent, plBlockStorageType::Compact>;

/// \brief A cylindrical volume in which wind shall be applied.
///
/// The wind direction may be either outwards from the cylinder center, or tangential (a vortex).
class PL_GAMEENGINE_DLL plWindVolumeCylinderComponent : public plWindVolumeComponent
{
  PL_DECLARE_COMPONENT_TYPE(plWindVolumeCylinderComponent, plWindVolumeComponent, plWindVolumeCylinderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plWindVolumeCylinderComponent

public:
  plWindVolumeCylinderComponent();
  ~plWindVolumeCylinderComponent();

  virtual plSimdVec4f ComputeForceAtLocalPosition(const plSimdVec4f& vLocalPos) const override;

  float GetRadius() const { return m_fRadius; } // [ property ]
  void SetRadius(float fVal);                   // [ property ]

  float GetLength() const { return m_fLength; } // [ property ]
  void SetLength(float fVal);                   // [ property ]

  plEnum<plWindVolumeCylinderMode> m_Mode; // [ property ]

private:
  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg);

  float m_fRadius = 1.0f;
  float m_fLength = 5.0f;
  plSimdFloat m_fOneDivRadius;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

using plWindVolumeConeComponentManager = plComponentManager<class plWindVolumeConeComponent, plBlockStorageType::Compact>;

/// \brief A conical shape in which wind shall be applied to objects.
///
/// The wind is applied from the tip of the cone along the cone axis.
/// Strength falloff is only by distance along the cone main axis.
class PL_GAMEENGINE_DLL plWindVolumeConeComponent : public plWindVolumeComponent
{
  PL_DECLARE_COMPONENT_TYPE(plWindVolumeConeComponent, plWindVolumeComponent, plWindVolumeConeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plWindVolumeCylinderComponent

public:
  plWindVolumeConeComponent();
  ~plWindVolumeConeComponent();

  virtual plSimdVec4f ComputeForceAtLocalPosition(const plSimdVec4f& vLocalPos) const override;

  float GetLength() const { return m_fLength; } // [ property ]
  void SetLength(float fVal);                   // [ property ]

  plAngle GetAngle() const { return m_Angle; } // [ property ]
  void SetAngle(plAngle val);                  // [ property ]

private:
  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg);

  float m_fLength = 1.0f;
  plAngle m_Angle = plAngle::MakeFromDegree(45);
};
