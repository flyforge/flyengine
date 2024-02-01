#include <EditorPluginSubstance/EditorPluginSubstancePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAsset.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAssetManager.h>
#include <Foundation/IO/FileSystem/FileReader.h>

#include <qsettings.h>
#include <qxmlstream.h>

namespace
{
  plResult GetSbsContent(plStringView sAbsolutePath, plStringBuilder& out_sContent)
  {
    plFileReader fileReader;
    PL_SUCCEED_OR_RETURN(fileReader.Open(sAbsolutePath));

    out_sContent.ReadAll(fileReader);
    return PL_SUCCESS;
  }

  plResult ReadUntilStartElement(QXmlStreamReader& inout_reader, const char* szName)
  {
    while (inout_reader.atEnd() == false)
    {
      auto tokenType = inout_reader.readNext();
      PL_IGNORE_UNUSED(tokenType);

      if (inout_reader.isStartElement() && inout_reader.name() == QLatin1StringView(szName))
        return PL_SUCCESS;
    }

    return PL_FAILURE;
  }

  plResult ReadUntilEndElement(QXmlStreamReader& inout_reader, const char* szName)
  {
    while (inout_reader.atEnd() == false)
    {
      auto tokenType = inout_reader.readNext();
      PL_IGNORE_UNUSED(tokenType);

      if (inout_reader.isEndElement() && inout_reader.name() == QLatin1StringView(szName))
        return PL_SUCCESS;
    }

    return PL_FAILURE;
  }

  template <typename T>
  T GetValueAttribute(QXmlStreamReader& inout_reader)
  {
    plString s(inout_reader.attributes().value("v").toUtf8().data());
    if constexpr (std::is_same_v<T, plString>)
    {
      return s;
    }
    else
    {
      plVariant v = s;
      return v.ConvertTo<T>();
    }
  }

  static const char* s_szSubstanceUsageMapping[] = {
    "", // Unknown,

    "baseColor",        // BaseColor,
    "emissive",         // Emissive,
    "height",           // Height,
    "metallic",         // Metallic,
    "mask",             // Mask,
    "normal",           // Normal,
    "ambientOcclusion", // Occlusion,
    "opacity",          // Opacity,
    "roughness",        // Roughness,
  };

  static_assert(PL_ARRAY_SIZE(s_szSubstanceUsageMapping) == plSubstanceUsage::Count);

  static plUInt8 s_substanceNumChannelsMapping[] = {
    1, // Unknown,

    3, // BaseColor,
    3, // Emissive,
    1, // Height,
    1, // Metallic,
    1, // Mask,
    3, // Normal,
    1, // Occlusion,
    1, // Opacity,
    1, // Roughness,
  };

  static_assert(PL_ARRAY_SIZE(s_szSubstanceUsageMapping) == plSubstanceUsage::Count);


  plSubstanceUsage::Enum GetUsage(QXmlStreamReader& inout_reader)
  {
    plString s = GetValueAttribute<plString>(inout_reader);
    for (plUInt32 i = 0; i < plSubstanceUsage::Count; ++i)
    {
      if (s.IsEqual_NoCase(s_szSubstanceUsageMapping[i]))
      {
        return static_cast<plSubstanceUsage::Enum>(i);
      }
    }

    return plSubstanceUsage::Unknown;
  }

