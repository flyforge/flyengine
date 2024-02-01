#pragma once

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/Component.h>
#include <JoltPlugin/JoltPluginDLL.h>

struct plMsgExtractGeometry;
struct plMsgUpdateLocalBounds;
class plJoltUserData;
class plJoltMaterial;

namespace JPH
{
  class Shape;
}

struct plJoltSubShape
{
  JPH::Shape* m_pShape = nullptr;
  plTransform m_Transform = plTransform::MakeIdentity();
};

/// \brief Base class for all Jolt physics shapes.
///
/// A physics shape is used to represent (part of) a physical actor, such as a box or sphere,
/// which is used for the rigid body simulation.
///
/// When an actor is created, it searches for plJoltShapeComponent on its own object and all child objects.
/// It then adds all these shapes, with their respective transforms, to the Jolt actor.
class PL_JOLTPLUGIN_DLL plJoltShapeComponent : public plComponent
{
  PL_DECLARE_ABSTRACT_COMPONENT_TYPE(plJoltShapeComponent, plComponent);


  //////////////////////////////////////////////////////////////////////////
  // plComponent

protected:
  virtual void Initialize() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // plJoltShapeComponent

public:
  plJoltShapeComponent();
  ~plJoltShapeComponent();

  /// \brief If overridden, a triangular representation of the physics shape is added to the geometry object.
  ///
  /// This may be used for debug visualization or navmesh generation (though there are also other ways to do those).
  virtual void ExtractGeometry(plMsgExtractGeometry& ref_msg) const {}

protected:
  friend class plJoltActorComponent;
  virtual void CreateShapes(plDynamicArray<plJoltSubShape>& out_Shapes, const plTransform& rootTransform, float fDensity, const plJoltMaterial* pMaterial) = 0;

  const plJoltUserData* GetUserData();
  plUInt32 GetUserDataIndex();

  plUInt32 m_uiUserDataIndex = plInvalidIndex;
};
