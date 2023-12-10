#include <EditorPluginAmplitudeAudio/EditorPluginAmplitudeAudioPCH.h>

#include <EditorPluginAmplitudeAudio/Assets/AmplitudeAudioControlCollectionAsset.h>
#include <EditorPluginAmplitudeAudio/Assets/AmplitudeAudioControlCollectionAssetManager.h>
#include <EditorPluginAmplitudeAudio/Assets/AmplitudeAudioControlCollectionAssetWindow.moc.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioControlCollectionAssetDocumentManager, 1, plRTTIDefaultAllocator<plAmplitudeAudioControlCollectionAssetDocumentManager>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plAmplitudeAudioControlCollectionAssetDocumentManager::plAmplitudeAudioControlCollectionAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plAmplitudeAudioControlCollectionAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Audio Control Collection";
  m_DocTypeDesc.m_sFileExtension = "plAudioControlCollectionAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Audio_Control_Collection.svg";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plAmplitudeAudioControlCollectionAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_AmplitudeAudio_Audio_Control_Collection");

  m_DocTypeDesc.m_sResourceFileExtension = "plAudioSystemControls";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::AutoTransformOnSave;

  plQtImageCache::GetSingleton()->RegisterTypeImage("Audio Control Collection", QPixmap(":/AssetIcons/Audio_Control_Collection.svg"));
}

plAmplitudeAudioControlCollectionAssetDocumentManager::~plAmplitudeAudioControlCollectionAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plAmplitudeAudioControlCollectionAssetDocumentManager::OnDocumentManagerEvent, this));
}

void plAmplitudeAudioControlCollectionAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plAmplitudeAudioControlCollectionAssetDocument>())
      {
        auto* pDocWnd = new plQtAmplitudeAudioControlCollectionAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;

    default:
      break;
  }
}

void plAmplitudeAudioControlCollectionAssetDocumentManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plAmplitudeAudioControlCollectionAssetDocument(szPath);
}

void plAmplitudeAudioControlCollectionAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}

#if defined(AM_WINDOWS_VERSION)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

PLASMA_STATICLINK_FILE(EditorPluginAmplitudeAudio, EditorPluginAmplitudeAudio_Assets_AmplitudeAudioControlCollectionAssetManager);
