#include <EditorPluginTypeScript/EditorPluginTypeScriptPCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAsset.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetManager.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetObjects.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Utilities/Progress.h>
#include <ToolsFoundation/Command/TreeCommands.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptAssetDocument, 2, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plTypeScriptAssetDocument::plTypeScriptAssetDocument(const char* szDocumentPath)
  : plSimpleAssetDocument<plTypeScriptAssetProperties>(szDocumentPath, plAssetDocEngineConnection::None)
{
}

void plTypeScriptAssetDocument::EditScript()
{
  plStringBuilder sTsPath(GetProperties()->m_sScriptFile);

  if (GetProperties()->m_sScriptFile.IsEmpty())
    return;

  if (!plFileSystem::ExistsFile(sTsPath))
  {
    CreateComponentFile(sTsPath);
  }

  plStringBuilder sTsFileAbsPath;
  if (plFileSystem::ResolvePath(sTsPath, &sTsFileAbsPath, nullptr).Failed())
    return;

  static_cast<plTypeScriptAssetDocumentManager*>(GetDocumentManager())->SetupProjectForTypeScript(false);

  CreateTsConfigFiles();

  {
    QStringList args;

    for (const auto& dd : plQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs)
    {
      plStringBuilder path;
      plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, path).IgnoreResult();

      args.append(QString::fromUtf8(path, path.GetElementCount()));
    }

    args.append(sTsFileAbsPath.GetData());

    if (plQtUiServices::OpenInVsCode(args).Failed())
    {
      // try again with a different program
      plQtUiServices::OpenFileInDefaultProgram(sTsFileAbsPath);
    }
  }

  {
    plTypeScriptAssetDocumentEvent e;
    e.m_Type = plTypeScriptAssetDocumentEvent::Type::ScriptOpened;
    e.m_pDocument = this;
    m_Events.Broadcast(e);
  }
}

void plTypeScriptAssetDocument::CreateComponentFile(const char* szFile)
{
  plStringBuilder sScriptFile = szFile;

  {
    plDataDirectoryType* pDataDir = nullptr;
    if (plFileSystem::ResolvePath(GetDocumentPath(), nullptr, nullptr, &pDataDir).Failed())
      return;

    sScriptFile.Prepend(pDataDir->GetRedirectedDataDirectoryPath(), "/");
  }

  const plStringBuilder sComponentName = plPathUtils::GetFileName(GetDocumentPath());

  if (sComponentName.IsEmpty())
    return;

  plStringBuilder sContent;

  {
    plFileReader fileIn;
    if (fileIn.Open(":plugins/TypeScript/NewComponent.ts").Succeeded())
    {
      sContent.ReadAll(fileIn);
      sContent.ReplaceAll("NewComponent", sComponentName.GetView());
      sContent.ReplaceAll("<PATH-TO-PLASMA-TS>", "TypeScript/pl");
    }
  }

  {
    plFileWriter file;
    if (file.Open(sScriptFile).Succeeded())
    {
      file.WriteBytes(sContent.GetData(), sContent.GetElementCount()).IgnoreResult();
    }
  }

  {
    plTypeScriptAssetDocumentEvent e;
    e.m_Type = plTypeScriptAssetDocumentEvent::Type::ScriptCreated;
    e.m_pDocument = this;
    m_Events.Broadcast(e);
  }
}

void plTypeScriptAssetDocument::CreateTsConfigFiles()
{
  for (const auto& dd : plQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs)
  {
    if (dd.m_sRootName.IsEqual_NoCase("BASE"))
      continue;

    plStringBuilder path;
    plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, path).IgnoreResult();
    path.MakeCleanPath();

    CreateTsConfigFile(path).IgnoreResult();
  }
}

