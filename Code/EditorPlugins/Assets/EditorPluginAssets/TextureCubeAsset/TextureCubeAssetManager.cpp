#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include "ToolsFoundation/Assets/AssetFileExtensionWhitelist.h"
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAsset.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetManager.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetWindow.moc.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTextureCubeAssetDocumentManager, 1, plRTTIDefaultAllocator<plTextureCubeAssetDocumentManager>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plTextureCubeAssetDocumentManager::plTextureCubeAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plTextureCubeAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  plAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Texture_Cube", "dds");

  m_DocTypeDesc.m_sDocumentTypeName = "Texture Cube";
  m_DocTypeDesc.m_sFileExtension = "plTextureCubeAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Texture_Cube.svg";
  m_DocTypeDesc.m_sAssetCategory = "Rendering";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plTextureCubeAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_Cube");

  m_DocTypeDesc.m_sResourceFileExtension = "plTextureCube";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::AutoThumbnailOnTransform;
}

plTextureCubeAssetDocumentManager::~plTextureCubeAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plTextureCubeAssetDocumentManager::OnDocumentManagerEvent, this));
}

void plTextureCubeAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plTextureCubeAssetDocument>())
      {
        new plQtTextureCubeAssetDocumentWindow(static_cast<plTextureCubeAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void plTextureCubeAssetDocumentManager::InternalCreateDocument(
  plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plTextureCubeAssetDocument(sPath);
}

void plTextureCubeAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}

plUInt64 plTextureCubeAssetDocumentManager::ComputeAssetProfileHashImpl(const plPlatformProfile* pAssetProfile) const
{
  // don't have any settings yet, but assets that generate profile specific output must not return 0 here
  return 1;
}
