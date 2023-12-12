#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptClassAsset/VisualScriptClassAsset.h>
#include <EditorPluginVisualScript/VisualScriptClassAsset/VisualScriptClassAssetManager.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptWindow.moc.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plVisualScriptClassAssetManager, 1, plRTTIDefaultAllocator<plVisualScriptClassAssetManager>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plVisualScriptClassAssetManager::plVisualScriptClassAssetManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plVisualScriptClassAssetManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "VisualScriptClass";
  m_DocTypeDesc.m_sFileExtension = "plVisualScriptClassAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/VisualScript.svg";
  //m_DocTypeDesc.m_sAssetCategory = "Scripting";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plVisualScriptClassAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_ScriptClass");

  m_DocTypeDesc.m_sResourceFileExtension = "plVisualScriptClassBin";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::AutoTransformOnSave;

  plQtImageCache::GetSingleton()->RegisterTypeImage("VisualScriptClass", QPixmap(":/AssetIcons/VisualScript.svg"));
}

plVisualScriptClassAssetManager::~plVisualScriptClassAssetManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plVisualScriptClassAssetManager::OnDocumentManagerEvent, this));
}

void plVisualScriptClassAssetManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plVisualScriptClassAssetDocument>())
      {
        new plQtVisualScriptWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void plVisualScriptClassAssetManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plVisualScriptClassAssetDocument(szPath);
}

void plVisualScriptClassAssetManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
