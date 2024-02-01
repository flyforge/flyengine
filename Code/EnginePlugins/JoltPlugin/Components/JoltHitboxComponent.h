#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <JoltPlugin/JoltPluginDLL.h>

struct plMsgAnimationPoseUpdated;

using plSkeletonResourceHandle = plTypedResourceHandle<class plSkeletonResource>;

using plJoltHitboxComponentManager = plComponentManager<class plJoltHitboxComponent, plBlockStorageType::Compact>;

/// \brief Adds physics shapes to an animated character for hit detection.
///
/// Attach this component to an animated mesh, to give it a physical representation.
/// The shapes for each bone are defined through the skeleton.
///
/// Typically these shapes are "query shapes" only, meaning they don't participate in the physical simulation,
/// so they won't push other objects aside.
/// They can only be detected through raycasts and scene queries (assuming those queries have the plPhysicsShapeType::Query flag set).
class PL_JOLTPLUGIN_DLL plJoltHitboxComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plJoltHitboxComponent, plComponent, plJoltHitboxComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltHitboxComponent

public:
  plJoltHitboxComponent();
  ~plJoltHitboxComponent();

  /// \brief The same object filter ID is assigned to all hit shapes.
  plUInt32 GetObjectFilterID() const { return m_uiObjectFilterID; } // [ scriptable ]

  /// \brief If true, shapes can only be detected with raycasts and scene queries. If false, they will be kinematic objects in the simulation and push other rigid bodies aside.
  bool m_bQueryShapeOnly = true; // [ property ]

  /// \brief At which interval to update the hitbox transforms. Set to zero for full updates every frame.
  plTime m_UpdateThreshold; // [ property ]

  /// \brief Updates the shape transforms to conform with the new pose, but only if the update threshold was exceeded.
  void OnAnimationPoseUpdated(plMsgAnimationPoseUpdated& ref_msg); // [ msg handler ]

  /// \brief Destroys the current shape objects and creates new ones.
  ///
  /// This can be used to update the shapes when for example the object was scaled.
  /// Be aware that all child objects that were attached to the previous physics shape objects will
  /// be deleted as well. So any 'attachments' will disappear.
  void RecreatePhysicsShapes(); // [ scriptable ]

protected:
  void CreatePhysicsShapes(const plSkeletonResourceHandle& hSkeleton);
  void DestroyPhysicsShapes();

  struct Shape
  {
    plUInt16 m_uiAttachedToBone = 0xFFFF;
    plVec3 m_vOffsetPos;
    plQuat m_qOffsetRot;

    plGameObjectHandle m_hActorObject;
  };

  plUInt32 m_uiObjectFilterID = plInvalidIndex;
  plTime m_LastUpdate;
  plDynamicArray<Shape> m_Shapes;
};
