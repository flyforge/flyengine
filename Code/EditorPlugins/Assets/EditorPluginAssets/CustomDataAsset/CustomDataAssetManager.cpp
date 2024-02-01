#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/CustomDataAsset/CustomDataAsset.h>
#include <EditorPluginAssets/CustomDataAsset/CustomDataAssetManager.h>
#include <EditorPluginAssets/CustomDataAsset/CustomDataAssetWindow.moc.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCustomDataAssetDocumentManager, 1, plRTTIDefaultAllocator<plCustomDataAssetDocumentManager>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plCustomDataAssetDocumentManager::plCustomDataAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plCustomDataAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "CustomData";
  m_DocTypeDesc.m_sFileExtension = "plCustomDataAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/CustomData.svg";
  m_DocTypeDesc.m_sAssetCategory = "Logic";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plCustomDataAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_CustomData"); // \todo should only be compatible with same type

  m_DocTypeDesc.m_sResourceFileExtension = "plCustomData";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::AutoTransformOnSave;

  plQtImageCache::GetSingleton()->RegisterTypeImage("CustomData", QPixmap(":/AssetIcons/CustomData.svg"));
}

plCustomDataAssetDocumentManager::~plCustomDataAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plCustomDataAssetDocumentManager::OnDocumentManagerEvent, this));
}

void plCustomDataAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plCustomDataAssetDocument>())
      {
        new plQtCustomDataAssetDocumentWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void plCustomDataAssetDocumentManager::InternalCreateDocument(plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plCustomDataAssetDocument(sPath);
}

void plCustomDataAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