  plResult ParseGraphOutput(QXmlStreamReader& inout_reader, plUInt32 uiGraphUid, plSubstanceGraphOutput& out_graphOutput)
  {
    PL_ASSERT_DEBUG(inout_reader.name() == QLatin1StringView("graphoutput"), "");

    while (inout_reader.readNextStartElement())
    {
      if (inout_reader.name() == QLatin1StringView("identifier"))
      {
        out_graphOutput.m_sName = GetValueAttribute<plString>(inout_reader);
      }
      else if (inout_reader.name() == QLatin1StringView("uid"))
      {
        plUInt32 outputUid = GetValueAttribute<plUInt32>(inout_reader);
        plUInt64 seed = plUInt64(uiGraphUid) << 32ull | outputUid;
        out_graphOutput.m_Uuid = plUuid::MakeStableUuidFromInt(seed);
      }
      else if (inout_reader.name() == QLatin1StringView("attributes"))
      {
        if (ReadUntilStartElement(inout_reader, "label").Succeeded())
        {
          out_graphOutput.m_sLabel = GetValueAttribute<plString>(inout_reader);
          PL_SUCCEED_OR_RETURN(ReadUntilEndElement(inout_reader, "label"));
        }
      }
      else if (inout_reader.name() == QLatin1StringView("usages"))
      {
        if (ReadUntilStartElement(inout_reader, "name").Succeeded())
        {
          out_graphOutput.m_Usage = GetUsage(inout_reader);
          out_graphOutput.m_uiNumChannels = s_substanceNumChannelsMapping[out_graphOutput.m_Usage];
          PL_SUCCEED_OR_RETURN(ReadUntilEndElement(inout_reader, "usage"));
        }
      }

      inout_reader.skipCurrentElement();
    }

    return PL_SUCCESS;
  }

  struct Option
  {
    plString m_sName;
    plString m_sValue;
  };

  plResult ParseOption(QXmlStreamReader& inout_reader, Option& out_option)
  {
    PL_ASSERT_DEBUG(inout_reader.name() == QLatin1StringView("option"), "");

    while (inout_reader.readNextStartElement())
    {
      if (inout_reader.name() == QLatin1StringView("name"))
      {
        out_option.m_sName = GetValueAttribute<plString>(inout_reader);
      }
      else if (inout_reader.name() == QLatin1StringView("value"))
      {
        out_option.m_sValue = GetValueAttribute<plString>(inout_reader);
      }

      inout_reader.skipCurrentElement();
    }

    return PL_SUCCESS;
  }

  plResult ParseGraph(QXmlStreamReader& inout_reader, plSubstanceGraph& out_graph)
  {
    PL_ASSERT_DEBUG(inout_reader.name() == QLatin1StringView("graph"), "");

    plUInt32 uiGraphUid = 0;
    while (inout_reader.readNextStartElement())
    {
      if (inout_reader.name() == QLatin1StringView("identifier"))
      {
        out_graph.m_sName = GetValueAttribute<plString>(inout_reader);
        inout_reader.skipCurrentElement();
      }
      else if (inout_reader.name() == QLatin1StringView("uid"))
      {
        uiGraphUid = GetValueAttribute<plUInt32>(inout_reader);
        inout_reader.skipCurrentElement();
      }
      else if (inout_reader.name() == QLatin1StringView("graphOutputs"))
      {
        while (inout_reader.readNextStartElement())
        {
          if (inout_reader.name() == QLatin1StringView("graphoutput"))
          {
            auto& graphOutput = out_graph.m_Outputs.ExpandAndGetRef();
            PL_SUCCEED_OR_RETURN(ParseGraphOutput(inout_reader, uiGraphUid, graphOutput));
          }
        }
      }
      else if (inout_reader.name() == QLatin1StringView("options"))
      {
        Option option;
        while (inout_reader.readNextStartElement())
        {
          if (inout_reader.name() == QLatin1StringView("option"))
          {
            PL_SUCCEED_OR_RETURN(ParseOption(inout_reader, option));

            if (option.m_sName == "defaultParentSize")
            {
              plStringView sValue = option.m_sValue;
              plUInt32 tmp = 0;
              const char* szLastPos = nullptr;
              PL_SUCCEED_OR_RETURN(plConversionUtils::StringToUInt(sValue, tmp, &szLastPos));
              out_graph.m_uiOutputWidth = static_cast<plUInt8>(tmp);

              if (*szLastPos != 'x')
                return PL_FAILURE;

              sValue = plStringView(szLastPos + 1);
              PL_SUCCEED_OR_RETURN(plConversionUtils::StringToUInt(sValue, tmp));
              out_graph.m_uiOutputHeight = static_cast<plUInt8>(tmp);
            }
            else if (option.m_sName.StartsWith("export/fromGraph/outputs/"))
            {
              const char* szLastSlash = option.m_sName.FindLastSubString("/");
              plStringView sOutputIdentifier = plStringView(szLastSlash + 1);

              bool bEnabled = false;
              PL_SUCCEED_OR_RETURN(plConversionUtils::StringToBool(option.m_sValue, bEnabled));

              for (auto& output : out_graph.m_Outputs)
              {
                if (output.m_sName == sOutputIdentifier)
                {
                  output.m_bEnabled = bEnabled;
                }
              }
            }
          }
        }
      }
      else
      {
        inout_reader.skipCurrentElement();
      }
    }

    return PL_SUCCESS;
  }

