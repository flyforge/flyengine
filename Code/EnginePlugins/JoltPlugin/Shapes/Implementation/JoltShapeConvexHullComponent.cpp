#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/Shapes/JoltShapeConvexHullComponent.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plJoltShapeConvexHullComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("CollisionMesh", GetMeshFile, SetMeshFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Jolt_Colmesh_Convex", plDependencyFlags::Package)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltShapeConvexHullComponent::plJoltShapeConvexHullComponent() = default;
plJoltShapeConvexHullComponent::~plJoltShapeConvexHullComponent() = default;

void plJoltShapeConvexHullComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_hCollisionMesh;
}

void plJoltShapeConvexHullComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_hCollisionMesh;
}

void plJoltShapeConvexHullComponent::SetMeshFile(const char* szFile)
{
  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    m_hCollisionMesh = plResourceManager::LoadResource<plJoltMeshResource>(szFile);
  }
}

const char* plJoltShapeConvexHullComponent::GetMeshFile() const
{
  if (!m_hCollisionMesh.IsValid())
    return "";

  return m_hCollisionMesh.GetResourceID();
}

void plJoltShapeConvexHullComponent::CreateShapes(plDynamicArray<plJoltSubShape>& out_Shapes, const plTransform& rootTransform, float fDensity, const plJoltMaterial* pMaterial)
{
  if (!m_hCollisionMesh.IsValid())
  {
    plLog::Warning("plJoltShapeConvexHullComponent '{0}' has no collision mesh set.", GetOwner()->GetName());
    return;
  }

  plResourceLock<plJoltMeshResource> pMesh(m_hCollisionMesh, plResourceAcquireMode::BlockTillLoaded);

  if (pMesh->GetNumConvexParts() == 0)
  {
    plLog::Warning("plJoltShapeConvexHullComponent '{0}' has a collision mesh set that does not contain a convex mesh: '{1}' ('{2}')", GetOwner()->GetName(), pMesh->GetResourceID(), pMesh->GetResourceDescription());
    return;
  }

  for (plUInt32 i = 0; i < pMesh->GetNumConvexParts(); ++i)
  {
    auto pShape = pMesh->InstantiateConvexPart(i, reinterpret_cast<plUInt64>(GetUserData()), pMaterial, fDensity);

    plJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
    sub.m_pShape = pShape;
    sub.m_Transform.SetLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
  }
}

void plJoltShapeConvexHullComponent::ExtractGeometry(plMsgExtractGeometry& ref_msg) const
{
  if (ref_msg.m_Mode != plWorldGeoExtractionUtil::ExtractionMode::CollisionMesh && ref_msg.m_Mode != plWorldGeoExtractionUtil::ExtractionMode::NavMeshGeneration)
    return;

  if (m_hCollisionMesh.IsValid())
  {
    plResourceLock<plJoltMeshResource> pMesh(m_hCollisionMesh, plResourceAcquireMode::BlockTillLoaded);

    ref_msg.AddMeshObject(GetOwner()->GetGlobalTransform(), pMesh->ConvertToCpuMesh());
  }
}


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeConvexHullComponent);

