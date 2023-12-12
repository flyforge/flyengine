#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/RendererCoreDLL.h>

using plSurfaceResourceHandle = plTypedResourceHandle<class plSurfaceResource>;

struct plSkeletonResourceGeometry
{
  // scale is used to resize a unit sphere / box / capsule
  plTransform m_Transform;
  plUInt16 m_uiAttachedToJoint = 0;
  plEnum<plSkeletonJointGeometryType> m_Type;

  // for convex geometry
  plDynamicArray<plVec3> m_VertexPositions;
  plDynamicArray<plUInt8> m_TriangleIndices;
};

struct PLASMA_RENDERERCORE_DLL plSkeletonResourceDescriptor
{
  plSkeletonResourceDescriptor();
  ~plSkeletonResourceDescriptor();
  plSkeletonResourceDescriptor(const plSkeletonResourceDescriptor& rhs) = delete;
  plSkeletonResourceDescriptor(plSkeletonResourceDescriptor&& rhs);
  void operator=(plSkeletonResourceDescriptor&& rhs);
  void operator=(const plSkeletonResourceDescriptor& rhs) = delete;

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);

  plUInt64 GetHeapMemoryUsage() const;

  plTransform m_RootTransform = plTransform::IdentityTransform();
  plSkeleton m_Skeleton;
  float m_fMaxImpulse = plMath::HighValue<float>();

  plDynamicArray<plSkeletonResourceGeometry> m_Geometry;
};

using plSkeletonResourceHandle = plTypedResourceHandle<class plSkeletonResource>;

class PLASMA_RENDERERCORE_DLL plSkeletonResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSkeletonResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plSkeletonResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plSkeletonResource, plSkeletonResourceDescriptor);

public:
  plSkeletonResource();
  ~plSkeletonResource();

  const plSkeletonResourceDescriptor& GetDescriptor() const { return *m_pDescriptor; }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  plUniquePtr<plSkeletonResourceDescriptor> m_pDescriptor;
};