  plResult ReadDependencies(plStringView sSbsFile, plSet<plString>& out_dependencies)
  {
    plStringBuilder sAbsolutePath = sSbsFile;
    if (!plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsolutePath))
    {
      return PL_FAILURE;
    }

    plStringBuilder sFileContent;
    PL_SUCCEED_OR_RETURN(GetSbsContent(sAbsolutePath, sFileContent));

    QXmlStreamReader reader(sFileContent.GetData());
    PL_SUCCEED_OR_RETURN(ReadUntilStartElement(reader, "dependencies"));

    while (reader.readNextStartElement())
    {
      if (reader.name() != QLatin1StringView("dependency"))
      {
        reader.skipCurrentElement();
        continue;
      }

      if (ReadUntilStartElement(reader, "filename").Failed())
        continue;

      plString sDependency = GetValueAttribute<plString>(reader);
      if (sDependency.EndsWith(".sbs") && sDependency.StartsWith("sbs://") == false)
      {
        plStringBuilder sFullPath;
        if (sDependency.IsAbsolutePath())
        {
          PL_ASSERT_NOT_IMPLEMENTED;
        }
        else
        {
          sFullPath = sSbsFile.GetFileDirectory();
          sFullPath.AppendPath(sDependency);
          sFullPath.MakeCleanPath();
        }

        if (out_dependencies.Contains(sFullPath) == false)
        {
          out_dependencies.Insert(sFullPath);

          PL_SUCCEED_OR_RETURN(ReadDependencies(sFullPath, out_dependencies));
        }
      }

      PL_SUCCEED_OR_RETURN(ReadUntilEndElement(reader, "dependency"));
    }

