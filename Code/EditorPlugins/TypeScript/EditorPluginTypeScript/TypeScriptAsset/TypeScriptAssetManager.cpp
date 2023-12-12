#include <EditorPluginTypeScript/EditorPluginTypeScriptPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAsset.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetManager.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetWindow.moc.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <TypeScriptPlugin/Resources/ScriptCompendiumResource.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptAssetDocumentManager, 1, plRTTIDefaultAllocator<plTypeScriptAssetDocumentManager>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plTypeScriptAssetDocumentManager::plTypeScriptAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plTypeScriptAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "TypeScript";
  m_DocTypeDesc.m_sFileExtension = "plTypeScriptAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/TypeScript.svg";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plTypeScriptAssetDocument>();
  m_DocTypeDesc.m_pManager = this;

  // TODO: Get typescript working with new script interface
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Code_TypeScript");

  m_DocTypeDesc.m_sResourceFileExtension = "plTypeScriptRes";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::None;

  plQtImageCache::GetSingleton()->RegisterTypeImage("TypeScript", QPixmap(":/AssetIcons/TypeScript.svg"));

  plToolsProject::s_Events.AddEventHandler(plMakeDelegate(&plTypeScriptAssetDocumentManager::ToolsProjectEventHandler, this));

  plGameObjectDocument::s_GameObjectDocumentEvents.AddEventHandler(plMakeDelegate(&plTypeScriptAssetDocumentManager::GameObjectDocumentEventHandler, this));

  // make sure the preferences exist
  plPreferences::QueryPreferences<plTypeScriptPreferences>();
}

plTypeScriptAssetDocumentManager::~plTypeScriptAssetDocumentManager()
{
  ShutdownTranspiler();

  plToolsProject::s_Events.RemoveEventHandler(plMakeDelegate(&plTypeScriptAssetDocumentManager::ToolsProjectEventHandler, this));

  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plTypeScriptAssetDocumentManager::OnDocumentManagerEvent, this));

  plGameObjectDocument::s_GameObjectDocumentEvents.RemoveEventHandler(plMakeDelegate(&plTypeScriptAssetDocumentManager::GameObjectDocumentEventHandler, this));
}

void plTypeScriptAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plTypeScriptAssetDocument>())
      {
        plQtTypeScriptAssetDocumentWindow* pDocWnd = new plQtTypeScriptAssetDocumentWindow(static_cast<plTypeScriptAssetDocument*>(e.m_pDocument));
      }
    }
    break;

    default:
      break;
  }
}

void plTypeScriptAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plTypeScriptAssetDocument(szPath);
}

void plTypeScriptAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}

void plTypeScriptAssetDocumentManager::ToolsProjectEventHandler(const plToolsProjectEvent& e)
{
  if (e.m_Type == plToolsProjectEvent::Type::ProjectOpened)
  {
    InitializeTranspiler();
  }

  if (e.m_Type == plToolsProjectEvent::Type::ProjectClosing)
  {
    ShutdownTranspiler();
  }
}

void plTypeScriptAssetDocumentManager::GameObjectDocumentEventHandler(const plGameObjectDocumentEvent& e)
{
  switch (e.m_Type)
  {
    case plGameObjectDocumentEvent::Type::GameMode_StartingSimulate:
    {
      if (plPreferences::QueryPreferences<plTypeScriptPreferences>()->m_bAutoUpdateScriptsForSimulation)
      {
        GenerateScriptCompendium(plTransformFlags::Default).IgnoreResult();
      }
      break;
    }

    case plGameObjectDocumentEvent::Type::GameMode_StartingPlay:
    case plGameObjectDocumentEvent::Type::GameMode_StartingExternal:
    {
      if (plPreferences::QueryPreferences<plTypeScriptPreferences>()->m_bAutoUpdateScriptsForPlayTheGame)
      {
        GenerateScriptCompendium(plTransformFlags::Default).IgnoreResult();
      }
      break;
    }

    default:
      break;
  }
}

void plTypeScriptAssetDocumentManager::ModifyTsBeforeTranspilation(plStringBuilder& content)
{
  const char* szTagBegin = "PLASMA_DECLARE_MESSAGE_TYPE;";

  plUInt32 uiContinueAfterOffset = 0;
  plStringBuilder sAutoGen;

  while (true)
  {

    const char* szBeginAG = content.FindSubString(szTagBegin, content.GetData() + uiContinueAfterOffset);

    if (szBeginAG == nullptr)
    {
      break;
    }

    const char* szClassAG = content.FindLastSubString("class", szBeginAG);
    if (szClassAG == nullptr)
    {
      // return plStatus(plFmt("'{}' tag is incorrectly placed.", szBeginAG));
      return;
    }

    plUInt32 uiTypeNameHash = 0;

    {
      plStringView classNameView(szClassAG + 5, szBeginAG);
      classNameView.Trim(" \t\n\r");

      plStringBuilder sClassName;

      plStringIterator classNameIt = classNameView.GetIteratorFront();
      while (classNameIt.IsValid() && !plStringUtils::IsIdentifierDelimiter_C_Code(classNameIt.GetCharacter()))
      {
        sClassName.Append(classNameIt.GetCharacter());
        ++classNameIt;
      }

      if (sClassName.IsEmpty())
      {
        // return plStatus("Message class name not found.");
        return;
      }

      uiTypeNameHash = plHashingUtils::StringHashTo32(plHashingUtils::StringHash(sClassName.GetData()));
    }

    sAutoGen.Format("public static GetTypeNameHash(): number { return {0}; }\nconstructor() { super(); this.TypeNameHash = {0}; }\n", uiTypeNameHash);

    uiContinueAfterOffset = szBeginAG - content.GetData();

    content.ReplaceSubString(szBeginAG, szBeginAG + plStringUtils::GetStringElementCount(szTagBegin), sAutoGen);
  }
}

