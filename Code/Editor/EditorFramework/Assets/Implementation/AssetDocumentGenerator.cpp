#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/AssetImportDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAssetDocumentGenerator, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plAssetDocumentGenerator::plAssetDocumentGenerator() {}

plAssetDocumentGenerator::~plAssetDocumentGenerator() {}

void plAssetDocumentGenerator::AddSupportedFileType(plStringView sExtension)
{
  plStringBuilder tmp = sExtension;
  tmp.ToLower();

  m_SupportedFileTypes.PushBack(tmp);
}

bool plAssetDocumentGenerator::SupportsFileType(plStringView sFile) const
{
  plStringBuilder tmp = plPathUtils::GetFileExtension(sFile);
  tmp.ToLower();

  return m_SupportedFileTypes.Contains(tmp);
}

void plAssetDocumentGenerator::BuildFileDialogFilterString(plStringBuilder& out_Filter) const
{
  bool semicolon = false;
  out_Filter.Format("{0} (", GetDocumentExtension());
  AppendFileFilterStrings(out_Filter, semicolon);
  out_Filter.Append(")");
}

void plAssetDocumentGenerator::AppendFileFilterStrings(plStringBuilder& out_Filter, bool& semicolon) const
{
  for (const plString ext : m_SupportedFileTypes)
  {
    plStringBuilder extWithStarDot;
    extWithStarDot.AppendFormat("*.{0}", ext);

    if (const char* pos = out_Filter.FindSubString(extWithStarDot.GetData()))
    {
      const char afterExt = *(pos + extWithStarDot.GetElementCount());

      if (afterExt == '\0' || afterExt == ';')
        continue;
    }

    if (semicolon)
    {
      out_Filter.AppendFormat("; {0}", extWithStarDot.GetView());
    }
    else
    {
      out_Filter.Append(extWithStarDot.GetView());
      semicolon = true;
    }
  }
}

void plAssetDocumentGenerator::CreateGenerators(plHybridArray<plAssetDocumentGenerator*, 16>& out_Generators)
{
  plRTTI::ForEachDerivedType<plAssetDocumentGenerator>(
    [&](const plRTTI* pRtti) {
      out_Generators.PushBack(pRtti->GetAllocator()->Allocate<plAssetDocumentGenerator>());
    },
    plRTTI::ForEachOptions::ExcludeNonAllocatable);
  
  // sort by name
  out_Generators.Sort([](plAssetDocumentGenerator* lhs, plAssetDocumentGenerator* rhs) -> bool { return lhs->GetDocumentExtension().Compare_NoCase(rhs->GetDocumentExtension()) < 0; });
}

void plAssetDocumentGenerator::DestroyGenerators(plHybridArray<plAssetDocumentGenerator*, 16>& generators)
{
  for (plAssetDocumentGenerator* pGen : generators)
  {
    pGen->GetDynamicRTTI()->GetAllocator()->Deallocate(pGen);
  }

  generators.Clear();
}


void plAssetDocumentGenerator::ExecuteImport(plDynamicArray<ImportData>& allImports)
{
  for (auto& data : allImports)
  {
    if (data.m_iSelectedOption < 0)
      continue;

    PLASMA_LOG_BLOCK("Asset Import", data.m_sInputFileParentRelative);

    auto& option = data.m_ImportOptions[data.m_iSelectedOption];

    if (DetermineInputAndOutputFiles(data, option).Failed())
      continue;

    plDocument* pGeneratedDoc = nullptr;
    const plStatus status = option.m_pGenerator->Generate(data.m_sInputFileRelative, option, pGeneratedDoc);

    if (pGeneratedDoc)
    {
      pGeneratedDoc->SaveDocument(true).IgnoreResult();
      pGeneratedDoc->GetDocumentManager()->CloseDocument(pGeneratedDoc);
    }

    if (status.Failed())
    {
      data.m_sImportMessage = status.m_sMessage;
      plLog::Error("Asset import failed: '{0}'", status.m_sMessage);
    }
    else
    {
      data.m_sImportMessage.Clear();
      data.m_bDoNotImport = true;
      plLog::Success("Generated asset document '{0}'", option.m_sOutputFileAbsolute);
    }
  }
}