    return PL_SUCCESS;
  }

  plResult GetInstallationPath(plStringBuilder& out_sPath)
  {
#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
    static plUntrackedString s_CachedPath;
    if (s_CachedPath.IsEmpty() == false)
    {
      out_sPath = s_CachedPath;
      return PL_SUCCESS;
    }

    auto CheckPath = [&](plStringView sPath)
    {
      plStringBuilder path = sPath;
      path.AppendPath("sbscooker.exe");

      if (plOSFile::ExistsFile(path))
      {
        s_CachedPath = sPath;
        out_sPath = sPath;
        return true;
      }

      return false;
    };

    plStringBuilder sPath = "C:/Program Files/Allegorithmic/Substance Designer";
    if (CheckPath(sPath))
    {
      return PL_SUCCESS;
    }

    QSettings settings("\\HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{e9e3d6d9-3023-41c7-b223-11d8fdd691b9}_is1", QSettings::NativeFormat);
    sPath = plStringView(settings.value("InstallLocation").toString().toUtf8());

    if (CheckPath(sPath))
    {
      return PL_SUCCESS;
    }

    plLog::Error("Installation of Substance Designer could not be located.");
    return PL_FAILURE;
#endif

    return PL_FAILURE;
  }

  plStatus RunSbsCooker(const char* szSbsFile, const char* szOutputPath)
  {
    plStringBuilder sToolPath;
    PL_SUCCEED_OR_RETURN(GetInstallationPath(sToolPath));
    sToolPath.AppendPath("sbscooker");

    QStringList arguments;

    arguments << "--inputs";
    arguments << szSbsFile;

    arguments << "--output-path";
    arguments << szOutputPath;

    arguments << "--no-optimization";

    PL_SUCCEED_OR_RETURN(plQtEditorApp::GetSingleton()->ExecuteTool(sToolPath, arguments, 180, plLog::GetThreadLocalLogSystem(), plLogMsgType::InfoMsg));

    return plStatus(PL_SUCCESS);
  }

  plStatus RunSbsRender(const char* szSbsarFile, const char* szGraph, const char* szGraphOutput, const char* szOutputName, const char* szOutputPath, plUInt8 uiOutputWidth, plUInt8 uiOutputHeight)
  {
    plStringBuilder sToolPath;
    PL_SUCCEED_OR_RETURN(GetInstallationPath(sToolPath));
    sToolPath.AppendPath("sbsrender");

    plStringBuilder sTmp;

    QStringList arguments;
    arguments << "render";

    arguments << "--engine";
    arguments << "d3d11pc";

    arguments << "--input";
    arguments << szSbsarFile;

    arguments << "--input-graph";
    arguments << szGraph;

    if (plStringUtils::IsNullOrEmpty(szGraphOutput) == false)
    {
      arguments << "--input-graph-output";
      arguments << szGraphOutput;
    }

    if (plStringUtils::IsNullOrEmpty(szOutputName) == false)
    {
      arguments << "--output-name";
      arguments << szOutputName;
    }

    arguments << "--output-path";
    arguments << szOutputPath;

    sTmp.SetFormat("$outputsize@{},{}", uiOutputWidth, uiOutputHeight);
    arguments << "--set-value";
    arguments << sTmp.GetData();

    PL_SUCCEED_OR_RETURN(plQtEditorApp::GetSingleton()->ExecuteTool(sToolPath, arguments, 180, plLog::GetThreadLocalLogSystem()));

    return plStatus(PL_SUCCESS);
  }
} // namespace

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plSubstanceUsage, 1)
  PL_ENUM_CONSTANT(plSubstanceUsage::Unknown),
  PL_ENUM_CONSTANT(plSubstanceUsage::BaseColor),
  PL_ENUM_CONSTANT(plSubstanceUsage::Emissive),
  PL_ENUM_CONSTANT(plSubstanceUsage::Height),
  PL_ENUM_CONSTANT(plSubstanceUsage::Metallic),
  PL_ENUM_CONSTANT(plSubstanceUsage::Mask),
  PL_ENUM_CONSTANT(plSubstanceUsage::Normal),
  PL_ENUM_CONSTANT(plSubstanceUsage::Occlusion),
  PL_ENUM_CONSTANT(plSubstanceUsage::Opacity),
  PL_ENUM_CONSTANT(plSubstanceUsage::Roughness),
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_TYPE(plSubstanceGraphOutput, plNoBase, 1, plRTTIDefaultAllocator<plSubstanceGraphOutput>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("Name", m_sName),
    PL_MEMBER_PROPERTY("Label", m_sLabel),
    PL_ENUM_MEMBER_PROPERTY("Usage", plSubstanceUsage, m_Usage),
    PL_MEMBER_PROPERTY("NumChannels", m_uiNumChannels)->AddAttributes(new plDefaultValueAttribute(1), new plClampValueAttribute(1, 4)),
    PL_ENUM_MEMBER_PROPERTY("CompressionMode", plTexConvCompressionMode, m_CompressionMode)->AddAttributes(new plDefaultValueAttribute(plTexConvCompressionMode::High)),
    PL_MEMBER_PROPERTY("PreserveAlphaCoverage", m_bPreserveAlphaCoverage),
    PL_MEMBER_PROPERTY("Uuid", m_Uuid)->AddAttributes(new plHiddenAttribute()),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plSubstanceGraph, plNoBase, 1, plRTTIDefaultAllocator<plSubstanceGraph>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Enabled", m_bEnabled)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("Name", m_sName),
    PL_MEMBER_PROPERTY("OutputWidth", m_uiOutputWidth)->AddAttributes(new plClampValueAttribute(4, 12)),
    PL_MEMBER_PROPERTY("OutputHeight", m_uiOutputHeight)->AddAttributes(new plClampValueAttribute(4, 12)),
    PL_ARRAY_MEMBER_PROPERTY("Outputs", m_Outputs),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSubstancePackageAssetProperties, 1, plRTTIDefaultAllocator<plSubstancePackageAssetProperties>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("SubstanceFile", m_sSubstancePackage)->AddAttributes(new plFileBrowserAttribute("Select Substance File", "*.sbs")),
    PL_MEMBER_PROPERTY("OutputPattern", m_sOutputPattern)->AddAttributes(new plDefaultValueAttribute(plStringView("$(graph)_$(label)"))),
    PL_ARRAY_MEMBER_PROPERTY("Graphs", m_Graphs)
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSubstancePackageAssetMetaData, 1, plRTTIDefaultAllocator<plSubstancePackageAssetMetaData>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ARRAY_MEMBER_PROPERTY("OutputUuids", m_OutputUuids),
    PL_ARRAY_MEMBER_PROPERTY("OutputNames", m_OutputNames)
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSubstancePackageAssetDocument, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSubstancePackageAssetDocument::plSubstancePackageAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument(sDocumentPath, plAssetDocEngineConnection::None)
{
  GetObjectManager()->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plSubstancePackageAssetDocument::OnPropertyChanged, this));
}

