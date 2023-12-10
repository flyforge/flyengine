#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/ImageDataAsset/ImageDataAsset.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetManager.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetWindow.moc.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plImageDataAssetDocumentManager, 1, plRTTIDefaultAllocator<plImageDataAssetDocumentManager>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plImageDataAssetDocumentManager::plImageDataAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plImageDataAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Image Data";
  m_DocTypeDesc.m_sFileExtension = "plImageDataAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/ImageData.svg";
  m_DocTypeDesc.m_sAssetCategory = "Utilities";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plImageDataAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_sResourceFileExtension = "plImageData";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::AutoThumbnailOnTransform;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Data_2D");
}

plImageDataAssetDocumentManager::~plImageDataAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plImageDataAssetDocumentManager::OnDocumentManagerEvent, this));
}

void plImageDataAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plImageDataAssetDocument>())
      {
        new plQtImageDataAssetDocumentWindow(static_cast<plImageDataAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void plImageDataAssetDocumentManager::InternalCreateDocument(plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  plImageDataAssetDocument* pDoc = new plImageDataAssetDocument(sPath);
  out_pDocument = pDoc;
}

void plImageDataAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