void plTypeScriptAssetDocumentManager::InitializeTranspiler()
{
  if (m_bTranspilerLoaded)
    return;

  m_bTranspilerLoaded = true;

  if (plFileSystem::FindDataDirectoryWithRoot("TypeScript") == nullptr)
  {
    plFileSystem::AddDataDirectory(">sdk/Data/Tools/PlasmaEditor/Typescript", "TypeScript", "TypeScript").IgnoreResult();
  }

  m_Transpiler.SetOutputFolder(":project/AssetCache/Temp");
  m_Transpiler.StartLoadTranspiler();
  m_Transpiler.SetModifyTsBeforeTranspilationCallback(&plTypeScriptAssetDocumentManager::ModifyTsBeforeTranspilation);
}

void plTypeScriptAssetDocumentManager::ShutdownTranspiler()
{
  if (!m_bTranspilerLoaded)
    return;

  m_bTranspilerLoaded = false;

  m_Transpiler.FinishLoadTranspiler();
}

void plTypeScriptAssetDocumentManager::SetupProjectForTypeScript(bool bForce)
{
  if (m_bProjectSetUp && !bForce)
    return;

  m_bProjectSetUp = true;

  if (plTypeScriptBinding::SetupProjectCode().Failed())
  {
    plLog::Error("Could not setup Typescript data in project directory");
    return;
  }
}

