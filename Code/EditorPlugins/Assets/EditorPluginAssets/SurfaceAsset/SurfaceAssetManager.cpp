#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SurfaceAsset/SurfaceAsset.h>
#include <EditorPluginAssets/SurfaceAsset/SurfaceAssetManager.h>
#include <EditorPluginAssets/SurfaceAsset/SurfaceAssetWindow.moc.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSurfaceAssetDocumentManager, 1, plRTTIDefaultAllocator<plSurfaceAssetDocumentManager>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plSurfaceAssetDocumentManager::plSurfaceAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plSurfaceAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Surface";
  m_DocTypeDesc.m_sFileExtension = "plSurfaceAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Surface.svg";
  m_DocTypeDesc.m_sAssetCategory = "Utilities";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plSurfaceAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Surface");

  m_DocTypeDesc.m_sResourceFileExtension = "plSurface";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::AutoTransformOnSave;

  plQtImageCache::GetSingleton()->RegisterTypeImage("Surface", QPixmap(":/AssetIcons/Surface.svg"));
}

plSurfaceAssetDocumentManager::~plSurfaceAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plSurfaceAssetDocumentManager::OnDocumentManagerEvent, this));
}

void plSurfaceAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plSurfaceAssetDocument>())
      {
        plQtSurfaceAssetDocumentWindow* pDocWnd = new plQtSurfaceAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;

    default:
      break;
  }
}

void plSurfaceAssetDocumentManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plSurfaceAssetDocument(szPath);
}

void plSurfaceAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
