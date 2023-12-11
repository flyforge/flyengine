#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/FileSystem/Implementation/FileReaderWriterBase.h>
#include <Foundation/IO/Stream.h>

/// \brief The default class to use to write data to a file, implements the plStreamWriter interface.
///
/// This file writer buffers writes up to a certain amount of bytes (configurable).
/// It closes the file automatically once it goes out of scope.
class PLASMA_FOUNDATION_DLL plFileWriter : public plFileWriterBase
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plFileWriter);

public:
  /// \brief Constructor, does nothing.
  plFileWriter() = default;

  /// \brief Destructor, closes the file, if it is still open (RAII).
  ~plFileWriter() { Close(); }

  /// \brief Opens the given file for writing. Returns PLASMA_SUCCESS if the file could be opened. A cache is created to speed up small writes.
  ///
  /// You should typically not disable bAllowFileEvents, unless you need to prevent recursive file events,
  /// which is only the case, if you are doing file accesses from within a File Event Handler.
  plResult Open(plStringView sFile, plUInt32 uiCacheSize = 1024 * 1024, plFileShareMode::Enum fileShareMode = plFileShareMode::Default,
    bool bAllowFileEvents = true);

  /// \brief Closes the file, if it is open.
  void Close();

  /// \brief Writes the given number of bytes to the file. Returns PLASMA_SUCCESS if all bytes were successfully written.
  ///
  /// As this class buffers writes with an internal cache, PLASMA_SUCCESS does NOT mean that the data is actually written to disk.
  virtual plResult WriteBytes(const void* pWriteBuffer, plUInt64 uiBytesToWrite) override;

  /// \brief Will write anything that's currently in the write-cache to disk. Will decrease performance if used excessively.
  ///
  /// \note Flush only guarantees that the data is sent through the OS file functions. It does not guarantee that the OS
  /// actually wrote the data on the disk, it might still use buffer itself and thus an application that crashes might
  /// still see data loss even when 'Flush' had been called.
  virtual plResult Flush() override;

private:
  plUInt64 m_uiCacheWritePosition;
  plDynamicArray<plUInt8> m_Cache;
};