plResult plAssetDocumentGenerator::DetermineInputAndOutputFiles(ImportData& data, Info& option)
{
  auto pApp = plQtEditorApp::GetSingleton();

  plStringBuilder inputFile = data.m_sInputFileParentRelative;
  if (!pApp->MakeParentDataDirectoryRelativePathAbsolute(inputFile, true))
  {
    data.m_sImportMessage = "Input file could not be located";
    return PLASMA_FAILURE;
  }

  data.m_sInputFileAbsolute = inputFile;

  if (!pApp->MakePathDataDirectoryRelative(inputFile))
  {
    data.m_sImportMessage = "Input file is not in any known data directory";
    return PLASMA_FAILURE;
  }

  data.m_sInputFileRelative = inputFile;

  plStringBuilder outputFile = option.m_sOutputFileParentRelative;
  plStringBuilder fileName = outputFile.GetFileName();
  fileName.AppendFormat("_{}", option.m_pGenerator->GetNameSuffix());
  outputFile.ChangeFileName(fileName);
  if (!pApp->MakeParentDataDirectoryRelativePathAbsolute(outputFile, false))
  {
    data.m_sImportMessage = "Target file location could not be found";
    return PLASMA_FAILURE;
  }

  option.m_sOutputFileAbsolute = outputFile;

  // don't create it when it already exists
  //TODO : review as there is now two layers of overwrite protection (import of the original file and this one), which can be confusing
  // add option to overwrite?
  if (plOSFile::ExistsFile(outputFile))
  {
    data.m_bDoNotImport = true;
    data.m_sImportMessage = "Target file already exists";
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

bool plAssetDocumentGenerator::ImportAssets(const plHybridArray<plString, 16>& filesToImport)
{
  plHybridArray<plAssetDocumentGenerator*, 16> generators;
  CreateGenerators(generators);

  plDynamicArray<plAssetDocumentGenerator::ImportData> allImports;
  allImports.Reserve(filesToImport.GetCount());

  CreateImportOptionList(filesToImport, allImports, generators);

  SortAndSelectBestImportOption(allImports);

  bool imported;
  plQtAssetImportDlg dlg(QApplication::activeWindow(), allImports, imported);
  dlg.exec();

  DestroyGenerators(generators);
  return imported;
}

void plAssetDocumentGenerator::ImportAssets()
{
  plHybridArray<plAssetDocumentGenerator*, 16> generators;
  CreateGenerators(generators);

  plStringBuilder singleFilter, fullFilter, allExtensions;
  bool semicolon = false;

  for (auto pGen : generators)
  {
    pGen->AppendFileFilterStrings(allExtensions, semicolon);
    pGen->BuildFileDialogFilterString(singleFilter);
    fullFilter.Append(singleFilter, "\n");
  }

  fullFilter.Append("All files (*.*)");
  fullFilter.Prepend("All asset files (", allExtensions, ")\n");

  static plStringBuilder s_StartDir;
  if (s_StartDir.IsEmpty())
  {
    s_StartDir = plToolsProject::GetSingleton()->GetProjectDirectory();
  }

  QStringList filenames = QFileDialog::getOpenFileNames(QApplication::activeWindow(), "Import Assets", s_StartDir.GetData(),
    QString::fromUtf8(fullFilter.GetData()), nullptr, QFileDialog::Option::DontResolveSymlinks);

  DestroyGenerators(generators);

  if (filenames.empty())
    return;

  s_StartDir = filenames[0].toUtf8().data();
  s_StartDir.PathParentDirectory();

  plHybridArray<plString, 16> filesToImport;
  for (QString s : filenames)
  {
    filesToImport.PushBack(s.toUtf8().data());
  }

  ImportAssets(filesToImport);
}

void plAssetDocumentGenerator::CreateImportOptionList(const plHybridArray<plString, 16>& filesToImport,
  plDynamicArray<plAssetDocumentGenerator::ImportData>& allImports, const plHybridArray<plAssetDocumentGenerator*, 16>& generators)
{
  plQtEditorApp* pApp = plQtEditorApp::GetSingleton();
  plStringBuilder sInputParentRelative, sInputRelative, sGroup;

  for (const plString& sInputAbsolute : filesToImport)
  {
    sInputParentRelative = sInputAbsolute;
    sInputRelative = sInputAbsolute;

    if (!pApp->MakePathDataDirectoryParentRelative(sInputParentRelative) || !pApp->MakePathDataDirectoryRelative(sInputRelative))
    {
      auto& data = allImports.ExpandAndGetRef();
      data.m_sInputFileAbsolute = sInputAbsolute;
      data.m_sInputFileParentRelative = sInputParentRelative;
      data.m_sInputFileRelative = sInputRelative;
      data.m_sImportMessage = "File is not located in any data directory.";
      data.m_bDoNotImport = true;
      continue;
    }

    for (plAssetDocumentGenerator* pGen : generators)
    {
      if (pGen->SupportsFileType(sInputParentRelative))
      {
        sGroup = pGen->GetGeneratorGroup();

        ImportData* pData = nullptr;
        for (auto& importer : allImports)
        {
          if (importer.m_sGroup == sGroup && importer.m_sInputFileAbsolute == sInputAbsolute)
          {
            pData = &importer;
          }
        }

        if (pData == nullptr)
        {
          pData = &allImports.ExpandAndGetRef();
          pData->m_sGroup = sGroup;
          pData->m_sInputFileAbsolute = sInputAbsolute;
          pData->m_sInputFileParentRelative = sInputParentRelative;
          pData->m_sInputFileRelative = sInputRelative;
        }

        plHybridArray<plAssetDocumentGenerator::Info, 4> options;
        pGen->GetImportModes(sInputParentRelative, options);

        for (auto& option : options)
        {
          option.m_pGenerator = pGen;
        }

        pData->m_ImportOptions.PushBackRange(options);
      }
    }
  }
}

void plAssetDocumentGenerator::SortAndSelectBestImportOption(plDynamicArray<plAssetDocumentGenerator::ImportData>& allImports)
{
  allImports.Sort([](const plAssetDocumentGenerator::ImportData& lhs, const plAssetDocumentGenerator::ImportData& rhs) -> bool
  {
      return lhs.m_sInputFileParentRelative < rhs.m_sInputFileParentRelative;
  });

  for (auto& singleImport : allImports)
  {
    singleImport.m_ImportOptions.Sort([](const plAssetDocumentGenerator::Info& lhs, const plAssetDocumentGenerator::Info& rhs) -> bool { return plStringUtils::Compare_NoCase(plTranslate(lhs.m_sName), plTranslate(rhs.m_sName)) < 0; });

    plUInt32 uiNumPrios[(plUInt32)plAssetDocGeneratorPriority::ENUM_COUNT] = {0};
    plUInt32 uiBestPrio[(plUInt32)plAssetDocGeneratorPriority::ENUM_COUNT] = {0};

    for (plUInt32 i = 0; i < singleImport.m_ImportOptions.GetCount(); ++i)
    {
      uiNumPrios[(plUInt32)singleImport.m_ImportOptions[i].m_Priority]++;
      uiBestPrio[(plUInt32)singleImport.m_ImportOptions[i].m_Priority] = i;
    }

    singleImport.m_iSelectedOption = -1;
    for (plUInt32 prio = (plUInt32)plAssetDocGeneratorPriority::HighPriority; prio > (plUInt32)plAssetDocGeneratorPriority::Undecided; --prio)
    {
      if (uiNumPrios[prio] == 1)
      {
        singleImport.m_iSelectedOption = uiBestPrio[prio];
        break;
      }

      if (uiNumPrios[prio] > 1)
        break;
    }
  }
}
