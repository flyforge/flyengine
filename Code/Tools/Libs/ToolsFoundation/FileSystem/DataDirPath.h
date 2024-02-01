#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <Foundation/IO/Stream.h>

/// \brief A reference to a file or folder inside a data directory.
///
/// Allows quick access to various sub-parts of the path as well as the data dir index.
/// To construct a plDataDirPath, the list of absolute data directory root directories must be supplied in order to determine whether the path is inside a data directory and in which. After calling the constructor, IsValid() should be called to determine if the file is inside a data directory.
/// The various sub-parts look like this with "Testing Chambers" being the data directory in this example:
///
///  GetAbsolutePath() == "C:/plEngine/Data/Samples/Testing Chambers/Objects/Barrel.plPrefab"
///  GetDataDir() == "C:/plEngine/Data/Samples/Testing Chambers"
///  GetDataDirParentRelativePath() == "Testing Chambers/Objects/Barrel.plPrefab"
///  GetDataDirRelativePath() == "Objects/Barrel.plPrefab"
class PL_TOOLSFOUNDATION_DLL plDataDirPath
{
public:
  /// \name Constructor
  ///@{

  /// \brief Default ctor, creates an invalid data directory path.
  plDataDirPath();
  /// \brief Tries to create a new data directory path from an absolute path. Check IsValid afterwards to confirm this path is inside a data directory.
  /// \param sAbsPath Absolute path to the file or folder. Must be normalized. Must not end with "/".
  /// \param dataDirRoots A list of normalized absolute paths to the roots of the data directories. These must not end in a "/" character.
  /// \param uiLastKnownDataDirIndex A hint to accelerate the search if the data directory index is known.
  plDataDirPath(plStringView sAbsPath, plArrayPtr<plString> dataDirRoots, plUInt32 uiLastKnownDataDirIndex = 0);
  /// \brief Overload for plStringBuilder to fix ambiguity between implicit conversions.
  plDataDirPath(const plStringBuilder& sAbsPath, plArrayPtr<plString> dataDirRoots, plUInt32 uiLastKnownDataDirIndex = 0);
  /// \brief Move constructor overload for the absolute path.
  plDataDirPath(plString&& sAbsPath, plArrayPtr<plString> dataDirRoots, plUInt32 uiLastKnownDataDirIndex = 0);


  ///@}
  /// \name Misc
  ///@{

  /// \brief Returns the same path this instance was created with. Calling this function is always valid.
  const plString& GetAbsolutePath() const;
  /// \brief Returns whether this path is inside a data directory. If not, none of the Get* functions except for GetAbsolutePath are allowed to be called.
  bool IsValid() const;
  /// \brief Same as the default constructor. Creates an empty, invalid path.
  void Clear();

  ///@}
  /// \name Operators
  ///@{

  operator plStringView() const;
  bool operator==(plStringView rhs) const;
  bool operator!=(plStringView rhs) const;

  ///@}
  /// \name Data directory access. Only allowed to be called if IsValid() is true.
  ///@{

  /// \brief Returns a relative path including the data directory the path belongs to, e.g. "Testing Chambers/Objects/Barrel.plPrefab".
  plStringView GetDataDirParentRelativePath() const;
  /// \brief Returns a path relative to the data directory the path belongs to, e.g. "Objects/Barrel.plPrefab".
  plStringView GetDataDirRelativePath() const;
  /// \brief Returns absolute path to the data directory this path belongs to, e.g.  "C:/plEngine/Data/Samples/Testing Chambers".
  plStringView GetDataDir() const;
  /// \brief Returns the index of the data directory the path belongs to.
  plUInt8 GetDataDirIndex() const;

  ///@}
  /// \name Data directory update
  ///@{

  /// \brief If a plDataDirPath is de-serialized, it might not be correct anymore and its data directory reference must be updated. It could potentially no longer be part of any data directory at all and become invalid so after calling this function, IsValid will match the return value of this function. On failure, the invalid data directory paths should then be destroyed.
  /// \param dataDirRoots A list of normalized absolute paths to the roots of the data directories. These must not end in a "/" character.
  /// \param uiLastKnownDataDirIndex A hint to accelerate the search if nothing has changed.
  /// \return Returns whether the data directory path is valid, i.e. it is still under one of the dataDirRoots.
  bool UpdateDataDirInfos(plArrayPtr<plString> dataDirRoots, plUInt32 uiLastKnownDataDirIndex = 0) const;

  ///@}
  /// \name Serialization
  ///@{

  plStreamWriter& Write(plStreamWriter& inout_stream) const;
  plStreamReader& Read(plStreamReader& inout_stream);

  ///@}

private:
  plString m_sAbsolutePath;
  mutable plUInt16 m_uiDataDirParent = 0;
  mutable plUInt8 m_uiDataDirLength = 0;
  mutable plUInt8 m_uiDataDirIndex = 0;
};

plStreamWriter& operator<<(plStreamWriter& inout_stream, const plDataDirPath& value);
plStreamReader& operator>>(plStreamReader& inout_stream, plDataDirPath& out_value);

/// \brief Comparator that first sort case-insensitive and then case-sensitive if necessary for a unique ordering.
///
/// Use this comparator when sorting e.g. files on disk like they would appear in a windows explorer.
/// This comparator is using plStringView instead of plDataDirPath as all string and plDataDirPath can be implicitly converted to plStringView.
struct PL_TOOLSFOUNDATION_DLL plCompareDataDirPath
{
  static inline bool Less(plStringView lhs, plStringView rhs);
  static inline bool Equal(plStringView lhs, plStringView rhs);
};

#include <ToolsFoundation/FileSystem/Implementation/DataDirPath_inl.h>
