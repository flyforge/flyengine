#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/Utils/IntervalScheduler.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

class plPhysicsWorldModuleInterface;

struct PLASMA_GAMEENGINE_DLL plMsgSensorDetectedObjectsChanged : public plEventMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgSensorDetectedObjectsChanged, plEventMessage);

  plArrayPtr<plGameObjectHandle> m_DetectedObjects;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Base class for sensor components that can be used for AI perception like vision or hearing.
///
/// Derived component classes implemented different shapes like sphere cylinder or cone.
/// All sensors do a query with the specified spatial category in the world's spatial system first, therefore it is necessary to have objects
/// with matching spatial category for the sensors to detect them. This can be achieved with components like e.g. plMarkerComponent.
/// Visibility tests via raycasts are done afterwards by default but can be disabled.
/// The components store an array of all their currently detected objects and send an plMsgSensorDetectedObjectsChanged message if this array changes.
class PLASMA_GAMEENGINE_DLL plSensorComponent : public plComponent
{
  PLASMA_DECLARE_ABSTRACT_COMPONENT_TYPE(plSensorComponent, plComponent);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plSensorComponent

public:
  plSensorComponent();
  ~plSensorComponent();

  virtual void GetObjectsInSensorVolume(plDynamicArray<plGameObject*>& out_Objects) const = 0;
  virtual void DebugDrawSensorShape() const = 0;

  void SetSpatialCategory(const char* szCategory); // [ property ]
  const char* GetSpatialCategory() const;          // [ property ]

  bool m_bTestVisibility = true;  // [ property ]
  plUInt8 m_uiCollisionLayer = 0; // [ property ]

  void SetUpdateRate(const plEnum<plUpdateRate>& updateRate); // [ property ]
  const plEnum<plUpdateRate>& GetUpdateRate() const;          // [ property ]

  void SetShowDebugInfo(bool bShow); // [ property ]
  bool GetShowDebugInfo() const;     // [ property ]

  void SetColor(plColorGammaUB color); // [ property ]
  plColorGammaUB GetColor() const;     // [ property ]

  /// \brief Returns the list of objects that this sensor has detected during its last update
  plArrayPtr<plGameObjectHandle> GetLastDetectedObjects() const { return m_LastDetectedObjects; }

protected:
  void UpdateSpatialCategory();
  void UpdateScheduling();
  void UpdateDebugInfo();

  plEnum<plUpdateRate> m_UpdateRate;
  bool m_bShowDebugInfo = false;
  plColorGammaUB m_Color = plColorScheme::LightUI(plColorScheme::Orange);

  plHashedString m_sSpatialCategory;
  plSpatialData::Category m_SpatialCategory = plInvalidSpatialDataCategory;

  friend class plSensorWorldModule;
  mutable plDynamicArray<plGameObjectHandle> m_LastDetectedObjects;

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  mutable plDynamicArray<plVec3> m_LastOccludedObjectPositions;
#endif
};

//////////////////////////////////////////////////////////////////////////

using plSensorSphereComponentManager = plComponentManager<class plSensorSphereComponent, plBlockStorageType::Compact>;

class PLASMA_GAMEENGINE_DLL plSensorSphereComponent : public plSensorComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plSensorSphereComponent, plSensorComponent, plSensorSphereComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plSensorComponent

  virtual void GetObjectsInSensorVolume(plDynamicArray<plGameObject*>& out_Objects) const override;
  virtual void DebugDrawSensorShape() const override;

  //////////////////////////////////////////////////////////////////////////
  // plSensorSphereComponent

public:
  plSensorSphereComponent();
  ~plSensorSphereComponent();

  float m_fRadius = 10.0f; // [ property ]
};

//////////////////////////////////////////////////////////////////////////

using plSensorCylinderComponentManager = plComponentManager<class plSensorCylinderComponent, plBlockStorageType::Compact>;

class PLASMA_GAMEENGINE_DLL plSensorCylinderComponent : public plSensorComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plSensorCylinderComponent, plSensorComponent, plSensorCylinderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plSensorComponent

  virtual void GetObjectsInSensorVolume(plDynamicArray<plGameObject*>& out_Objects) const override;
  virtual void DebugDrawSensorShape() const override;

  //////////////////////////////////////////////////////////////////////////
  // plSensorCylinderComponent

public:
  plSensorCylinderComponent();
  ~plSensorCylinderComponent();

  float m_fRadius = 10.0f; // [ property ]
  float m_fHeight = 10.0f; // [ property ]
};

//////////////////////////////////////////////////////////////////////////

using plSensorConeComponentManager = plComponentManager<class plSensorConeComponent, plBlockStorageType::Compact>;

class PLASMA_GAMEENGINE_DLL plSensorConeComponent : public plSensorComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plSensorConeComponent, plSensorComponent, plSensorConeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plSensorComponent

  virtual void GetObjectsInSensorVolume(plDynamicArray<plGameObject*>& out_Objects) const override;
  virtual void DebugDrawSensorShape() const override;

  //////////////////////////////////////////////////////////////////////////
  // plSensorConeComponent

public:
  plSensorConeComponent();
  ~plSensorConeComponent();

  float m_fNearDistance = 0.0f;             // [ property ]
  float m_fFarDistance = 10.0f;             // [ property ]
  plAngle m_Angle = plAngle::Degree(90.0f); // [ property ]
};

//////////////////////////////////////////////////////////////////////////

class plSensorWorldModule : public plWorldModule
{
  PLASMA_DECLARE_WORLD_MODULE();
  PLASMA_ADD_DYNAMIC_REFLECTION(plSensorWorldModule, plWorldModule);

public:
  plSensorWorldModule(plWorld* pWorld);

  virtual void Initialize() override;

  void AddComponentToSchedule(plSensorComponent* pComponent, plUpdateRate::Enum updateRate);
  void RemoveComponentToSchedule(plSensorComponent* pComponent);

  void AddComponentForDebugRendering(plSensorComponent* pComponent);
  void RemoveComponentForDebugRendering(plSensorComponent* pComponent);

private:
  void UpdateSensors(const plWorldModule::UpdateContext& context);
  void DebugDrawSensors(const plWorldModule::UpdateContext& context);

  plIntervalScheduler<plComponentHandle> m_Scheduler;
  plPhysicsWorldModuleInterface* m_pPhysicsWorldModule = nullptr;

  plDynamicArray<plGameObject*> m_ObjectsInSensorVolume;
  plDynamicArray<plGameObjectHandle> m_DetectedObjects;

  plDynamicArray<plComponentHandle> m_DebugComponents;
};
