#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <JoltPlugin/JoltPluginDLL.h>

struct plMsgAnimationPoseUpdated;

using plSkeletonResourceHandle = plTypedResourceHandle<class plSkeletonResource>;

using plJoltBoneColliderComponentManager = plComponentManager<class plJoltBoneColliderComponent, plBlockStorageType::Compact>;

class PLASMA_JOLTPLUGIN_DLL plJoltBoneColliderComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJoltBoneColliderComponent, plComponent, plJoltBoneColliderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltBoneColliderComponent

public:
  plJoltBoneColliderComponent();
  ~plJoltBoneColliderComponent();

  plUInt32 GetObjectFilterID() const { return m_uiObjectFilterID; } // [ scriptable ]

  bool m_bQueryShapeOnly = true; // [ property ]
  plTime m_UpdateThreshold;      // [ property ]

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
