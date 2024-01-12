#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Communication/Message.h>

struct plGameObjectHandle;
struct plSkeletonResourceDescriptor;

using plSurfaceResourceHandle = plTypedResourceHandle<class plSurfaceResource>;

/// \brief Classifies the facing of an individual raycast hit
enum class plPhysicsHitType : int8_t
{
  Undefined = -1,        ///< Returned if the respective physics binding does not provide this information
  TriangleFrontFace = 0, ///< The raycast hit the front face of a triangle
  TriangleBackFace = 1,  ///< The raycast hit the back face of a triangle
};

/// \brief Used for raycast and seep tests
struct plPhysicsCastResult
{
  plVec3 m_vPosition;
  plVec3 m_vNormal;
  float m_fDistance;

  plGameObjectHandle m_hShapeObject;                        ///< The game object to which the hit physics shape is attached.
  plGameObjectHandle m_hActorObject;                        ///< The game object to which the parent actor of the hit physics shape is attached.
  plSurfaceResourceHandle m_hSurface;                       ///< The type of surface that was hit (if available)
  plUInt32 m_uiObjectFilterID = plInvalidIndex;             ///< An ID either per object (rigid-body / ragdoll) or per shape (implementation specific) that can be used to ignore this object during raycasts and shape queries.
  plPhysicsHitType m_hitType = plPhysicsHitType::Undefined; ///< Classification of the triangle face, see plPhysicsHitType

  // Physics-engine specific information, may be available or not.
  void* m_pInternalPhysicsShape = nullptr;
  void* m_pInternalPhysicsActor = nullptr;
};

struct plPhysicsCastResultArray
{
  plHybridArray<plPhysicsCastResult, 16> m_Results;
};

/// \brief Used to report overlap query results
struct plPhysicsOverlapResult
{
  PLASMA_DECLARE_POD_TYPE();

  plGameObjectHandle m_hShapeObject;            ///< The game object to which the hit physics shape is attached.
  plGameObjectHandle m_hActorObject;            ///< The game object to which the parent actor of the hit physics shape is attached.
  plUInt32 m_uiObjectFilterID = plInvalidIndex; ///< The shape id of the hit physics shape
  plVec3 m_vCenterPosition;                     ///< The center position of the reported object in world space.

  // Physics-engine specific information, may be available or not.
  void* m_pInternalPhysicsShape = nullptr;
  void* m_pInternalPhysicsActor = nullptr;
};

struct plPhysicsOverlapResultArray
{
  plHybridArray<plPhysicsOverlapResult, 16> m_Results;
};

struct plPhysicsTriangle
{
  plVec3 m_Vertices[3];
  const plSurfaceResource* m_pSurface = nullptr;
};

/// \brief Flags for selecting which types of physics shapes should be included in things like overlap queries and raycasts.
///
/// This is mainly for optimization purposes. It is up to the physics integration to support some or all of these flags.
///
/// Note: If this is modified, 'Physics.ts' also has to be updated.
PLASMA_DECLARE_FLAGS_WITH_DEFAULT(plUInt32, plPhysicsShapeType, 0xFFFFFFFF,
  Static,    ///< Static geometry
  Dynamic,   ///< Dynamic and kinematic objects
  Query,     ///< Query shapes are kinematic bodies that don't participate in the simulation and are only used for raycasts and other queries.
  Trigger,   ///< Trigger shapes
  Character, ///< Shapes associated with character controllers.
  Ragdoll,   ///< All shapes belonging to ragdolls.
  Rope,      ///< All shapes belonging to ropes.
  Cloth      ///< Soft-body shapes. Mainly for decorative purposes.
);

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_CORE_DLL, plPhysicsShapeType);

struct plPhysicsQueryParameters
{
  plPhysicsQueryParameters() = default;
  explicit plPhysicsQueryParameters(plUInt32 uiCollisionLayer,
    plBitflags<plPhysicsShapeType> shapeTypes = plPhysicsShapeType::Default, plUInt32 uiIgnoreObjectFilterID = plInvalidIndex)
    : m_uiCollisionLayer(uiCollisionLayer)
    , m_ShapeTypes(shapeTypes)
    , m_uiIgnoreObjectFilterID(uiIgnoreObjectFilterID)
  {
  }

  plUInt32 m_uiCollisionLayer = 0;
  plBitflags<plPhysicsShapeType> m_ShapeTypes = plPhysicsShapeType::Default;
  plUInt32 m_uiIgnoreObjectFilterID = plInvalidIndex;
  bool m_bIgnoreInitialOverlap = false;
};

enum class plPhysicsHitCollection
{
  Closest,
  Any
};

class PLASMA_CORE_DLL plPhysicsWorldModuleInterface : public plWorldModule
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPhysicsWorldModuleInterface, plWorldModule);

protected:
  plPhysicsWorldModuleInterface(plWorld* pWorld)
    : plWorldModule(pWorld)
  {
  }

