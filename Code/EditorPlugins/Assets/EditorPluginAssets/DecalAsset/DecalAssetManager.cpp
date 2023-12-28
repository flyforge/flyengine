#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetManager.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetWindow.moc.h>
#include <Texture/Utils/TextureAtlasDesc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>

const char* ToCompressionMode(plTexConvCompressionMode::Enum mode);
const char* ToMipmapMode(plTexConvMipmapMode::Enum mode);

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDecalAssetDocumentManager, 1, plRTTIDefaultAllocator<plDecalAssetDocumentManager>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plDecalAssetDocumentManager::plDecalAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plDecalAssetDocumentManager::OnDocumentManagerEvent, this));

  // texture asset source files
  plAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "dds");
  plAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "tga");

  m_DocTypeDesc.m_sDocumentTypeName = "Decal";
  m_DocTypeDesc.m_sFileExtension = "plDecalAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Decal.svg";
  m_DocTypeDesc.m_sAssetCategory = "Effects";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plDecalAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Decal");

  m_DocTypeDesc.m_sResourceFileExtension = "plDecalStub";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::SupportsThumbnail;
}

plDecalAssetDocumentManager::~plDecalAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plDecalAssetDocumentManager::OnDocumentManagerEvent, this));
}

void plDecalAssetDocumentManager::AddEntriesToAssetTable(const char* szDataDirectory, const plPlatformProfile* pAssetProfile, plMap<plString, plString>& inout_GuidToPath) const
{
  plStringBuilder projectDir = plToolsProject::GetSingleton()->GetProjectDirectory();
  projectDir.MakeCleanPath();
  projectDir.Append("/");

  if (projectDir.StartsWith_NoCase(szDataDirectory))
  {
    inout_GuidToPath["{ ProjectDecalAtlas }"] = "PC/Decals.plTextureAtlas";
  }
}

plString plDecalAssetDocumentManager::GetAssetTableEntry(const plSubAsset* pSubAsset, const char* szDataDirectory, const plPlatformProfile* pAssetProfile) const
{
  // means NO table entry will be written, because for decals we don't need a redirection
  return plString();
}

void plDecalAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plDecalAssetDocument>())
      {
        plQtDecalAssetDocumentWindow* pDocWnd = new plQtDecalAssetDocumentWindow(static_cast<plDecalAssetDocument*>(e.m_pDocument));
      }
    }
    break;

    default:
      break;
  }
}

void plDecalAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plDecalAssetDocument(szPath);
}

void plDecalAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}

plUInt64 plDecalAssetDocumentManager::ComputeAssetProfileHashImpl(const plPlatformProfile* pAssetProfile) const
{
  // don't have any settings yet, but assets that generate profile specific output must not return 0 here
  return 1;
}

