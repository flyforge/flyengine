#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAssetManager.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimationClipAssetDocumentManager, 1, plRTTIDefaultAllocator<plAnimationClipAssetDocumentManager>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plAnimationClipAssetDocumentManager::plAnimationClipAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plAnimationClipAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Animation Clip";
  m_DocTypeDesc.m_sFileExtension = "plAnimationClipAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Animation_Clip.svg";
  m_DocTypeDesc.m_sAssetCategory = "Animation";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plAnimationClipAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Keyframe_Animation");

  m_DocTypeDesc.m_sResourceFileExtension = "plAnimationClip";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::SupportsThumbnail;

  //plQtImageCache::GetSingleton()->RegisterTypeImage("Animation Clip", QPixmap(":/AssetIcons/Animation_Clip.png"));
}

plAnimationClipAssetDocumentManager::~plAnimationClipAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plAnimationClipAssetDocumentManager::OnDocumentManagerEvent, this));
}

void plAnimationClipAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plAnimationClipAssetDocument>())
      {
        plQtAnimationClipAssetDocumentWindow* pDocWnd =
          new plQtAnimationClipAssetDocumentWindow(static_cast<plAnimationClipAssetDocument*>(e.m_pDocument));
      }
    }
    break;

    default:
      break;
  }
}

void plAnimationClipAssetDocumentManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plAnimationClipAssetDocument(szPath);
}

void plAnimationClipAssetDocumentManager::InternalGetSupportedDocumentTypes(
  plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