plResult plTypeScriptAssetDocument::CreateTsConfigFile(const char* szDirectory)
{
  plStringBuilder sTsConfig;
  plStringBuilder sTmp;

  for (plUInt32 iPlus1 = plQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs.GetCount(); iPlus1 > 0; --iPlus1)
  {
    const auto& dd = plQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs[iPlus1 - 1];

    plStringBuilder path;
    PLASMA_SUCCEED_OR_RETURN(plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, path));
    path.MakeCleanPath();
    path.AppendPath("*");

    sTmp.AppendWithSeparator(", ", "\"", path, "\"");
  }

  sTsConfig.Format(
    R"({
  "compilerOptions": {
    "target": "es5",
    "baseUrl": "",
    "paths": {
      "*": [{0}]
    }    
  }
}
)",
    sTmp);


  {
    sTmp = szDirectory;
    sTmp.AppendPath("tsconfig.json");

    plFileWriter file;
    PLASMA_SUCCEED_OR_RETURN(file.Open(sTmp));
    PLASMA_SUCCEED_OR_RETURN(file.WriteBytes(sTsConfig.GetData(), sTsConfig.GetElementCount()));
  }

  {
    sTmp = szDirectory;
    sTmp.AppendPath(".gitignore");

    plQtUiServices::AddToGitIgnore(sTmp, "tsconfig.json").IgnoreResult();
  }

  return PLASMA_SUCCESS;
}

