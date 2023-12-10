#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/StateMachineAsset/StateMachineAsset.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineAssetManager.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineAssetWindow.moc.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachineAssetManager, 1, plRTTIDefaultAllocator<plStateMachineAssetManager>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plStateMachineAssetManager::plStateMachineAssetManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plStateMachineAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "StateMachine";
  m_DocTypeDesc.m_sFileExtension = "plStateMachineAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/StateMachine.svg";
  m_DocTypeDesc.m_sAssetCategory = "Logic";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plStateMachineAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_StateMachine");

  m_DocTypeDesc.m_sResourceFileExtension = "plStateMachineBin";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::AutoTransformOnSave;

  plQtImageCache::GetSingleton()->RegisterTypeImage("StateMachine", plSvgThumbnailToPixmap(":/AssetIcons/StateMachine.svg"));
}

plStateMachineAssetManager::~plStateMachineAssetManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plStateMachineAssetManager::OnDocumentManagerEvent, this));
}

void plStateMachineAssetManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plStateMachineAssetDocument>())
      {
        new plQtStateMachineAssetDocumentWindow(e.m_pDocument); // Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void plStateMachineAssetManager::InternalCreateDocument(
  plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plStateMachineAssetDocument(sPath);
}

void plStateMachineAssetManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