plStatus plDecalAssetDocumentManager::GenerateDecalTexture(const plPlatformProfile* pAssetProfile)
{
  plAssetCurator* pCurator = plAssetCurator::GetSingleton();
  const auto& allAssets = pCurator->GetKnownSubAssets();

  plUInt64 uiAssetHash = 1;

  for (auto it = allAssets->GetIterator(); it.IsValid(); ++it)
  {
    const auto& asset = it.Value();

    if (asset.m_pAssetInfo->GetManager() != this)
      continue;

    uiAssetHash += pCurator->GetAssetDependencyHash(it.Key());
  }

  plStringBuilder decalFile = plToolsProject::GetSingleton()->GetProjectDirectory();
  decalFile.AppendPath("AssetCache", GetDecalTexturePath(pAssetProfile));

  if (IsDecalTextureUpToDate(decalFile, uiAssetHash))
    return plStatus(PLASMA_SUCCESS);

  plTextureAtlasCreationDesc atlasDesc;

  // find all decal assets, extract their file information to pass it along to TexConv
  {
    atlasDesc.m_Layers.SetCount(3);
    atlasDesc.m_Layers[0].m_Usage = plTexConvUsage::Color;
    atlasDesc.m_Layers[1].m_Usage = plTexConvUsage::NormalMap;
    atlasDesc.m_Layers[2].m_Usage = plTexConvUsage::Linear;
    atlasDesc.m_Layers[2].m_uiNumChannels = 3;

    atlasDesc.m_Items.Reserve(64);

    plQtEditorApp* pEditorApp = plQtEditorApp::GetSingleton();
    plStringBuilder sAbsPath;

    for (auto it = allAssets->GetIterator(); it.IsValid(); ++it)
    {
      const auto& asset = it.Value();

      if (asset.m_pAssetInfo->GetManager() != this)
        continue;

      PLASMA_LOG_BLOCK("Decal", asset.m_pAssetInfo->m_sDataDirParentRelativePath);

      // does the document already exist and is it open ?
      bool bWasOpen = false;
      plDocument* pDoc = GetDocumentByPath(asset.m_pAssetInfo->m_sAbsolutePath);
      if (pDoc)
        bWasOpen = true;
      else
        pDoc = pEditorApp->OpenDocument(asset.m_pAssetInfo->m_sAbsolutePath, plDocumentFlags::None);

      if (pDoc == nullptr)
        return plStatus(plFmt("Could not open asset document '{0}'", asset.m_pAssetInfo->m_sDataDirParentRelativePath));

      plDecalAssetDocument* pDecalAsset = static_cast<plDecalAssetDocument*>(pDoc);

      {
        auto& item = atlasDesc.m_Items.ExpandAndGetRef();

        // store the GUID as the decal identifier
        plConversionUtils::ToString(pDecalAsset->GetGuid(), sAbsPath);
        item.m_uiUniqueID = plHashingUtils::StringHashTo32(plHashingUtils::StringHash(sAbsPath));

        auto pDecalProps = pDecalAsset->GetProperties();
        item.m_uiFlags = 0;
        item.m_uiFlags |= pDecalProps->NeedsNormal() ? DECAL_USE_NORMAL : 0;
        item.m_uiFlags |= pDecalProps->NeedsORM() ? DECAL_USE_ORM : 0;
        item.m_uiFlags |= pDecalProps->NeedsEmissive() ? DECAL_USE_EMISSIVE : 0;
        item.m_uiFlags |= pDecalProps->m_bBlendModeColorize ? DECAL_BLEND_MODE_COLORIZE : 0;

        if (!pDecalProps->m_sAlphaMask.IsEmpty())
        {
          sAbsPath = pDecalAsset->GetProperties()->m_sAlphaMask;
          if (sAbsPath.IsEmpty() || !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return plStatus(plFmt("Invalid alpha mask texture path '{0}'", sAbsPath));
          }

          item.m_sAlphaInput = sAbsPath;
        }

        if (pDecalProps->NeedsBaseColor())
        {
          sAbsPath = pDecalAsset->GetProperties()->m_sBaseColor;
          if (sAbsPath.IsEmpty() || !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return plStatus(plFmt("Invalid base color texture path '{0}'", sAbsPath));
          }

          item.m_sLayerInput[0] = sAbsPath;
        }

        if (pDecalProps->NeedsNormal())
        {
          sAbsPath = pDecalAsset->GetProperties()->m_sNormal;
          if (sAbsPath.IsEmpty() || !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return plStatus(plFmt("Invalid normal texture path '{0}'", sAbsPath));
          }

          item.m_sLayerInput[1] = sAbsPath;
        }

        if (pDecalProps->NeedsORM())
        {
          sAbsPath = pDecalAsset->GetProperties()->m_sORM;
          if (sAbsPath.IsEmpty() || !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return plStatus(plFmt("Invalid ORM texture path '{0}'", sAbsPath));
          }

          item.m_sLayerInput[2] = sAbsPath;
        }

        if (pDecalProps->NeedsEmissive())
        {
          sAbsPath = pDecalAsset->GetProperties()->m_sEmissive;
          if (sAbsPath.IsEmpty() || !pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
          {
            return plStatus(plFmt("Invalid emissive texture path '{0}'", sAbsPath));
          }

          item.m_sLayerInput[2] = sAbsPath;
        }
      }


      if (!pDoc->HasWindowBeenRequested() && !bWasOpen)
        pDoc->GetDocumentManager()->CloseDocument(pDoc);
    }
  }

  plAssetFileHeader header;
  {
    plUInt16 uiVersion = plGetStaticRTTI<plDecalAssetDocument>()->GetTypeVersion() & 0xFF;
    uiVersion |= (plGetStaticRTTI<plDecalAssetProperties>()->GetTypeVersion() & 0xFF) << 8;

    header.SetFileHashAndVersion(uiAssetHash, uiVersion);
  }

  plStatus result;

  // Send information to TexConv to do all the work
  {
    plStringBuilder texGroupFile = plToolsProject::GetSingleton()->GetProjectDirectory();
    texGroupFile.AppendPath("AssetCache", GetDecalTexturePath(pAssetProfile));
    texGroupFile.ChangeFileExtension("plDecalAtlasDesc");

    if (atlasDesc.Save(texGroupFile).Failed())
      return plStatus(plFmt("Failed to save texture atlas descriptor file '{0}'", texGroupFile));

    result = RunTexConv(decalFile, texGroupFile, header);
  }

  plFileStats stat;
  if (plOSFile::GetFileStats(decalFile, stat).Succeeded() && stat.m_uiFileSize == 0)
  {
    // if the file was touched, but nothing written to it, delete the file
    // might happen if TexConv crashed or had an error
    plOSFile::DeleteFile(decalFile).IgnoreResult();
    result.m_Result = PLASMA_FAILURE;
  }

  return result;
}

bool plDecalAssetDocumentManager::IsDecalTextureUpToDate(const char* szDecalFile, plUInt64 uiAssetHash) const
{
  plFileReader file;
  if (file.Open(szDecalFile).Succeeded())
  {
    plAssetFileHeader header;
    header.Read(file).IgnoreResult();

    // file still valid
    if (header.GetFileHash() == uiAssetHash)
      return true;
  }

  return false;
}

plString plDecalAssetDocumentManager::GetDecalTexturePath(const plPlatformProfile* pAssetProfile0) const
{
  const plPlatformProfile* pAssetProfile = plAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile0);
  plStringBuilder result = "Decals";
  GenerateOutputFilename(result, pAssetProfile, "plTextureAtlas", true);

  return result;
}