plSubstancePackageAssetDocument::~plSubstancePackageAssetDocument()
{
  GetObjectManager()->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plSubstancePackageAssetDocument::OnPropertyChanged, this));
}

void plSubstancePackageAssetDocument::UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  const plSubstancePackageAssetProperties* pProp = GetProperties();

  // Dependencies
  {
    pInfo->m_TransformDependencies.Insert(pProp->m_sSubstancePackage);

    ReadDependencies(pProp->m_sSubstancePackage, pInfo->m_TransformDependencies).IgnoreResult();
  }

  // Outputs
  {
    plStringBuilder sName;

    auto pMetaData = plGetStaticRTTI<plSubstancePackageAssetMetaData>()->GetAllocator()->Allocate<plSubstancePackageAssetMetaData>();
    for (auto& graph : pProp->m_Graphs)
    {
      if (graph.m_bEnabled == false)
        continue;

      for (auto& output : graph.m_Outputs)
      {
        if (output.m_bEnabled == false)
          continue;

        GenerateOutputName(graph, output, sName);
        if (pMetaData->m_OutputNames.Contains(sName))
        {
          plLog::Error("A substance texture named '{}' already exists.", sName);
          continue;
        }

        pMetaData->m_OutputUuids.PushBack(output.m_Uuid);
        pMetaData->m_OutputNames.PushBack(sName);
      }
    }

    pInfo->m_MetaInfo.PushBack(pMetaData);
  }
}

