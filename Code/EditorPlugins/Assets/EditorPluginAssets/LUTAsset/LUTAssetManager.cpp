#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/LUTAsset/LUTAsset.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetManager.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLUTAssetDocumentManager, 1, plRTTIDefaultAllocator<plLUTAssetDocumentManager>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plLUTAssetDocumentManager::plLUTAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plLUTAssetDocumentManager::OnDocumentManagerEvent, this));

  // LUT asset source files
  plAssetFileExtensionWhitelist::AddAssetFileExtension("LUT", "cube");
  plAssetFileExtensionWhitelist::AddAssetFileExtension("LUT", "dds");

  m_DocTypeDesc.m_sDocumentTypeName = "LUT";
  m_DocTypeDesc.m_sFileExtension = "plLUTAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/LUT.svg";
  m_DocTypeDesc.m_sAssetCategory = "Rendering";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plLUTAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_sResourceFileExtension = "plLUT";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::None;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_3D");

  plQtImageCache::GetSingleton()->RegisterTypeImage("LUT", QPixmap(":/AssetIcons/LUT.svg"));

  // plQtImageCache::GetSingleton()->RegisterTypeImage("LUT", QPixmap(":/AssetIcons/Render_Target.svg"));
}

plLUTAssetDocumentManager::~plLUTAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plLUTAssetDocumentManager::OnDocumentManagerEvent, this));
}

void plLUTAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plLUTAssetDocument>())
      {
        plQtLUTAssetDocumentWindow* pDocWnd = new plQtLUTAssetDocumentWindow(static_cast<plLUTAssetDocument*>(e.m_pDocument));
      }
    }
    break;

    default:
      break;
  }
}

void plLUTAssetDocumentManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  plLUTAssetDocument* pDoc = new plLUTAssetDocument(szPath);
  out_pDocument = pDoc;
}

void plLUTAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
