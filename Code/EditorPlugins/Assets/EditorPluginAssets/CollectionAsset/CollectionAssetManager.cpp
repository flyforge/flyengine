#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/CollectionAsset/CollectionAsset.h>
#include <EditorPluginAssets/CollectionAsset/CollectionAssetManager.h>
#include <EditorPluginAssets/CollectionAsset/CollectionAssetWindow.moc.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCollectionAssetDocumentManager, 1, plRTTIDefaultAllocator<plCollectionAssetDocumentManager>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plCollectionAssetDocumentManager::plCollectionAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plCollectionAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Collection";
  m_DocTypeDesc.m_sFileExtension = "plCollectionAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Collection.svg";
  m_DocTypeDesc.m_sAssetCategory = "Utilities";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plCollectionAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_AssetCollection");

  m_DocTypeDesc.m_sResourceFileExtension = "plCollection";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::AutoTransformOnSave;

  plQtImageCache::GetSingleton()->RegisterTypeImage("Collection", QPixmap(":/AssetIcons/Collection.svg"));
}

plCollectionAssetDocumentManager::~plCollectionAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plCollectionAssetDocumentManager::OnDocumentManagerEvent, this));
}


void plCollectionAssetDocumentManager::GetAssetTypesRequiringTransformForSceneExport(plSet<plTempHashedString>& inout_assetTypes)
{
  inout_assetTypes.Insert(plTempHashedString(m_DocTypeDesc.m_sDocumentTypeName));
}

void plCollectionAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plCollectionAssetDocument>())
      {
        new plQtCollectionAssetDocumentWindow(e.m_pDocument); // NOLINT: not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void plCollectionAssetDocumentManager::InternalCreateDocument(
  plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plCollectionAssetDocument(sPath);
}

void plCollectionAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
