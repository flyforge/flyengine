#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetManager.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetWindow.moc.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimatedMeshAssetDocumentManager, 1, plRTTIDefaultAllocator<plAnimatedMeshAssetDocumentManager>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plAnimatedMeshAssetDocumentManager::plAnimatedMeshAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plAnimatedMeshAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Animated Mesh";
  m_DocTypeDesc.m_sFileExtension = "plAnimatedMeshAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Animated_Mesh.svg";
  m_DocTypeDesc.m_sAssetCategory = "Rendering";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plAnimatedMeshAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Mesh_Static");
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Mesh_Skinned");

  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::SupportsThumbnail;
  m_DocTypeDesc.m_sResourceFileExtension = "plAnimatedMesh";
}

plAnimatedMeshAssetDocumentManager::~plAnimatedMeshAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plAnimatedMeshAssetDocumentManager::OnDocumentManagerEvent, this));
}

void plAnimatedMeshAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plAnimatedMeshAssetDocument>())
      {
        new plQtAnimatedMeshAssetDocumentWindow(static_cast<plAnimatedMeshAssetDocument*>(e.m_pDocument)); // NOLINT
      }
    }
    break;

    default:
      break;
  }
}

void plAnimatedMeshAssetDocumentManager::InternalCreateDocument(plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plAnimatedMeshAssetDocument(sPath);
}

void plAnimatedMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
