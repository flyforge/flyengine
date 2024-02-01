#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/AssetImportDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAssetDocumentGenerator, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plAssetDocumentGenerator::plAssetDocumentGenerator() = default;

plAssetDocumentGenerator::~plAssetDocumentGenerator() = default;

void plAssetDocumentGenerator::AddSupportedFileType(plStringView sExtension)
{
  plStringBuilder tmp = sExtension;
  tmp.ToLower();

  m_SupportedFileTypes.PushBack(tmp);
}

bool plAssetDocumentGenerator::SupportsFileType(plStringView sFile) const
{
  plStringBuilder tmp = plPathUtils::GetFileExtension(sFile);

  if (tmp.IsEmpty())
    tmp = sFile;

  tmp.ToLower();

  return m_SupportedFileTypes.Contains(tmp);
}

void plAssetDocumentGenerator::BuildFileDialogFilterString(plStringBuilder& out_sFilter) const
{
  bool semicolon = false;
  out_sFilter.SetFormat("{0} (", GetDocumentExtension());
  AppendFileFilterStrings(out_sFilter, semicolon);
  out_sFilter.Append(")");
}

void plAssetDocumentGenerator::AppendFileFilterStrings(plStringBuilder& out_sFilter, bool& ref_bSemicolon) const
{
  for (const plString& ext : m_SupportedFileTypes)
  {
    plStringBuilder extWithStarDot;
    extWithStarDot.AppendFormat("*.{0}", ext);

    if (const char* pos = out_sFilter.FindSubString(extWithStarDot.GetData()))
    {
      const char afterExt = *(pos + extWithStarDot.GetElementCount());

      if (afterExt == '\0' || afterExt == ';')
        continue;
    }

    if (ref_bSemicolon)
    {
      out_sFilter.AppendFormat("; {0}", extWithStarDot.GetView());
    }
    else
    {
      out_sFilter.Append(extWithStarDot.GetView());
      ref_bSemicolon = true;
    }
  }
}

void plAssetDocumentGenerator::CreateGenerators(plHybridArray<plAssetDocumentGenerator*, 16>& out_generators)
{
  plRTTI::ForEachDerivedType<plAssetDocumentGenerator>(
    [&](const plRTTI* pRtti)
    {
      out_generators.PushBack(pRtti->GetAllocator()->Allocate<plAssetDocumentGenerator>());
    },
    plRTTI::ForEachOptions::ExcludeNonAllocatable);

  // sort by name
  out_generators.Sort([](plAssetDocumentGenerator* lhs, plAssetDocumentGenerator* rhs) -> bool
    { return lhs->GetDocumentExtension().Compare_NoCase(rhs->GetDocumentExtension()) < 0; });
}

void plAssetDocumentGenerator::DestroyGenerators(const plHybridArray<plAssetDocumentGenerator*, 16>& generators)
{
  for (plAssetDocumentGenerator* pGen : generators)
  {
    pGen->GetDynamicRTTI()->GetAllocator()->Deallocate(pGen);
  }
}

void plAssetDocumentGenerator::ImportAssets(const plDynamicArray<plString>& filesToImport)
{
  plHybridArray<plAssetDocumentGenerator*, 16> generators;
  CreateGenerators(generators);

  plDynamicArray<plAssetDocumentGenerator::ImportGroupOptions> allImports;
  allImports.Reserve(filesToImport.GetCount());

  CreateImportOptionList(filesToImport, allImports, generators);

  SortAndSelectBestImportOption(allImports);

  plQtAssetImportDlg dlg(QApplication::activeWindow(), allImports);
  dlg.exec();

  DestroyGenerators(generators);
}

void plAssetDocumentGenerator::GetSupportsFileTypes(plSet<plString>& out_extensions)
{
  out_extensions.Clear();

  plHybridArray<plAssetDocumentGenerator*, 16> generators;
  CreateGenerators(generators);
  for (auto pGen : generators)
  {
    for (const plString& ext : pGen->m_SupportedFileTypes)
    {
      out_extensions.Insert(ext);
    }
  }
  DestroyGenerators(generators);
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

void plAssetDocumentGenerator::CreateImportOptionList(const plDynamicArray<plString>& filesToImport, plDynamicArray<plAssetDocumentGenerator::ImportGroupOptions>& allImports, const plHybridArray<plAssetDocumentGenerator*, 16>& generators)
{
  plQtEditorApp* pApp = plQtEditorApp::GetSingleton();
  plStringBuilder sInputRelative, sGroup;

  for (const plString& sInputAbsolute : filesToImport)
  {
    sInputRelative = sInputAbsolute;

    if (!pApp->MakePathDataDirectoryRelative(sInputRelative))
    {
      // error, file is not in data directory -> skip
      continue;
    }

    for (plAssetDocumentGenerator* pGen : generators)
    {
      if (pGen->SupportsFileType(sInputRelative))
      {
        sGroup = pGen->GetGeneratorGroup();

        ImportGroupOptions* pData = nullptr;
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
          pData->m_sInputFileRelative = sInputRelative;
        }

        plHybridArray<plAssetDocumentGenerator::ImportMode, 4> options;
        pGen->GetImportModes(sInputAbsolute, options);

        for (auto& option : options)
        {
          option.m_pGenerator = pGen;
        }

        pData->m_ImportOptions.PushBackRange(options);
      }
    }
  }
}

void plAssetDocumentGenerator::SortAndSelectBestImportOption(plDynamicArray<plAssetDocumentGenerator::ImportGroupOptions>& allImports)
{
  allImports.Sort([](const plAssetDocumentGenerator::ImportGroupOptions& lhs, const plAssetDocumentGenerator::ImportGroupOptions& rhs) -> bool
    { return lhs.m_sInputFileRelative < rhs.m_sInputFileRelative; });

  for (auto& singleImport : allImports)
  {
    singleImport.m_ImportOptions.Sort([](const plAssetDocumentGenerator::ImportMode& lhs, const plAssetDocumentGenerator::ImportMode& rhs) -> bool
      { return plTranslate(lhs.m_sName).Compare_NoCase(plTranslate(rhs.m_sName)) < 0; });

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

plStatus plAssetDocumentGenerator::Import(plStringView sInputFileAbs, plStringView sMode, bool bOpenDocument)
{
  plStringBuilder ext = sInputFileAbs.GetFileExtension();
  ext.ToLower();

  if (!m_SupportedFileTypes.Contains(ext))
    return plStatus(plFmt("Files of type '{}' cannot be imported as '{}' documents.", ext, GetDocumentExtension()));

  plDocument* pGeneratedDoc = nullptr;
  PL_SUCCEED_OR_RETURN(Generate(sInputFileAbs, sMode, pGeneratedDoc));

  PL_ASSERT_DEV(pGeneratedDoc != nullptr, "");

  const plString sDocPath = pGeneratedDoc->GetDocumentPath();

  pGeneratedDoc->SaveDocument(true).LogFailure();
  pGeneratedDoc->GetDocumentManager()->CloseDocument(pGeneratedDoc);

  if (bOpenDocument)
  {
    plQtEditorApp::GetSingleton()->OpenDocumentQueued(sDocPath);
  }

  return plStatus(PL_SUCCESS);
}
