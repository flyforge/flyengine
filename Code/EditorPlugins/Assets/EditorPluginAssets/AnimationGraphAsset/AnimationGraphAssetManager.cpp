#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAsset.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAssetManager.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAssetWindow.moc.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimationGraphAssetManager, 1, plRTTIDefaultAllocator<plAnimationGraphAssetManager>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plAnimationGraphAssetManager::plAnimationGraphAssetManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plAnimationGraphAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Animation Graph";
  m_DocTypeDesc.m_sFileExtension = "plAnimationGraphAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/AnimationGraph.svg";
  m_DocTypeDesc.m_sAssetCategory = "Animation";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plAnimationGraphAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Keyframe_Graph");

  m_DocTypeDesc.m_sResourceFileExtension = "plAnimGraphBin";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::AutoTransformOnSave;

  plQtImageCache::GetSingleton()->RegisterTypeImage("Animation Graph", QPixmap(":/AssetIcons/AnimationGraph.svg"));
}

plAnimationGraphAssetManager::~plAnimationGraphAssetManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plAnimationGraphAssetManager::OnDocumentManagerEvent, this));
}

void plAnimationGraphAssetManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plAnimationGraphAssetDocument>())
      {
        new plQtAnimationGraphAssetDocumentWindow(e.m_pDocument); // NOLINT: not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void plAnimationGraphAssetManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plAnimationGraphAssetDocument(szPath);
}

void plAnimationGraphAssetManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
