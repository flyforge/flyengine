#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetManager.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetWindow.moc.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plKrautTreeAssetDocumentManager, 1, plRTTIDefaultAllocator<plKrautTreeAssetDocumentManager>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plKrautTreeAssetDocumentManager::plKrautTreeAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plKrautTreeAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Kraut Tree";
  m_DocTypeDesc.m_sFileExtension = "plKrautTreeAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Kraut_Tree.svg";
  m_DocTypeDesc.m_sAssetCategory = "Terrain";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plKrautTreeAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Kraut_Tree");

  m_DocTypeDesc.m_sResourceFileExtension = "plKrautTree";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::SupportsThumbnail;
}

plKrautTreeAssetDocumentManager::~plKrautTreeAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plKrautTreeAssetDocumentManager::OnDocumentManagerEvent, this));
}

void plKrautTreeAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plKrautTreeAssetDocument>())
      {
        new plQtKrautTreeAssetDocumentWindow(static_cast<plKrautTreeAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void plKrautTreeAssetDocumentManager::InternalCreateDocument(
  plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plKrautTreeAssetDocument(sPath);
}

void plKrautTreeAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
