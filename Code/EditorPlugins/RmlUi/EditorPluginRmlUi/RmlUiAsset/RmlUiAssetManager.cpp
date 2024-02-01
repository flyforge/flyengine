#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetManager.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetWindow.moc.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRmlUiAssetDocumentManager, 1, plRTTIDefaultAllocator<plRmlUiAssetDocumentManager>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plRmlUiAssetDocumentManager::plRmlUiAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plRmlUiAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "RmlUi";
  m_DocTypeDesc.m_sFileExtension = "plRmlUiAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/RmlUi.svg";
  m_DocTypeDesc.m_sAssetCategory = "Input";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plRmlUiAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Rml_UI");

  m_DocTypeDesc.m_sResourceFileExtension = "plRmlUi";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::SupportsThumbnail;
}

plRmlUiAssetDocumentManager::~plRmlUiAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plRmlUiAssetDocumentManager::OnDocumentManagerEvent, this));
}

void plRmlUiAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plRmlUiAssetDocument>())
      {
        new plQtRmlUiAssetDocumentWindow(static_cast<plRmlUiAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void plRmlUiAssetDocumentManager::InternalCreateDocument(
  plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plRmlUiAssetDocument(sPath);
}

void plRmlUiAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