plTransformStatus plSubstancePackageAssetDocument::InternalTransformAsset(const char* szTargetFile, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& assetHeader, plBitflags<plTransformFlags> transformFlags)
{
  plSubstancePackageAssetProperties* pProp = GetProperties();
  plStringBuilder sAbsolutePackagePath = pProp->m_sSubstancePackage;
  if (!plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsolutePackagePath))
  {
    return plStatus(plFmt("Couldn't make path absolute: '{0};", sAbsolutePackagePath));
  }

  PL_SUCCEED_OR_RETURN(UpdateGraphOutputs(sAbsolutePackagePath, transformFlags.IsSet(plTransformFlags::BackgroundProcessing) == false));

  plStringBuilder sTempDir;
  PL_SUCCEED_OR_RETURN(GetTempDir(sTempDir));
  PL_SUCCEED_OR_RETURN(plOSFile::CreateDirectoryStructure(sTempDir));

  PL_SUCCEED_OR_RETURN(RunSbsCooker(sAbsolutePackagePath, sTempDir));

  plStringView sPackageName = sAbsolutePackagePath.GetFileName();

  plStringBuilder sSbsarPath = sTempDir;
  sSbsarPath.AppendPath(sPackageName);
  sSbsarPath.Append(".sbsar");

  plStringBuilder sOutputName, sPngPath, sTargetFile;
  auto& textureTypeDesc = static_cast<const plSubstancePackageAssetDocumentManager*>(GetDocumentManager())->GetTextureTypeDesc();
  const bool bUpdateThumbnail = pAssetProfile == plAssetCurator::GetSingleton()->GetDevelopmentAssetProfile();
  auto pAssetConfig = pAssetProfile->GetTypeConfig<plTextureAssetProfileConfig>();

  for (auto& graph : pProp->m_Graphs)
  {
    if (graph.m_bEnabled == false)
      continue;

    PL_SUCCEED_OR_RETURN(RunSbsRender(sSbsarPath, graph.m_sName, nullptr, nullptr, sTempDir, graph.m_uiOutputWidth, graph.m_uiOutputHeight));

    for (auto& output : graph.m_Outputs)
    {
      if (output.m_bEnabled == false)
        continue;

      sPngPath = sTempDir;
      sPngPath.AppendPath(sPackageName);
      sPngPath.Append("_", graph.m_sName, "_", output.m_sName, ".png");

      GenerateOutputName(graph, output, sOutputName);
      sTargetFile = plStringView(GetDocumentPath()).GetFileDirectory();
      sTargetFile.AppendPath(sOutputName);
      plString sAbsTargetFile = GetAssetDocumentManager()->GetAbsoluteOutputFileName(&textureTypeDesc, sTargetFile, "", pAssetProfile);

      plString sThumbnailFile = GetAssetDocumentManager()->GenerateResourceThumbnailPath(sTargetFile);
      PL_SUCCEED_OR_RETURN(RunTexConv(sPngPath, sAbsTargetFile, assetHeader, output, sThumbnailFile, pAssetConfig));
    }
  }

  return SUPER::InternalTransformAsset(szTargetFile, sOutputTag, pAssetProfile, assetHeader, transformFlags);
}

void plSubstancePackageAssetDocument::OnPropertyChanged(const plDocumentObjectPropertyEvent& e)
{
  if (e.m_EventType == plDocumentObjectPropertyEvent::Type::PropertySet && e.m_sProperty == "SubstanceFile")
  {
    plStringBuilder sAbsolutePackagePath = e.m_NewValue.Get<plString>();
    GetProperties()->m_sSubstancePackage = sAbsolutePackagePath;

    bool bSuccess = plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsolutePackagePath) &&
                    UpdateGraphOutputs(sAbsolutePackagePath, true).Succeeded();
    if (bSuccess == false)
    {
      plLog::Error("Substance package not found or invalid '{}'", sAbsolutePackagePath);
    }
  }
}

plResult plSubstancePackageAssetDocument::GetTempDir(plStringBuilder& out_sTempDir) const
{
  auto szDocumentPath = GetDocumentPath();
  const plString sDataDir = plAssetCurator::GetSingleton()->FindDataDirectoryForAsset(szDocumentPath);

  plStringBuilder sRelativePath(szDocumentPath);
  PL_SUCCEED_OR_RETURN(sRelativePath.MakeRelativeTo(sDataDir));

  out_sTempDir.Set(sDataDir, "/AssetCache/Temp/", sRelativePath.GetFileDirectory());
  out_sTempDir.MakeCleanPath();
  return PL_SUCCESS;
}

