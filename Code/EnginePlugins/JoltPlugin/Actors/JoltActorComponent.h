#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Types/Bitflags.h>
#include <JoltPlugin/Declarations.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>

namespace JPH
{
  class Shape;
  class BodyCreationSettings;
} // namespace JPH

/// \brief Base class for all Jolt actors.
///
/// An actor is an object that participates in the physical simulation.
/// It is often also called a (rigid) body.
/// An actor is made out of one or multiple shapes that define its geometry.
/// Different types of actors differ in how they participate in the simulation.
class PL_JOLTPLUGIN_DLL plJoltActorComponent : public plComponent
{
  PL_DECLARE_ABSTRACT_COMPONENT_TYPE(plJoltActorComponent, plComponent);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltActorComponent

public:
  plJoltActorComponent();
  ~plJoltActorComponent();

  /// \brief The collision layer determines with which other actors this actor collides.
  ///
  /// Which collision layers collide with each other is configured through the plCollisionFilterConfig.
  /// \see plJoltCollisionFiltering::GetCollisionFilterConfig()
  plUInt8 m_uiCollisionLayer = 0; // [ property ]

  /// \brief Sets the object filter ID to use. This can only be set right after creation, before the component gets activated.
  void SetInitialObjectFilterID(plUInt32 uiObjectFilterID);

  /// \brief The object filter ID can be used to ignore collisions specifically with this one object.
  plUInt32 GetObjectFilterID() const { return m_uiObjectFilterID; }

protected:
  const plJoltUserData* GetUserData() const;

  void ExtractSubShapeGeometry(const plGameObject* pObject, plMsgExtractGeometry& msg) const;

  static void GatherShapes(plDynamicArray<plJoltSubShape>& shapes, plGameObject* pObject, const plTransform& rootTransform, float fDensity, const plJoltMaterial* pMaterial);
  plResult CreateShape(JPH::BodyCreationSettings* pSettings, float fDensity, const plJoltMaterial* pMaterial);

  virtual void CreateShapes(plDynamicArray<plJoltSubShape>& out_Shapes, const plTransform& rootTransform, float fDensity, const plJoltMaterial* pMaterial) {}

  plUInt32 m_uiUserDataIndex = plInvalidIndex;
  plUInt32 m_uiJoltBodyID = plInvalidIndex;
  plUInt32 m_uiObjectFilterID = plInvalidIndex;
};