public:
  /// \brief Searches for a collision layer with the given name and returns its index.
  ///
  /// Returns plInvalidIndex if no such collision layer exists.
  virtual plUInt32 GetCollisionLayerByName(plStringView sName) const = 0;

  virtual bool Raycast(plPhysicsCastResult& out_result, const plVec3& vStart, const plVec3& vDir, float fDistance, const plPhysicsQueryParameters& params, plPhysicsHitCollection collection = plPhysicsHitCollection::Closest) const = 0;

  virtual bool RaycastAll(plPhysicsCastResultArray& out_results, const plVec3& vStart, const plVec3& vDir, float fDistance, const plPhysicsQueryParameters& params) const = 0;

  virtual bool SweepTestSphere(plPhysicsCastResult& out_result, float fSphereRadius, const plVec3& vStart, const plVec3& vDir, float fDistance, const plPhysicsQueryParameters& params, plPhysicsHitCollection collection = plPhysicsHitCollection::Closest) const = 0;

  virtual bool SweepTestBox(plPhysicsCastResult& out_result, plVec3 vBoxExtends, const plTransform& transform, const plVec3& vDir, float fDistance, const plPhysicsQueryParameters& params, plPhysicsHitCollection collection = plPhysicsHitCollection::Closest) const = 0;

  virtual bool SweepTestCapsule(plPhysicsCastResult& out_result, float fCapsuleRadius, float fCapsuleHeight, const plTransform& transform, const plVec3& vDir, float fDistance, const plPhysicsQueryParameters& params, plPhysicsHitCollection collection = plPhysicsHitCollection::Closest) const = 0;

  virtual bool OverlapTestSphere(float fSphereRadius, const plVec3& vPosition, const plPhysicsQueryParameters& params) const = 0;

  virtual bool OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const plTransform& transform, const plPhysicsQueryParameters& params) const = 0;

  virtual void QueryShapesInSphere(plPhysicsOverlapResultArray& out_results, float fSphereRadius, const plVec3& vPosition, const plPhysicsQueryParameters& params) const = 0;

  virtual plVec3 GetGravity() const = 0;

  virtual void QueryGeometryInBox(const plPhysicsQueryParameters& params, plBoundingBox box, plDynamicArray<plPhysicsTriangle>& out_triangles) const = 0;

  //////////////////////////////////////////////////////////////////////////
  // ABSTRACTION HELPERS
  //
  // These functions are used to be able to use certain physics functionality, without having a direct dependency on the exact implementation (Jolt / PhysX).
  // If no physics module is available, they simply do nothing.
  // Add functions on demand.

  /// \brief Adds a static actor with a box shape to pOwner.
  virtual void AddStaticCollisionBox(plGameObject* pOwner, plVec3 vBoxSize) {}

  struct JointConfig
  {
    plGameObjectHandle m_hActorA;
    plGameObjectHandle m_hActorB;
    plTransform m_LocalFrameA = plTransform::IdentityTransform();
    plTransform m_LocalFrameB = plTransform::IdentityTransform();
  };

  struct FixedJointConfig : JointConfig
  {
  };

  /// \brief Adds a fixed joint to pOwner.
  virtual void AddFixedJointComponent(plGameObject* pOwner, const plPhysicsWorldModuleInterface::FixedJointConfig& cfg) {}
};

/// \brief Used to apply a physical impulse on the object
struct PLASMA_CORE_DLL plMsgPhysicsAddImpulse : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgPhysicsAddImpulse, plMessage);

  plVec3 m_vGlobalPosition;
  plVec3 m_vImpulse;
  plUInt32 m_uiObjectFilterID = plInvalidIndex;

  // Physics-engine specific information, may be available or not.
  void* m_pInternalPhysicsShape = nullptr;
  void* m_pInternalPhysicsActor = nullptr;
};

/// \brief Used to apply a physical force on the object
struct PLASMA_CORE_DLL plMsgPhysicsAddForce : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgPhysicsAddForce, plMessage);

  plVec3 m_vGlobalPosition;
  plVec3 m_vForce;

  // Physics-engine specific information, may be available or not.
  void* m_pInternalPhysicsShape = nullptr;
  void* m_pInternalPhysicsActor = nullptr;
};

struct PLASMA_CORE_DLL plMsgPhysicsJointBroke : public plEventMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgPhysicsJointBroke, plEventMessage);

  plGameObjectHandle m_hJointObject;
};

/// \brief Sent by components such as plJoltGrabObjectComponent to indicate that the object has been grabbed or released.
struct PLASMA_CORE_DLL plMsgObjectGrabbed : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgObjectGrabbed, plMessage);

  plGameObjectHandle m_hGrabbedBy;
  bool m_bGotGrabbed = true;
};

/// \brief Send this to components such as plJoltGrabObjectComponent to demand that m_hGrabbedObjectToRelease should no longer be grabbed.
struct PLASMA_CORE_DLL plMsgReleaseObjectGrab : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgReleaseObjectGrab, plMessage);

  plGameObjectHandle m_hGrabbedObjectToRelease;
};

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Communication/Message.h>

struct PLASMA_CORE_DLL plSmcTriangle
{
  PLASMA_DECLARE_POD_TYPE();

  plUInt32 m_uiVertexIndices[3];
};

struct PLASMA_CORE_DLL plSmcSubMesh
{
  PLASMA_DECLARE_POD_TYPE();

  plUInt32 m_uiFirstTriangle = 0;
  plUInt32 m_uiNumTriangles = 0;
  plUInt16 m_uiSurfaceIndex = 0;
};

struct PLASMA_CORE_DLL plSmcDescription
{
  plDeque<plVec3> m_Vertices;
  plDeque<plSmcTriangle> m_Triangles;
  plDeque<plSmcSubMesh> m_SubMeshes;
  plDeque<plString> m_Surfaces;
};

struct PLASMA_CORE_DLL plMsgBuildStaticMesh : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgBuildStaticMesh, plMessage);

  /// \brief Append data to this description to add meshes to the automatic static mesh generation
  plSmcDescription* m_pStaticMeshDescription = nullptr;
};
