#pragma once

#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>

/// \brief A file writer that caches all written data and only opens and writes to the output file when everything is finished.
/// Useful to ensure that only complete files are written, or nothing at all, in case of a crash.
class PLASMA_FOUNDATION_DLL plDeferredFileWriter : public plStreamWriter
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plDeferredFileWriter);

public:
  plDeferredFileWriter();

  /// \brief Upon destruction the file is closed and thus written, unless Discard was called before.
  ~plDeferredFileWriter() { Close().IgnoreResult(); }

  /// \brief This must be configured before anything is written to the file.
  void SetOutput(plStringView sFileToWriteTo, bool bOnlyWriteIfDifferent = false); // [tested]

  virtual plResult WriteBytes(const void* pWriteBuffer, plUInt64 uiBytesToWrite) override; // [tested]

  /// \brief Upon calling this the content is written to the file specified with SetOutput().
  /// The return value is PLASMA_FAILURE if the file could not be opened or not completely written.
  plResult Close(bool* out_pWasWrittenTo = nullptr); // [tested]

  /// \brief Calling this abandons the content and a later Close or destruction of the instance
  /// will no longer write anything to file.
  void Discard(); // [tested]

private:
  bool m_bOnlyWriteIfDifferent = false;
  bool m_bAlreadyClosed = false;
  plString m_sOutputFile;
  plDefaultMemoryStreamStorage m_Storage;
  plMemoryStreamWriter m_Writer;
};
