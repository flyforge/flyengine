#include <EditorPluginSubstance/EditorPluginSubstancePCH.h>

#include <EditorPluginSubstance/Assets/SubstancePackageAsset.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAssetManager.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSubstancePackageAssetDocumentManager, 1, plRTTIDefaultAllocator<plSubstancePackageAssetDocumentManager>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plSubstancePackageAssetDocumentManager::plSubstancePackageAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plSubstancePackageAssetDocumentManager::OnDocumentManagerEvent, this));

  {
    m_PackageTypeDesc.m_sDocumentTypeName = "Substance Package";
    m_PackageTypeDesc.m_sFileExtension = "plSubstancePackageAsset";
    m_PackageTypeDesc.m_sIcon = ":/AssetIcons/SubstanceDesigner.svg";
    m_TextureTypeDesc.m_sAssetCategory = "Rendering";
    m_PackageTypeDesc.m_pDocumentType = plGetStaticRTTI<plSubstancePackageAssetDocument>();
    m_PackageTypeDesc.m_pManager = this;
    m_PackageTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Substance_Package");

    m_PackageTypeDesc.m_sResourceFileExtension = "plSubstancePackage";
    m_PackageTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::None;

    plQtImageCache::GetSingleton()->RegisterTypeImage("Substance Package", QPixmap(":/AssetIcons/SubstanceDesigner.svg"));
  }

  {
    m_TextureTypeDesc.m_bCanCreate = false;
    m_TextureTypeDesc.m_sDocumentTypeName = "Substance Texture";
    m_TextureTypeDesc.m_sFileExtension = "plSubstanceTextureAsset";
    m_TextureTypeDesc.m_sIcon = ":/AssetIcons/SubstanceDesigner.svg";
    m_TextureTypeDesc.m_sAssetCategory = "Rendering";
    m_TextureTypeDesc.m_pManager = this;
    m_TextureTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_2D");

    m_TextureTypeDesc.m_sResourceFileExtension = "plTexture2D";
    // m_TextureTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::AutoThumbnailOnTransform;
  }
}

plSubstancePackageAssetDocumentManager::~plSubstancePackageAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plSubstancePackageAssetDocumentManager::OnDocumentManagerEvent, this));
}

void plSubstancePackageAssetDocumentManager::FillOutSubAssetList(const plAssetDocumentInfo& assetInfo, plDynamicArray<plSubAssetData>& out_subAssets) const
{
  auto pMetaData = assetInfo.GetMetaInfo<plSubstancePackageAssetMetaData>();
  if (pMetaData == nullptr)
    return;

  for (plUInt32 i = 0; i < pMetaData->m_OutputUuids.GetCount(); ++i)
  {
    auto& subAsset = out_subAssets.ExpandAndGetRef();
    subAsset.m_Guid = pMetaData->m_OutputUuids[i];
    subAsset.m_sName = pMetaData->m_OutputNames[i];
    subAsset.m_sSubAssetsDocumentTypeName.Assign(m_TextureTypeDesc.m_sDocumentTypeName);
  }
}

plString plSubstancePackageAssetDocumentManager::GetAssetTableEntry(const plSubAsset* pSubAsset, const char* szDataDirectory, const plPlatformProfile* pAssetProfile) const
{
  if (pSubAsset->m_bMainAsset)
  {
    return SUPER::GetAssetTableEntry(pSubAsset, szDataDirectory, pAssetProfile);
  }

  plStringBuilder sTargetFile = pSubAsset->m_pAssetInfo->m_sAbsolutePath.GetFileDirectory();
  sTargetFile.Append(pSubAsset->m_Data.m_sName);

  return GetRelativeOutputFileName(&m_TextureTypeDesc, szDataDirectory, sTargetFile, "", pAssetProfile);
}

void plSubstancePackageAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plSubstancePackageAssetDocument>())
      {
        new plQtSubstancePackageAssetWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void plSubstancePackageAssetDocumentManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plSubstancePackageAssetDocument(szPath);
}

void plSubstancePackageAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_PackageTypeDesc);
  inout_DocumentTypes.PushBack(&m_TextureTypeDesc);
}
