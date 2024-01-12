#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plJoltStaticActorComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("CollisionMesh", GetMeshFile, SetMeshFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Jolt_Colmesh_Triangle", plDependencyFlags::Package)),
    PLASMA_MEMBER_PROPERTY("IncludeInNavmesh", m_bIncludeInNavmesh)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("PullSurfacesFromGraphicsMesh", m_bPullSurfacesFromGraphicsMesh),
    PLASMA_ACCESSOR_PROPERTY("Surface", GetSurfaceFile, SetSurfaceFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface", plDependencyFlags::Package)),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgExtractGeometry, OnMsgExtractGeometry),
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltStaticActorComponent::plJoltStaticActorComponent()
{
  m_uiJoltBodyID = JPH::BodyID::cInvalidBodyID;
}

plJoltStaticActorComponent::~plJoltStaticActorComponent() = default;

void plJoltStaticActorComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hCollisionMesh;
  s << m_bIncludeInNavmesh;
  s << m_bPullSurfacesFromGraphicsMesh;
  s << m_hSurface;
}


void plJoltStaticActorComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hCollisionMesh;
  s >> m_bIncludeInNavmesh;
  s >> m_bPullSurfacesFromGraphicsMesh;
  s >> m_hSurface;
}

void plJoltStaticActorComponent::OnDeactivated()
{
  m_UsedSurfaces.Clear();

  SUPER::OnDeactivated();
}

void plJoltStaticActorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();

  plJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  auto* pMaterial = GetJoltMaterial();

  JPH::BodyCreationSettings bodyCfg;
  if (CreateShape(&bodyCfg, 1.0f, pMaterial).Failed())
  {
    plLog::Error("Jolt static actor component '{}' has no valid shape.", GetOwner()->GetName());
    return;
  }

  const plSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  if (pMaterial == nullptr)
    pMaterial = plJoltCore::GetDefaultMaterial();

  bodyCfg.mPosition = plJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation = plJoltConversionUtils::ToQuat(trans.m_Rotation);
  bodyCfg.mMotionType = JPH::EMotionType::Static;
  bodyCfg.mObjectLayer = plJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, plJoltBroadphaseLayer::Static);
  bodyCfg.mRestitution = pMaterial->m_fRestitution;
  bodyCfg.mFriction = pMaterial->m_fFriction;
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
  bodyCfg.mUserData = reinterpret_cast<plUInt64>(pUserData);

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody, true);
}

void plJoltStaticActorComponent::CreateShapes(plDynamicArray<plJoltSubShape>& out_Shapes, const plTransform& rootTransform, float fDensity, const plJoltMaterial* pMaterial)
{
  if (!m_hCollisionMesh.IsValid())
    return;

  plResourceLock<plJoltMeshResource> pMesh(m_hCollisionMesh, plResourceAcquireMode::BlockTillLoaded);

  if (pMesh->GetNumConvexParts() > 0)
  {
    for (plUInt32 i = 0; i < pMesh->GetNumConvexParts(); ++i)
    {
      auto pShape = pMesh->InstantiateConvexPart(i, reinterpret_cast<plUInt64>(GetUserData()), pMaterial, fDensity);

      plJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
      sub.m_pShape = pShape;
      sub.m_Transform.SetLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
    }
  }

  if (auto pTriMesh = pMesh->HasTriangleMesh())
  {
    plHybridArray<const plJoltMaterial*, 32> materials;

    if (pMaterial != nullptr)
    {
      materials.SetCount(pMesh->GetSurfaces().GetCount(), pMaterial);
    }

    if (m_bPullSurfacesFromGraphicsMesh)
    {
      materials.SetCount(pMesh->GetSurfaces().GetCount());
      PullSurfacesFromGraphicsMesh(materials);
    }

    auto pNewShape = pMesh->InstantiateTriangleMesh(reinterpret_cast<plUInt64>(GetUserData()), materials);

    plJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
    sub.m_pShape = pNewShape;
    sub.m_Transform.SetLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
  }
}

