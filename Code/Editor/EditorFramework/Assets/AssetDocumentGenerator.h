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

/// \brief Provides functionality for importing files as asset documents.
///
/// Derived from this class to add a custom importer (see existing derived classes for examples).
/// Each importer typically handles one target asset type.
class PL_EDITORFRAMEWORK_DLL plAssetDocumentGenerator : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plAssetDocumentGenerator, plReflectedClass);

public:
  plAssetDocumentGenerator();
  ~plAssetDocumentGenerator();

  /// \brief Describes one option to import an asset.
  ///
  /// The name is used to identify which option the user chose.
  /// The priority should be set to pick a 'likely' option for the UI to prefer.
  struct ImportMode
  {
    plAssetDocumentGenerator* m_pGenerator = nullptr; ///< automatically set by plAssetDocumentGenerator
    plAssetDocGeneratorPriority m_Priority = plAssetDocGeneratorPriority::Undecided;
    plString m_sName;
    plString m_sIcon;
  };

  /// \brief Creates a list of all importable file extensions. Note that this is an expensive function so the the result should be cached.
  /// \param out_Extensions List of all file extensions that can be imported.
  static void GetSupportsFileTypes(plSet<plString>& out_extensions);

  /// \brief Opens a file browse dialog to let the user choose which files to import.
  ///
  /// After the user chose one or multiple files, opens the "Asset Import" dialog to let them choose details.
  static void ImportAssets();

  /// \brief Opens the "Asset Import" dialog to let the user choose how to import the given files.
  static void ImportAssets(const plDynamicArray<plString>& filesToImport);

  /// \brief Imports the given file with the mode. Must be a mode that the generator supports.
  plStatus Import(plStringView sInputFileAbs, plStringView sMode, bool bOpenDocument);

  /// \brief Used to fill out which import modes may be available for the given asset.
  ///
  /// Note: sAbsInputFile may be empty, in this case it should fill out the array for "general purpose" import (any file of the supported types).
  virtual void GetImportModes(plStringView sAbsInputFile, plDynamicArray<ImportMode>& out_modes) const = 0;

  /// \brief Returns the target asset document file extension.
  virtual plStringView GetDocumentExtension() const = 0;

  /// \brief Allows to merge the import modes of multiple generators in the UI in one group.
  virtual plStringView GetGeneratorGroup() const = 0;

  /// \brief Tells the generator to create a new asset document with the chosen mode.
  virtual plStatus Generate(plStringView sInputFileAbs, plStringView sMode, plDocument*& out_pGeneratedDocument) = 0;

  /// \brief Returns whether this generator supports the given file type for import.
  bool SupportsFileType(plStringView sFile) const;

  /// \brief Instantiates all currently available generators.
  static void CreateGenerators(plHybridArray<plAssetDocumentGenerator*, 16>& out_generators);

  /// \brief Destroys the previously instantiated generators.
  static void DestroyGenerators(const plHybridArray<plAssetDocumentGenerator*, 16>& generators);

protected:
  void AddSupportedFileType(plStringView sExtension);

private:
  void BuildFileDialogFilterString(plStringBuilder& out_sFilter) const;
  void AppendFileFilterStrings(plStringBuilder& out_sFilter, bool& ref_bSemicolon) const;

  friend class plQtAssetImportDlg;

  struct ImportGroupOptions
  {
    plString m_sGroup;
    plString m_sInputFileRelative;
    plString m_sInputFileAbsolute;
    plInt32 m_iSelectedOption = -1;

    plHybridArray<plAssetDocumentGenerator::ImportMode, 4> m_ImportOptions;
  };

  static void SortAndSelectBestImportOption(plDynamicArray<plAssetDocumentGenerator::ImportGroupOptions>& allImports);
  static void CreateImportOptionList(const plDynamicArray<plString>& filesToImport, plDynamicArray<plAssetDocumentGenerator::ImportGroupOptions>& allImports, const plHybridArray<plAssetDocumentGenerator*, 16>& generators);

  plHybridArray<plString, 16> m_SupportedFileTypes;
};
