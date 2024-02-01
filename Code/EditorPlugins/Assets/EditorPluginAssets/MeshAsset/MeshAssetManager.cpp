#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetManager.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetWindow.moc.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMeshAssetDocumentManager, 1, plRTTIDefaultAllocator<plMeshAssetDocumentManager>)
PL_END_DYNAMIC_REFLECTED_TYPE;

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
    return PL_FAILURE;

  // first try the materials array on the component itself, and see if we have a material override to pick
  if ((plInt32)uiPartIndex < pPickedComponent->GetTypeAccessor().GetCount("Materials"))
  {
    // access the material at the given index
    // this might be empty, though, in which case we still need to check the mesh asset
    const plVariant varMatGuid = pPickedComponent->GetTypeAccessor().GetValue("Materials", uiPartIndex);

    // if it were anything else than a string that would be weird
    PL_ASSERT_DEV(varMatGuid.IsA<plString>(), "Material override property is not a string type");

    if (varMatGuid.IsA<plString>())
    {
      if (TryOpenAssetDocument(varMatGuid.Get<plString>()).Succeeded())
        return PL_SUCCESS;
    }
  }

  // couldn't open it through the override, so we now need to inspect the mesh asset
  const plVariant varMeshGuid = pPickedComponent->GetTypeAccessor().GetValue("Mesh");

  PL_ASSERT_DEV(varMeshGuid.IsA<plString>(), "Mesh property is not a string type");

  if (!varMeshGuid.IsA<plString>())
    return PL_FAILURE;

  // we don't support non-guid mesh asset references, because I'm too lazy
  if (!plConversionUtils::IsStringUuid(varMeshGuid.Get<plString>()))
    return PL_FAILURE;

  const plUuid meshGuid = plConversionUtils::ConvertStringToUuid(varMeshGuid.Get<plString>());

  auto pSubAsset = plAssetCurator::GetSingleton()->GetSubAsset(meshGuid);

  // unknown mesh asset
  if (!pSubAsset)
    return PL_FAILURE;

  // now we need to open the mesh and we cannot wait for it (usually that is queued for GUI reasons)
  // though we do not want a window
  plMeshAssetDocument* pMeshDoc =
    static_cast<plMeshAssetDocument*>(plQtEditorApp::GetSingleton()->OpenDocument(pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath(), plDocumentFlags::None));

  if (!pMeshDoc)
    return PL_FAILURE;

  plResult result = PL_FAILURE;

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
        new plQtMeshAssetDocumentWindow(static_cast<plMeshAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void plMeshAssetDocumentManager::InternalCreateDocument(
  plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plMeshAssetDocument(sPath);
}

void plMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
