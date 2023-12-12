#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAsset.h>
#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAssetManager.h>
#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAssetWindow.moc.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plBlackboardTemplateAssetDocumentManager, 1, plRTTIDefaultAllocator<plBlackboardTemplateAssetDocumentManager>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plBlackboardTemplateAssetDocumentManager::plBlackboardTemplateAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plBlackboardTemplateAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "BlackboardTemplate";
  m_DocTypeDesc.m_sFileExtension = "plBlackboardTemplateAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/BlackboardTemplate.svg";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plBlackboardTemplateAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_BlackboardTemplate");

  m_DocTypeDesc.m_sResourceFileExtension = "plBlackboardTemplate";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::AutoTransformOnSave;

  plQtImageCache::GetSingleton()->RegisterTypeImage("BlackboardTemplate", QPixmap(":/AssetIcons/BlackboardTemplate.svg"));
}

plBlackboardTemplateAssetDocumentManager::~plBlackboardTemplateAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plBlackboardTemplateAssetDocumentManager::OnDocumentManagerEvent, this));
}

void plBlackboardTemplateAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plBlackboardTemplateAssetDocument>())
      {
        new plQtBlackboardTemplateAssetDocumentWindow(e.m_pDocument); // NOLINT: not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void plBlackboardTemplateAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plBlackboardTemplateAssetDocument(szPath);
}

void plBlackboardTemplateAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
