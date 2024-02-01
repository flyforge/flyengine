#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/Utils/IntervalScheduler.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

class plPhysicsWorldModuleInterface;

struct PL_GAMEENGINE_DLL plMsgSensorDetectedObjectsChanged : public plEventMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgSensorDetectedObjectsChanged, plEventMessage);

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
class PL_GAMEENGINE_DLL plSensorComponent : public plComponent
{
  PL_DECLARE_ABSTRACT_COMPONENT_TYPE(plSensorComponent, plComponent);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plSensorComponent

public:
  plSensorComponent();
  ~plSensorComponent();

  virtual void GetObjectsInSensorVolume(plDynamicArray<plGameObject*>& out_objects) const = 0;
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

  plTagSet m_IncludeTags; // [ property ]
  plTagSet m_ExcludeTags; // [ property ]

  /// \brief Returns the list of objects that this sensor has detected during its last update
  plArrayPtr<const plGameObjectHandle> GetLastDetectedObjects() const { return m_LastDetectedObjects; }

  /// \brief Updates the sensor state right now.
  ///
  /// If the update rate isn't set to 'Never', this is periodically done automatically.
  /// Otherwise, it has to be called manually to update the state on demand.
  ///
  /// Afterwards out_objectsInSensorVolume will contain all objects that were found inside the volume.
  /// ref_detectedObjects needs to be provided as a temp array, but will not contain a usable result afterwards,
  /// call GetLastDetectedObjects() instead.
  ///
  /// If bPostChangeMsg is true, plMsgSensorDetectedObjectsChanged is posted in case there is a change.
  /// Physical visibility checks are skipped in case pPhysicsWorldModule is null.
  ///
  /// Returns true, if there was a change in detected objects, false if the same objects were detected as last time.
  bool RunSensorCheck(plPhysicsWorldModuleInterface* pPhysicsWorldModule, plDynamicArray<plGameObject*>& out_objectsInSensorVolume, plDynamicArray<plGameObjectHandle>& ref_detectedObjects, bool bPostChangeMsg) const;

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

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  mutable plDynamicArray<plVec3> m_LastOccludedObjectPositions;
#endif
};

//////////////////////////////////////////////////////////////////////////

using plSensorSphereComponentManager = plComponentManager<class plSensorSphereComponent, plBlockStorageType::Compact>;

class PL_GAMEENGINE_DLL plSensorSphereComponent : public plSensorComponent
{
  PL_DECLARE_COMPONENT_TYPE(plSensorSphereComponent, plSensorComponent, plSensorSphereComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plSensorComponent

  virtual void GetObjectsInSensorVolume(plDynamicArray<plGameObject*>& out_objects) const override;
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

class PL_GAMEENGINE_DLL plSensorCylinderComponent : public plSensorComponent
{
  PL_DECLARE_COMPONENT_TYPE(plSensorCylinderComponent, plSensorComponent, plSensorCylinderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plSensorComponent

  virtual void GetObjectsInSensorVolume(plDynamicArray<plGameObject*>& out_objects) const override;
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

class PL_GAMEENGINE_DLL plSensorConeComponent : public plSensorComponent
{
  PL_DECLARE_COMPONENT_TYPE(plSensorConeComponent, plSensorComponent, plSensorConeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plSensorComponent

  virtual void GetObjectsInSensorVolume(plDynamicArray<plGameObject*>& out_objects) const override;
  virtual void DebugDrawSensorShape() const override;

  //////////////////////////////////////////////////////////////////////////
  // plSensorConeComponent

public:
  plSensorConeComponent();
  ~plSensorConeComponent();

  float m_fNearDistance = 0.0f;             // [ property ]
  float m_fFarDistance = 10.0f;             // [ property ]
  plAngle m_Angle = plAngle::MakeFromDegree(90.0f); // [ property ]
};

//////////////////////////////////////////////////////////////////////////

class plSensorWorldModule : public plWorldModule
{
  PL_DECLARE_WORLD_MODULE();
  PL_ADD_DYNAMIC_REFLECTION(plSensorWorldModule, plWorldModule);

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
