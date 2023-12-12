#pragma once

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/Component.h>
#include <Core/World/Declarations.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

struct plMsgUpdateLocalBounds;
struct plMsgComponentInternalTrigger;
struct plMsgDeleteGameObject;

class PLASMA_GAMEENGINE_DLL plWindVolumeComponent : public plComponent
{
  PLASMA_DECLARE_ABSTRACT_COMPONENT_TYPE(plWindVolumeComponent, plComponent);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // plWindVolumeComponent

public:
  plWindVolumeComponent();
  ~plWindVolumeComponent();

  static plSpatialData::Category SpatialDataCategory;

  plTime m_BurstDuration;            // [ property ]
  plEnum<plWindStrength> m_Strength; // [ property ]
  bool m_bReverseDirection = false;  // [ property ]

  plSimdVec4f ComputeForceAtGlobalPosition(const plSimdVec4f& globalPos) const;

  virtual plSimdVec4f ComputeForceAtLocalPosition(const plSimdVec4f& localPos) const = 0;

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

class PLASMA_GAMEENGINE_DLL plWindVolumeSphereComponent : public plWindVolumeComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plWindVolumeSphereComponent, plWindVolumeComponent, plWindVolumeSphereComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plWindVolumeSphereComponent

public:
  plWindVolumeSphereComponent();
  ~plWindVolumeSphereComponent();

  virtual plSimdVec4f ComputeForceAtLocalPosition(const plSimdVec4f& localPos) const override;

  float GetRadius() const { return m_fRadius; } // [ property ]
  void SetRadius(float val);                    // [ property ]

private:
  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg);

  float m_fRadius = 1.0f;
  plSimdFloat m_fOneDivRadius;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct plWindVolumeCylinderMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    Directional,
    Vortex,

    Default = Directional
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GAMEENGINE_DLL, plWindVolumeCylinderMode);

using plWindVolumeCylinderComponentManager = plComponentManager<class plWindVolumeCylinderComponent, plBlockStorageType::Compact>;

class PLASMA_GAMEENGINE_DLL plWindVolumeCylinderComponent : public plWindVolumeComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plWindVolumeCylinderComponent, plWindVolumeComponent, plWindVolumeCylinderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plWindVolumeCylinderComponent

public:
  plWindVolumeCylinderComponent();
  ~plWindVolumeCylinderComponent();

  virtual plSimdVec4f ComputeForceAtLocalPosition(const plSimdVec4f& localPos) const override;

  float GetRadius() const { return m_fRadius; } // [ property ]
  void SetRadius(float val);                    // [ property ]

  float GetLength() const { return m_fLength; } // [ property ]
  void SetLength(float val);                    // [ property ]

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

class PLASMA_GAMEENGINE_DLL plWindVolumeConeComponent : public plWindVolumeComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plWindVolumeConeComponent, plWindVolumeComponent, plWindVolumeConeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plWindVolumeCylinderComponent

public:
  plWindVolumeConeComponent();
  ~plWindVolumeConeComponent();

  virtual plSimdVec4f ComputeForceAtLocalPosition(const plSimdVec4f& localPos) const override;

  float GetLength() const { return m_fLength; } // [ property ]
  void SetLength(float val);                    // [ property ]

  plAngle GetAngle() const { return m_Angle; } // [ property ]
  void SetAngle(plAngle val);                  // [ property ]

private:
  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg);

  float m_fLength = 1.0f;
  plAngle m_Angle = plAngle::Degree(45);
};