void plJoltStaticActorComponent::PullSurfacesFromGraphicsMesh(plDynamicArray<const plJoltMaterial*>& ref_materials)
{
  // the materials don't hold a handle to the surfaces, so they don't keep them alive
  // therefore, we need to keep them alive by storing a handle
  m_UsedSurfaces.Clear();

  plMeshComponent* pMeshComp;
  if (!GetOwner()->TryGetComponentOfBaseType(pMeshComp))
    return;

  auto hMeshRes = pMeshComp->GetMesh();
  if (!hMeshRes.IsValid())
    return;

  plResourceLock<plMeshResource> pMeshRes(hMeshRes, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pMeshRes.GetAcquireResult() != plResourceAcquireResult::Final)
    return;

  if (pMeshRes->GetMaterials().GetCount() != ref_materials.GetCount())
    return;

  const plUInt32 uiNumMats = ref_materials.GetCount();
  m_UsedSurfaces.SetCount(uiNumMats);

  for (plUInt32 s = 0; s < uiNumMats; ++s)
  {
    // first check whether the component has a material override
    auto hMat = pMeshComp->GetMaterial(s);

    if (!hMat.IsValid())
    {
      // otherwise ask the mesh resource about the material
      hMat = pMeshRes->GetMaterials()[s];
    }

    if (!hMat.IsValid())
      continue;

    plResourceLock<plMaterialResource> pMat(hMat, plResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pMat.GetAcquireResult() != plResourceAcquireResult::Final)
      continue;

    if (pMat->GetSurface().IsEmpty())
      continue;

    m_UsedSurfaces[s] = plResourceManager::LoadResource<plSurfaceResource>(pMat->GetSurface().GetString());

    plResourceLock<plSurfaceResource> pSurface(m_UsedSurfaces[s], plResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pSurface.GetAcquireResult() != plResourceAcquireResult::Final)
      continue;

    PLASMA_ASSERT_DEV(pSurface->m_pPhysicsMaterialJolt != nullptr, "Invalid Jolt material pointer on surface");
    ref_materials[s] = static_cast<plJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
  }
}

void plJoltStaticActorComponent::OnMsgExtractGeometry(plMsgExtractGeometry& msg) const
{
  if (msg.m_Mode == plWorldGeoExtractionUtil::ExtractionMode::CollisionMesh || (msg.m_Mode == plWorldGeoExtractionUtil::ExtractionMode::NavMeshGeneration && m_bIncludeInNavmesh))
  {
    if (m_hCollisionMesh.IsValid())
    {
      plResourceLock<plJoltMeshResource> pMesh(m_hCollisionMesh, plResourceAcquireMode::BlockTillLoaded);

      msg.AddMeshObject(GetOwner()->GetGlobalTransform(), pMesh->ConvertToCpuMesh());
    }

    ExtractSubShapeGeometry(GetOwner(), msg);
  }
}

void plJoltStaticActorComponent::SetMeshFile(const char* szFile)
{
  plJoltMeshResourceHandle hMesh;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hMesh = plResourceManager::LoadResource<plJoltMeshResource>(szFile);
  }

  SetMesh(hMesh);
}

const char* plJoltStaticActorComponent::GetMeshFile() const
{
  if (!m_hCollisionMesh.IsValid())
    return "";

  return m_hCollisionMesh.GetResourceID();
}

void plJoltStaticActorComponent::SetMesh(const plJoltMeshResourceHandle& hMesh)
{
  m_hCollisionMesh = hMesh;
}

const plJoltMaterial* plJoltStaticActorComponent::GetJoltMaterial() const
{
  if (m_hSurface.IsValid())
  {
    plResourceLock<plSurfaceResource> pSurface(m_hSurface, plResourceAcquireMode::BlockTillLoaded);

    if (pSurface->m_pPhysicsMaterialJolt != nullptr)
    {
      return static_cast<plJoltMaterial*>(pSurface->m_pPhysicsMaterialJolt);
    }
  }

  return nullptr;
}

void plJoltStaticActorComponent::SetSurfaceFile(const char* szFile)
{
  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    m_hSurface = plResourceManager::LoadResource<plSurfaceResource>(szFile);
  }

  if (m_hSurface.IsValid())
    plResourceManager::PreloadResource(m_hSurface);
}

const char* plJoltStaticActorComponent::GetSurfaceFile() const
{
  if (!m_hSurface.IsValid())
    return "";

  return m_hSurface.GetResourceID();
}


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltStaticActorComponent);