void plSubstancePackageAssetDocument::GenerateOutputName(const plSubstanceGraph& graph, const plSubstanceGraphOutput& graphOutput, plStringBuilder& out_sOutputName) const
{
  out_sOutputName = GetProperties()->m_sOutputPattern;
  out_sOutputName.ReplaceAll("$(graph)", graph.m_sName);
  out_sOutputName.ReplaceAll("$(name)", graphOutput.m_sName);
  out_sOutputName.ReplaceAll("$(identifier)", graphOutput.m_sName);
  out_sOutputName.ReplaceAll("$(label)", graphOutput.m_sLabel);

  plStringBuilder sUsage;
  plReflectionUtils::EnumerationToString(graphOutput.m_Usage, sUsage, plReflectionUtils::EnumConversionMode::ValueNameOnly);
  out_sOutputName.ReplaceAll("$(usage)", sUsage);
}

plTransformStatus plSubstancePackageAssetDocument::UpdateGraphOutputs(plStringView sAbsolutePath, bool bAllowPropertyModifications)
{
  plStringBuilder sFileContent;
  PL_SUCCEED_OR_RETURN(GetSbsContent(sAbsolutePath, sFileContent));

  plHybridArray<plSubstanceGraph, 2> graphs;

  QXmlStreamReader reader(sFileContent.GetData());
  PL_SUCCEED_OR_RETURN(ReadUntilStartElement(reader, "content"));

  while (reader.atEnd() == false)
  {
    auto tokenType = reader.readNext();
    PL_IGNORE_UNUSED(tokenType);

    if (reader.isStartElement() && reader.name() == QLatin1StringView("graph"))
    {
      plSubstanceGraph graph;
      PL_SUCCEED_OR_RETURN(ParseGraph(reader, graph));

      if (graph.m_sName.StartsWith("_") == false)
      {
        graphs.PushBack(std::move(graph));
      }
    }
  }

  // Transfer enabled state, label and usage
  plSubstancePackageAssetProperties* pProp = GetProperties();
  for (auto& newGraph : graphs)
  {
    plSubstanceGraph* pExistingGraph = nullptr;
    for (auto& existingGraph : pProp->m_Graphs)
    {
      if (newGraph.m_sName == existingGraph.m_sName)
      {
        newGraph.m_bEnabled = existingGraph.m_bEnabled;
        pExistingGraph = &existingGraph;
        break;
      }
    }

    if (pExistingGraph == nullptr)
      continue;

    for (auto& newOutput : newGraph.m_Outputs)
    {
      plSubstanceGraphOutput* pExistingOutput = nullptr;
      for (auto& existingOutput : pExistingGraph->m_Outputs)
      {
        if (newOutput.m_sName == existingOutput.m_sName)
        {
          newOutput.m_bEnabled = existingOutput.m_bEnabled;
          newOutput.m_CompressionMode = existingOutput.m_CompressionMode;
          newOutput.m_uiNumChannels = existingOutput.m_uiNumChannels;
          newOutput.m_bPreserveAlphaCoverage = existingOutput.m_bPreserveAlphaCoverage;
          newOutput.m_Usage = existingOutput.m_Usage;
          newOutput.m_sLabel = existingOutput.m_sLabel;
          break;
        }
      }
    }
  }

  if (pProp->m_Graphs != graphs)
  {
    if (!bAllowPropertyModifications)
    {
      return plTransformStatus(plTransformResult::NeedsImport);
    }

    GetObjectAccessor()->StartTransaction("Update Graphs");

    pProp->m_Graphs = std::move(graphs);

    ApplyNativePropertyChangesToObjectManager();
    GetObjectAccessor()->FinishTransaction();
  }

  return plStatus(PL_SUCCESS);
}

static const char* s_szTexConvUsageMapping[] = {
  "Auto", // Unknown,

  "Color",     // BaseColor,
  "Color",     // Emissive,
  "Linear",    // Height,
  "Linear",    // Metallic,
  "Linear",    // Mask,
  "NormalMap", // Normal,
  "Linear",    // Occlusion,
  "Linear",    // Opacity,
  "Linear",    // Roughness,
};

