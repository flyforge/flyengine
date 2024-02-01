#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/Curve1DAsset/Curve1DAsset.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetManager.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetWindow.moc.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCurve1DAssetDocumentManager, 1, plRTTIDefaultAllocator<plCurve1DAssetDocumentManager>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plCurve1DAssetDocumentManager::plCurve1DAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plCurve1DAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Curve1D";
  m_DocTypeDesc.m_sFileExtension = "plCurve1DAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Curve1D.svg";
  m_DocTypeDesc.m_sAssetCategory = "Utilities";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plCurve1DAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Data_Curve");

  m_DocTypeDesc.m_sResourceFileExtension = "plCurve1D";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::AutoTransformOnSave | plAssetDocumentFlags::SupportsThumbnail;
}

plCurve1DAssetDocumentManager::~plCurve1DAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plCurve1DAssetDocumentManager::OnDocumentManagerEvent, this));
}

void plCurve1DAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plCurve1DAssetDocument>())
      {
        new plQtCurve1DAssetDocumentWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void plCurve1DAssetDocumentManager::InternalCreateDocument(
  plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plCurve1DAssetDocument(sPath);
}

void plCurve1DAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
