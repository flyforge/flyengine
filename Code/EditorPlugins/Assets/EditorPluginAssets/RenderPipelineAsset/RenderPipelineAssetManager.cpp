#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAssetManager.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAssetWindow.moc.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRenderPipelineAssetManager, 1, plRTTIDefaultAllocator<plRenderPipelineAssetManager>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plRenderPipelineAssetManager::plRenderPipelineAssetManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plRenderPipelineAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "RenderPipeline";
  m_DocTypeDesc.m_sFileExtension = "plRenderPipelineAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/RenderPipeline.svg";
  m_DocTypeDesc.m_sAssetCategory = "Rendering";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plRenderPipelineAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_RenderPipeline");

  m_DocTypeDesc.m_sResourceFileExtension = "plRenderPipelineBin";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::AutoTransformOnSave;

  plQtImageCache::GetSingleton()->RegisterTypeImage("RenderPipeline", QPixmap(":/AssetIcons/RenderPipeline.svg"));
}

plRenderPipelineAssetManager::~plRenderPipelineAssetManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plRenderPipelineAssetManager::OnDocumentManagerEvent, this));
}

void plRenderPipelineAssetManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plRenderPipelineAssetDocument>())
      {
        plQtRenderPipelineAssetDocumentWindow* pDocWnd = new plQtRenderPipelineAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;

    default:
      break;
  }
}

void plRenderPipelineAssetManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plRenderPipelineAssetDocument(szPath);
}

void plRenderPipelineAssetManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