plStatus plDecalAssetDocumentManager::RunTexConv(const char* szTargetFile, const char* szInputFile, const plAssetFileHeader& AssetHeader)
{
  QStringList arguments;
  plStringBuilder temp;

  // Asset Version
  {
    arguments << "-assetVersion";
    arguments << plConversionUtils::ToString(AssetHeader.GetFileVersion(), temp).GetData();
  }

  // Asset Hash
  {
    const plUInt64 uiHash64 = AssetHeader.GetFileHash();
    const plUInt32 uiHashLow32 = uiHash64 & 0xFFFFFFFF;
    const plUInt32 uiHashHigh32 = (uiHash64 >> 32) & 0xFFFFFFFF;

    temp.Format("{0}", plArgU(uiHashLow32, 8, true, 16, true));
    arguments << "-assetHashLow";
    arguments << temp.GetData();

    temp.Format("{0}", plArgU(uiHashHigh32, 8, true, 16, true));
    arguments << "-assetHashHigh";
    arguments << temp.GetData();
  }


  arguments << "-out";
  arguments << szTargetFile;

  arguments << "-type";
  arguments << "Atlas";

  arguments << "-compression";
  arguments << ToCompressionMode(plTexConvCompressionMode::High);

  arguments << "-mipmaps";
  arguments << ToMipmapMode(plTexConvMipmapMode::Linear);

  arguments << "-atlasDesc";
  arguments << QString(szInputFile);

  PLASMA_SUCCEED_OR_RETURN(plQtEditorApp::GetSingleton()->ExecuteTool("TexConv", arguments, 180, plLog::GetThreadLocalLogSystem()));

  return plStatus(PLASMA_SUCCESS);
}
