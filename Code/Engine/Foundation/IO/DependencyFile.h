#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Time/Timestamp.h>

/// \brief This class represents a set of files of which one wants to know when any one of them changes.
///
/// plDependencyFile stores a list of files that are the 'dependency set'. It can be serialized.
/// Through HasAnyFileChanged() one can detect whether any of the files has changed, since the last call to StoreCurrentTimeStamp().
/// The time stamp that is retrieved through StoreCurrentTimeStamp() will also be serialized.
class PL_FOUNDATION_DLL plDependencyFile
{
public:
  plDependencyFile();

  /// \brief Clears all files that were added with AddFileDependency()
  void Clear();

  /// \brief Adds one file as a dependency to the list
  void AddFileDependency(plStringView sFile);

  /// \brief Allows read access to all currently stored file dependencies
  const plHybridArray<plString, 16>& GetFileDependencies() const { return m_AssetTransformDependencies; }

  /// \brief Writes the current state to a stream. Note that you probably should call StoreCurrentTimeStamp() before this, to serialize the latest
  /// file stamp
  plResult WriteDependencyFile(plStreamWriter& inout_stream) const;

  /// \brief Reads the state from a stream. Call HasAnyFileChanged() afterwards to determine whether anything has changed since when the data was
  /// serialized.
  plResult ReadDependencyFile(plStreamReader& inout_stream);

  /// \brief Writes the current state to a file. Note that you probably should call StoreCurrentTimeStamp() before this, to serialize the latest file
  /// stamp
  plResult WriteDependencyFile(plStringView sFile) const;

  /// \brief Reads the state from a file. Call HasAnyFileChanged() afterwards to determine whether anything has changed since when the data was
  /// serialized.
  plResult ReadDependencyFile(plStringView sFile);

  /// \brief Retrieves the current file time stamps from the filesystem and determines whether any file has changed since the last call to
  /// StoreCurrentTimeStamp() (or ReadDependencyFile())
  bool HasAnyFileChanged() const;

  /// \brief Retrieves the current file time stamps from the filesystem and stores it for later comparison. This value is also serialized through
  /// WriteDependencyFile(), so it should be called before that, to store the latest state.
  void StoreCurrentTimeStamp();

private:
  static plResult RetrieveFileTimeStamp(plStringView sFile, plTimestamp& out_Result);

  plHybridArray<plString, 16> m_AssetTransformDependencies;
  plInt64 m_iMaxTimeStampStored = 0;
  plUInt64 m_uiSumTimeStampStored = 0;

  struct FileCheckCache
  {
    plTimestamp m_FileTimestamp;
    plTime m_LastCheck;
  };

  static plMap<plString, FileCheckCache> s_FileTimestamps;
};