static_assert(PL_ARRAY_SIZE(s_szTexConvUsageMapping) == plSubstanceUsage::Count);

static const char* s_szTexConvCompressionMapping[] = {
  "None",
  "Medium",
  "High",
};

static_assert(PL_ARRAY_SIZE(s_szTexConvCompressionMapping) == plTexConvCompressionMode::High + 1);

plStatus plSubstancePackageAssetDocument::RunTexConv(const char* szInputFile, const char* szTargetFile, const plAssetFileHeader& assetHeader, const plSubstanceGraphOutput& graphOutput, plStringView sThumbnailFile, const plTextureAssetProfileConfig* pAssetConfig)
{
  QStringList arguments;
  plStringBuilder temp;

  // Asset Version
  {
    arguments << "-assetVersion";
    arguments << plConversionUtils::ToString(assetHeader.GetFileVersion(), temp).GetData();
  }

  // Asset Hash
  {
    const plUInt64 uiHash64 = assetHeader.GetFileHash();
    const plUInt32 uiHashLow32 = uiHash64 & 0xFFFFFFFF;
    const plUInt32 uiHashHigh32 = (uiHash64 >> 32) & 0xFFFFFFFF;

    temp.SetFormat("{0}", plArgU(uiHashLow32, 8, true, 16, true));
    arguments << "-assetHashLow";
    arguments << temp.GetData();

    temp.SetFormat("{0}", plArgU(uiHashHigh32, 8, true, 16, true));
    arguments << "-assetHashHigh";
    arguments << temp.GetData();
  }

  arguments << "-in0";
  arguments << szInputFile;

  arguments << "-out";
  arguments << szTargetFile;

  if (sThumbnailFile.IsEmpty() == false)
  {
    // Thumbnail
    const plStringView sDir = sThumbnailFile.GetFileDirectory();
    plOSFile::CreateDirectoryStructure(sDir).IgnoreResult();

    arguments << "-thumbnailRes";
    arguments << "256";
    arguments << "-thumbnailOut";

    arguments << QString::fromUtf8(sThumbnailFile.GetData(temp));
  }

  arguments << "-compression";
  arguments << s_szTexConvCompressionMapping[graphOutput.m_CompressionMode];

  arguments << "-maxRes" << QString::number(pAssetConfig->m_uiMaxResolution);

  arguments << "-usage";
  arguments << s_szTexConvUsageMapping[graphOutput.m_Usage];

  switch (graphOutput.m_uiNumChannels)
  {
    case 1:
      arguments << "-r";
      arguments << "in0.r";
      break;

    case 2:
      arguments << "-rg";
      arguments << "in0.rg";
      break;

    case 3:
      arguments << "-rgb";
      arguments << "in0.rgb";
      break;

    default:
      arguments << "-rgba";
      arguments << "in0.rgba";
      break;
  }

  if (graphOutput.m_bPreserveAlphaCoverage)
  {
    arguments << "-mipsPreserveCoverage";
    arguments << "-mipsAlphaThreshold";
    arguments << "0.5";
  }

  PL_SUCCEED_OR_RETURN(plQtEditorApp::GetSingleton()->ExecuteTool("TexConv", arguments, 180, plLog::GetThreadLocalLogSystem()));

  if (sThumbnailFile.IsEmpty() == false)
  {
    plUInt64 uiThumbnailHash = plAssetCurator::GetSingleton()->GetAssetReferenceHash(GetGuid());
    PL_ASSERT_DEV(uiThumbnailHash != 0, "Thumbnail hash should never be zero when reaching this point!");

    ThumbnailInfo thumbnailInfo;
    thumbnailInfo.SetFileHashAndVersion(uiThumbnailHash, GetAssetTypeVersion());
    AppendThumbnailInfo(sThumbnailFile, thumbnailInfo);
  }

  return plStatus(PL_SUCCESS);
}
