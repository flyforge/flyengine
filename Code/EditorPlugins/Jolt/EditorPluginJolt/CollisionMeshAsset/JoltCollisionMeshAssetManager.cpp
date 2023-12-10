#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetManager.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetWindow.moc.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plJoltCollisionMeshAssetDocumentManager, 1, plRTTIDefaultAllocator<plJoltCollisionMeshAssetDocumentManager>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plJoltCollisionMeshAssetDocumentManager::plJoltCollisionMeshAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plJoltCollisionMeshAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Jolt_Colmesh_Triangle";
  m_DocTypeDesc.m_sFileExtension = "plJoltCollisionMeshAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Jolt_Collision_Mesh.svg";
  m_DocTypeDesc.m_sAssetCategory = "Physics";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plJoltCollisionMeshAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Jolt_Colmesh_Triangle");

  m_DocTypeDesc.m_sResourceFileExtension = "plJoltMesh";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::SupportsThumbnail;

  m_DocTypeDesc2.m_sDocumentTypeName = "Jolt_Colmesh_Convex";
  m_DocTypeDesc2.m_sFileExtension = "plJoltConvexCollisionMeshAsset";
  m_DocTypeDesc2.m_sIcon = ":/AssetIcons/Jolt_Collision_Mesh_Convex.svg";
  m_DocTypeDesc2.m_sAssetCategory = "Physics";
  m_DocTypeDesc2.m_pDocumentType = plGetStaticRTTI<plJoltCollisionMeshAssetDocument>();
  m_DocTypeDesc2.m_pManager = this;
  m_DocTypeDesc2.m_CompatibleTypes.PushBack("CompatibleAsset_Jolt_Colmesh_Triangle"); // convex meshes can also be used as triangle meshes (concave)
  m_DocTypeDesc2.m_CompatibleTypes.PushBack("CompatibleAsset_Jolt_Colmesh_Convex");

  m_DocTypeDesc2.m_sResourceFileExtension = "plJoltMesh";
  m_DocTypeDesc2.m_AssetDocumentFlags = plAssetDocumentFlags::SupportsThumbnail;
}

plJoltCollisionMeshAssetDocumentManager::~plJoltCollisionMeshAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plJoltCollisionMeshAssetDocumentManager::OnDocumentManagerEvent, this));
}

void plJoltCollisionMeshAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plJoltCollisionMeshAssetDocument>())
      {
        new plQtJoltCollisionMeshAssetDocumentWindow(static_cast<plAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;
    default:
      break;
  }
}

void plJoltCollisionMeshAssetDocumentManager::InternalCreateDocument(plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  if (sDocumentTypeName.IsEqual("Jolt_Colmesh_Convex"))
  {
    out_pDocument = new plJoltCollisionMeshAssetDocument(sPath, true);
  }
  else
  {
    out_pDocument = new plJoltCollisionMeshAssetDocument(sPath, false);
  }
}

void plJoltCollisionMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
  inout_DocumentTypes.PushBack(&m_DocTypeDesc2);
}

plUInt64 plJoltCollisionMeshAssetDocumentManager::ComputeAssetProfileHashImpl(const plPlatformProfile* pAssetProfile) const
{
  // don't have any settings yet, but assets that generate profile specific output must not return 0 here
  return 1;
}
