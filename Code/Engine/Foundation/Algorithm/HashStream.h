#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>

/// \brief A stream writer that hashes the data written to it.
///
/// This stream writer allows to conveniently generate a 32 bit hash value for any kind of data.
class PL_FOUNDATION_DLL plHashStreamWriter32 : public plStreamWriter
{
public:
  /// \brief Pass an initial seed for the hash calculation.
  plHashStreamWriter32(plUInt32 uiSeed = 0);
  ~plHashStreamWriter32();

  /// \brief Writes bytes directly to the stream.
  virtual plResult WriteBytes(const void* pWriteBuffer, plUInt64 uiBytesToWrite) override;

  /// \brief Returns the current hash value. You can read this at any time between write operations, or after writing is done to get the final hash
  /// value.
  plUInt32 GetHashValue() const;

private:
  void* m_pState = nullptr;
};


/// \brief A stream writer that hashes the data written to it.
///
/// This stream writer allows to conveniently generate a 64 bit hash value for any kind of data.
class PL_FOUNDATION_DLL plHashStreamWriter64 : public plStreamWriter
{
public:
  /// \brief Pass an initial seed for the hash calculation.
  plHashStreamWriter64(plUInt64 uiSeed = 0);
  ~plHashStreamWriter64();

  /// \brief Writes bytes directly to the stream.
  virtual plResult WriteBytes(const void* pWriteBuffer, plUInt64 uiBytesToWrite) override;

  /// \brief Returns the current hash value. You can read this at any time between write operations, or after writing is done to get the final hash
  /// value.
  plUInt64 GetHashValue() const;

private:
  void* m_pState = nullptr;
};