void plTypeScriptAssetDocument::UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  plExposedParameters* pExposedParams = PLASMA_DEFAULT_NEW(plExposedParameters);

  {
    for (const auto& p : GetProperties()->m_NumberParameters)
    {
      plExposedParameter* param = PLASMA_DEFAULT_NEW(plExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = p.m_sName;
      param->m_DefaultValue = p.m_DefaultValue;
    }

    for (const auto& p : GetProperties()->m_BoolParameters)
    {
      plExposedParameter* param = PLASMA_DEFAULT_NEW(plExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = p.m_sName;
      param->m_DefaultValue = p.m_DefaultValue;
    }

    for (const auto& p : GetProperties()->m_StringParameters)
    {
      plExposedParameter* param = PLASMA_DEFAULT_NEW(plExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = p.m_sName;
      param->m_DefaultValue = p.m_DefaultValue;
    }

    for (const auto& p : GetProperties()->m_Vec3Parameters)
    {
      plExposedParameter* param = PLASMA_DEFAULT_NEW(plExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = p.m_sName;
      param->m_DefaultValue = p.m_DefaultValue;
    }

    for (const auto& p : GetProperties()->m_ColorParameters)
    {
      plExposedParameter* param = PLASMA_DEFAULT_NEW(plExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = p.m_sName;
      param->m_DefaultValue = p.m_DefaultValue;
    }
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

plTransformStatus plTypeScriptAssetDocument::InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  PLASMA_SUCCEED_OR_RETURN(ValidateScriptCode());
  PLASMA_SUCCEED_OR_RETURN(AutoGenerateVariablesCode());

  plStringBuilder sTypeName = plPathUtils::GetFileName(GetDocumentPath());
  stream << sTypeName;

  const plUuid& docGuid = GetGuid();
  stream << docGuid;

  {
    plTypeScriptAssetDocumentEvent e;
    e.m_Type = plTypeScriptAssetDocumentEvent::Type::ScriptTransformed;
    e.m_pDocument = this;
    m_Events.Broadcast(e);
  }

  plTypeScriptAssetDocumentManager* pAssMan = static_cast<plTypeScriptAssetDocumentManager*>(GetAssetDocumentManager());
  PLASMA_SUCCEED_OR_RETURN(pAssMan->GenerateScriptCompendium(transformFlags));

  return plTransformStatus();
}

plStatus plTypeScriptAssetDocument::ValidateScriptCode()
{
  plStringBuilder sTsDocPath = GetProperties()->m_sScriptFile;
  plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sTsDocPath);

  plStringBuilder content;

  // read typescript file content
  {
    plFileReader tsFile;
    if (tsFile.Open(sTsDocPath).Failed())
    {
      return plStatus(plFmt("Could not read .ts file '{}'", GetProperties()->m_sScriptFile));
    }

    content.ReadAll(tsFile);
  }

  // validate that the class with the correct name exists
  {
    plStringBuilder sClass;
    sClass = "class ";
    sClass.Append(sTsDocPath.GetFileName());
    sClass.Append(" extends");

    if (content.FindSubString(sClass) == nullptr)
    {
      return plStatus(plFmt("Sub-string '{}' not found. Class name may be incorrect.", sClass));
    }
  }

  return plStatus(PLASMA_SUCCESS);
}

plStatus plTypeScriptAssetDocument::AutoGenerateVariablesCode()
{
  plStringBuilder sTsDocPath = GetProperties()->m_sScriptFile;
  plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sTsDocPath);

  plStringBuilder content;

  // read typescript file content
  {
    plFileReader tsFile;
    if (tsFile.Open(sTsDocPath).Failed())
    {
      return plStatus(plFmt("Could not read .ts file '{}'", GetProperties()->m_sScriptFile));
    }

    content.ReadAll(tsFile);
  }

  const char* szTagBegin = "/* BEGIN AUTO-GENERATED: VARIABLES */";
  const char* szTagEnd = "/* END AUTO-GENERATED: VARIABLES */";

  const char* szBeginAG = content.FindSubString(szTagBegin);

  if (szBeginAG == nullptr)
  {
    return plStatus(plFmt("'{}' tag is missing or corrupted.", szTagBegin));
  }

  const char* szEndAG = content.FindSubString(szTagEnd, szBeginAG);


  if (szEndAG == nullptr)
  {
    return plStatus(plFmt("'{}' tag is missing or corrupted.", szTagEnd));
  }

  plStringBuilder sAutoGen;

  // create code for exposed parameters
  {
    for (const auto& p : GetProperties()->m_NumberParameters)
    {
      sAutoGen.AppendFormat("    {}: number = {};\n", p.m_sName, p.m_DefaultValue);
    }
    for (const auto& p : GetProperties()->m_BoolParameters)
    {
      sAutoGen.AppendFormat("    {}: boolean = {};\n", p.m_sName, p.m_DefaultValue);
    }
    for (const auto& p : GetProperties()->m_StringParameters)
    {
      sAutoGen.AppendFormat("    {}: string = \"{}\";\n", p.m_sName, p.m_DefaultValue);
    }
    for (const auto& p : GetProperties()->m_Vec3Parameters)
    {
      sAutoGen.AppendFormat("    {}: pl.Vec3 = new pl.Vec3({}, {}, {});\n", p.m_sName, p.m_DefaultValue.x, p.m_DefaultValue.y, p.m_DefaultValue.z);
    }
    for (const auto& p : GetProperties()->m_ColorParameters)
    {
      sAutoGen.AppendFormat("    {}: pl.Color = new pl.Color({}, {}, {}, {});\n", p.m_sName, p.m_DefaultValue.r, p.m_DefaultValue.g, p.m_DefaultValue.b, p.m_DefaultValue.a);
    }
  }

  // write back the modified file
  {
    sAutoGen.Prepend(szTagBegin, "\n");
    sAutoGen.Append("    ", szTagEnd);

    content.ReplaceSubString(szBeginAG, szEndAG + plStringUtils::GetStringElementCount(szTagEnd), sAutoGen);

    plFileWriter tsWriteBack;
    if (tsWriteBack.Open(sTsDocPath).Failed())
    {
      return plStatus(plFmt("Could not update .ts file '{}'", GetProperties()->m_sScriptFile));
    }

    PLASMA_SUCCEED_OR_RETURN(tsWriteBack.WriteBytes(content.GetData(), content.GetElementCount()));
  }

  return plStatus(PLASMA_SUCCESS);
}

void plTypeScriptAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  if (bFirstTimeCreation)
  {
    auto history = GetCommandHistory();
    history->StartTransaction("Initial Setup");

    if (GetProperties()->m_sScriptFile.IsEmpty())
    {
      plStringBuilder sDefaultFile = GetDocumentPath();
      sDefaultFile.ChangeFileExtension("ts");
      plQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sDefaultFile);

      plSetObjectPropertyCommand propCmd;
      propCmd.m_Object = GetPropertyObject()->GetGuid();
      propCmd.m_sProperty = "ScriptFile";
      propCmd.m_NewValue = plString(sDefaultFile);
      PLASMA_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    history->FinishTransaction();

    const plString& sTsPath = GetProperties()->m_sScriptFile;

    if (!plFileSystem::ExistsFile(sTsPath))
    {
      CreateComponentFile(sTsPath);
    }
  }
}
