#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetManager.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetWindow.moc.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMeshAssetDocumentManager, 1, plRTTIDefaultAllocator<plMeshAssetDocumentManager>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plMeshAssetDocumentManager::plMeshAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plMeshAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  plAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Mesh_Static", "plMesh");

  m_DocTypeDesc.m_sDocumentTypeName = "Mesh";
  m_DocTypeDesc.m_sFileExtension = "plMeshAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Mesh.svg";
  m_DocTypeDesc.m_sAssetCategory = "Rendering";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plMeshAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Mesh_Static");

  m_DocTypeDesc.m_sResourceFileExtension = "plMesh";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::SupportsThumbnail;
}

plMeshAssetDocumentManager::~plMeshAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plMeshAssetDocumentManager::OnDocumentManagerEvent, this));
}

plResult plMeshAssetDocumentManager::OpenPickedDocument(const plDocumentObject* pPickedComponent, plUInt32 uiPartIndex)
{
  // check that we actually picked a mesh component
  if (!pPickedComponent->GetTypeAccessor().GetType()->IsDerivedFrom<plMeshComponent>())
    return PLASMA_FAILURE;

  // first try the materials array on the component itself, and see if we have a material override to pick
  if ((plInt32)uiPartIndex < pPickedComponent->GetTypeAccessor().GetCount("Materials"))
  {
    // access the material at the given index
    // this might be empty, though, in which case we still need to check the mesh asset
    const plVariant varMatGuid = pPickedComponent->GetTypeAccessor().GetValue("Materials", uiPartIndex);

    // if it were anything else than a string that would be weird
    PLASMA_ASSERT_DEV(varMatGuid.IsA<plString>(), "Material override property is not a string type");

    if (varMatGuid.IsA<plString>())
    {
      if (TryOpenAssetDocument(varMatGuid.Get<plString>()).Succeeded())
        return PLASMA_SUCCESS;
    }
  }

  // couldn't open it through the override, so we now need to inspect the mesh asset
  const plVariant varMeshGuid = pPickedComponent->GetTypeAccessor().GetValue("Mesh");

  PLASMA_ASSERT_DEV(varMeshGuid.IsA<plString>(), "Mesh property is not a string type");

  if (!varMeshGuid.IsA<plString>())
    return PLASMA_FAILURE;

  // we don't support non-guid mesh asset references, because I'm too lazy
  if (!plConversionUtils::IsStringUuid(varMeshGuid.Get<plString>()))
    return PLASMA_FAILURE;

  const plUuid meshGuid = plConversionUtils::ConvertStringToUuid(varMeshGuid.Get<plString>());

  auto pSubAsset = plAssetCurator::GetSingleton()->GetSubAsset(meshGuid);

  // unknown mesh asset
  if (!pSubAsset)
    return PLASMA_FAILURE;

  // now we need to open the mesh and we cannot wait for it (usually that is queued for GUI reasons)
  // though we do not want a window
  plMeshAssetDocument* pMeshDoc =
    static_cast<plMeshAssetDocument*>(plQtEditorApp::GetSingleton()->OpenDocument(pSubAsset->m_pAssetInfo->m_sAbsolutePath, plDocumentFlags::None));

  if (!pMeshDoc)
    return PLASMA_FAILURE;

  plResult result = PLASMA_FAILURE;

  // if we are outside the stored index, tough luck
  if (uiPartIndex < pMeshDoc->GetProperties()->m_Slots.GetCount())
  {
    result = TryOpenAssetDocument(pMeshDoc->GetProperties()->m_Slots[uiPartIndex].m_sResource);
  }

  // make sure to close the document again, if we were the ones to open it
  // otherwise keep it open
  if (!pMeshDoc->HasWindowBeenRequested())
    pMeshDoc->GetDocumentManager()->CloseDocument(pMeshDoc);

  return result;
}

void plMeshAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plMeshAssetDocument>())
      {
        plQtMeshAssetDocumentWindow* pDocWnd = new plQtMeshAssetDocumentWindow(static_cast<plMeshAssetDocument*>(e.m_pDocument));
      }
    }
    break;

    default:
      break;
  }
}

void plMeshAssetDocumentManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plMeshAssetDocument(szPath);
}

void plMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
