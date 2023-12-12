#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Status.h>

class plDocument;

enum class plAssetDocGeneratorPriority
{
  Undecided,
  LowPriority,
  DefaultPriority,
  HighPriority,
  ENUM_COUNT
};

class PLASMA_EDITORFRAMEWORK_DLL plAssetDocumentGenerator : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAssetDocumentGenerator, plReflectedClass);

public:
  plAssetDocumentGenerator();
  ~plAssetDocumentGenerator();

  struct Info
  {
    plAssetDocumentGenerator* m_pGenerator = nullptr; ///< automatically set by plAssetDocumentGenerator
    plAssetDocGeneratorPriority m_Priority;           ///< has to be specified by generator
    plString m_sOutputFileParentRelative;             ///< has to be specified by generator
    plString m_sOutputFileAbsolute;                   ///< automatically generated from m_sOutputFileParentRelative
    plString m_sName;                                 ///< has to be specified by generator, used to know which action to take by Generate()
    plString m_sIcon;                                 ///< has to be specified by generator
  };

  struct ImportData
  {
    plString m_sGroup;
    plString m_sInputFileRelative;
    plString m_sInputFileParentRelative;
    plString m_sInputFileAbsolute;
    plInt32 m_iSelectedOption = -1;
    plString m_sImportMessage; // error text or "already exists"
    bool m_bDoNotImport = false;

    plHybridArray<plAssetDocumentGenerator::Info, 4> m_ImportOptions;
  };

  static void ImportAssets();
  static bool ImportAssets(const plHybridArray<plString, 16>& filesToImport);
  static void ExecuteImport(plDynamicArray<plAssetDocumentGenerator::ImportData>& allImports);

  virtual void GetImportModes(plStringView sParentDirRelativePath, plHybridArray<plAssetDocumentGenerator::Info, 4>& out_Modes) const = 0;
  virtual plStatus Generate(plStringView sDataDirRelativePath, const plAssetDocumentGenerator::Info& mode, plDocument*& out_pGeneratedDocument) = 0;
  virtual plStringView GetDocumentExtension() const = 0;
  virtual plStringView GetGeneratorGroup() const = 0;
  virtual plStringView GetNameSuffix() const = 0;

  bool SupportsFileType(plStringView sFile) const;
  void BuildFileDialogFilterString(plStringBuilder& out_Filter) const;
  void AppendFileFilterStrings(plStringBuilder& out_Filter, bool& semicolon) const;

protected:
  void AddSupportedFileType(plStringView sExtension);

private:
  static void CreateGenerators(plHybridArray<plAssetDocumentGenerator*, 16>& out_Generators);
  static void DestroyGenerators(plHybridArray<plAssetDocumentGenerator*, 16>& generators);
  static plResult DetermineInputAndOutputFiles(ImportData& data, Info& option);
  static void SortAndSelectBestImportOption(plDynamicArray<plAssetDocumentGenerator::ImportData>& allImports);
  static void CreateImportOptionList(const plHybridArray<plString, 16>& filesToImport, plDynamicArray<plAssetDocumentGenerator::ImportData>& allImports, const plHybridArray<plAssetDocumentGenerator*, 16>& generators);

  plHybridArray<plString, 16> m_SupportedFileTypes;
};