plResult plTypeScriptAssetDocumentManager::GenerateScriptCompendium(plBitflags<plTransformFlags> transformFlags)
{
  PLASMA_LOG_BLOCK("Generating Script Compendium");

  plHybridArray<plAssetInfo*, 256> allTsAssets;

  // keep this locked until the end of the function
  plAssetCurator::plLockedSubAssetTable AllAssetsLocked = plAssetCurator::GetSingleton()->GetKnownSubAssets();
  const plHashTable<plUuid, plSubAsset>& AllAssets = *AllAssetsLocked;

  for (auto it = AllAssets.GetIterator(); it.IsValid(); ++it)
  {
    const plSubAsset* pSub = &it.Value();

    if (pSub->m_pAssetInfo->GetManager() == this)
    {
      allTsAssets.PushBack(pSub->m_pAssetInfo);
    }
  }

  if (allTsAssets.IsEmpty())
  {
    plLog::Debug("Skipping script compendium creation - no TypeScript assets in project.");
    return PLASMA_SUCCESS;
  }


  SetupProjectForTypeScript(false);

  // read m_CheckedTsFiles cache
  if (m_CheckedTsFiles.IsEmpty())
  {
    plStringBuilder sFile = plApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    sFile.AppendPath("LastTypeScriptChanges.tmp");

    plFileReader file;
    if (file.Open(sFile).Succeeded())
    {
      file.ReadMap(m_CheckedTsFiles).IgnoreResult();
    }
  }

  plMap<plString, plUInt32> relPathToDataDirIdx;

  plScriptCompendiumResourceDesc compendium;
  bool bAnythingNew = false;

  for (plUInt32 ddIdx = 0; ddIdx < plFileSystem::GetNumDataDirectories(); ++ddIdx)
  {
    plStringBuilder sDataDirPath = plFileSystem::GetDataDirectory(ddIdx)->GetRedirectedDataDirectoryPath();
    sDataDirPath.MakeCleanPath();

    if (!sDataDirPath.IsAbsolutePath())
      continue;

    plFileSystemIterator fsIt;
    fsIt.StartSearch(sDataDirPath, plFileSystemIteratorFlags::ReportFilesRecursive);

    plStringBuilder sTsFilePath;
    for (; fsIt.IsValid(); fsIt.Next())
    {
      if (!fsIt.GetStats().m_sName.EndsWith_NoCase(".ts"))
        continue;

      fsIt.GetStats().GetFullPath(sTsFilePath);

      sTsFilePath.MakeRelativeTo(sDataDirPath).IgnoreResult();

      relPathToDataDirIdx[sTsFilePath] = ddIdx;

      compendium.m_PathToSource.Insert(sTsFilePath, plString());

      plTimestamp& lastModification = m_CheckedTsFiles[sTsFilePath];

      if (!lastModification.Compare(fsIt.GetStats().m_LastModificationTime, plTimestamp::CompareMode::FileTimeEqual))
      {
        bAnythingNew = true;
        lastModification = fsIt.GetStats().m_LastModificationTime;
      }
    }
  }

  plStringBuilder sOutFile(":project/AssetCache/Common/Scripts.plScriptCompendium");

  if (!transformFlags.IsSet(plTransformFlags::ForceTransform))
  {
    if (bAnythingNew == false && plFileSystem::ExistsFile(sOutFile))
      return PLASMA_SUCCESS;
  }

  plMap<plString, plString> filenameToSourceTsPath;

  plProgressRange progress("Transpiling Scripts", compendium.m_PathToSource.GetCount(), true);

  // remove the output file, so that if anything fails from here on out, it will be re-generated next time
  plFileSystem::DeleteFile(sOutFile);

  plStringBuilder sFilename;

  // TODO: could multi-thread this, if we had multiple transpilers loaded
  {
    plStringBuilder sOutputFolder;

    plStringBuilder sTranspiledJs;
    for (auto it : compendium.m_PathToSource)
    {
      if (!progress.BeginNextStep(it.Key()))
        return PLASMA_FAILURE;

      sOutputFolder = plFileSystem::GetDataDirectory(relPathToDataDirIdx[it.Key()])->GetRedirectedDataDirectoryPath();
      sOutputFolder.MakeCleanPath();
      sOutputFolder.AppendPath("AssetCache/Temp");
      m_Transpiler.SetOutputFolder(sOutputFolder);

      if (m_Transpiler.TranspileFileAndStoreJS(it.Key(), sTranspiledJs).Failed())
      {
        plLog::Error("Failed to transpile '{}'", it.Key());
        return PLASMA_FAILURE;
      }

      it.Value() = sTranspiledJs;

      sFilename = plPathUtils::GetFileName(it.Key());
      filenameToSourceTsPath[sFilename] = it.Key();
    }
  }

  // at runtime we need to be able to load a typescript component
  // at edit time, the plTypeScriptComponent should present the component type as a reference to an asset document
  // thus at edit time, this reference should look like a path to a document
  // however, at runtime we only need the name of the component type to instantiate (for the call to 'new' in Duktape/JS)
  // and the relative path to the source ts/js file (for the call to 'require' in Duktape/JS to 'load' the module)
  // just for these two strings we do not want to load an entire resource, as we would typically do with other asset types
  // therefore we extract the required data (component name and path) here and store it in the compendium
  // now all we need is the GUID of the TypeScript asset to look up this information at runtime
  // thus the plTypeScriptComponent does not need to store the asset document reference as a full string (path), but can just
  // store it as the GUID
  // at runtime this 'path' is not used as an plResource path/id, as would be common, but is used to look up the information
  // directly from the compendium
  {
    for (auto pAssetInfo : allTsAssets)
    {
      const plString& docPath = pAssetInfo->m_sDataDirParentRelativePath;
      const plUuid& docGuid = pAssetInfo->m_Info->m_DocumentID;

      sFilename = plPathUtils::GetFileName(docPath);

      // TODO: handle filenameToSourceTsPath[sFilename] == "" case (log error)
      compendium.m_AssetGuidToInfo[docGuid].m_sComponentTypeName = sFilename;
      compendium.m_AssetGuidToInfo[docGuid].m_sComponentFilePath = filenameToSourceTsPath[sFilename];
    }
  }

  {
    plDeferredFileWriter file;
    file.SetOutput(sOutFile);

    plAssetFileHeader header;
    header.SetFileHashAndVersion(1, 1);

    PLASMA_SUCCEED_OR_RETURN(header.Write(file));

    PLASMA_SUCCEED_OR_RETURN(compendium.Serialize(file));

    PLASMA_SUCCEED_OR_RETURN(file.Close());
  }

  // write m_CheckedTsFiles cache
  {
    plStringBuilder sFile = plApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    sFile.AppendPath("LastTypeScriptChanges.tmp");

    plFileWriter file;
    if (file.Open(sFile).Succeeded())
    {
      PLASMA_SUCCEED_OR_RETURN(file.WriteMap(m_CheckedTsFiles));
    }
  }

  return PLASMA_SUCCESS;
}

plStatus plTypeScriptAssetDocumentManager::GetAdditionalOutputs(plDynamicArray<plString>& files)
{
  if (GenerateScriptCompendium(plTransformFlags::Default).Failed())
    return plStatus("Failed to build TypeScript compendium.");

  files.PushBack("AssetCache/Common/Scripts.plScriptCompendium");

  return plStatus(PLASMA_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptPreferences, 1, plRTTIDefaultAllocator<plTypeScriptPreferences>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("CompileForSimulation", m_bAutoUpdateScriptsForSimulation),
    PLASMA_MEMBER_PROPERTY("CompileForPlayTheGame", m_bAutoUpdateScriptsForPlayTheGame),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plTypeScriptPreferences::plTypeScriptPreferences()
  : plPreferences(plPreferences::Domain::Application, "TypeScript")
{
}
