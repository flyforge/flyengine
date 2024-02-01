#pragma once

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <RendererCore/RendererCoreDLL.h>
#include <ozz/base/io/stream.h>

namespace ozz::animation
{
  class Skeleton;
  class Animation;
}; // namespace ozz::animation

/// \brief Stores or gather the data for an ozz file, for random access operations (seek / tell).
///
/// Since ozz::io::Stream requires seek/tell functionality, it cannot be implemented with basic plStreamReader / plStreamWriter.
/// Instead, we must have the entire ozz archive data in memory, to be able to jump around arbitrarily.
class PL_RENDERERCORE_DLL plOzzArchiveData
{
  PL_DISALLOW_COPY_AND_ASSIGN(plOzzArchiveData);

public:
  plOzzArchiveData();
  ~plOzzArchiveData();

  plResult FetchRegularFile(const char* szFile);
  plResult FetchEmbeddedArchive(plStreamReader& inout_stream);
  plResult StoreEmbeddedArchive(plStreamWriter& inout_stream) const;

  plDefaultMemoryStreamStorage m_Storage;
};

/// \brief Implements the ozz::io::Stream interface for reading. The data has to be present in an plOzzArchiveData object.
///
/// The class is implemented inline and not DLL exported because ozz is only available as a static library.
class PL_RENDERERCORE_DLL plOzzStreamReader : public ozz::io::Stream
{
public:
  plOzzStreamReader(const plOzzArchiveData& data);

  virtual bool opened() const override;

  virtual size_t Read(void* pBuffer, size_t uiSize) override;

  virtual size_t Write(const void* pBuffer, size_t uiSize) override;

  virtual int Seek(int iOffset, Origin origin) override;

  virtual int Tell() const override;

  virtual size_t Size() const override;

private:
  plMemoryStreamReader m_Reader;
};

/// \brief Implements the ozz::io::Stream interface for writing. The data is gathered in an plOzzArchiveData object.
///
/// The class is implemented inline and not DLL exported because ozz is only available as a static library.
class PL_RENDERERCORE_DLL plOzzStreamWriter : public ozz::io::Stream
{
public:
  plOzzStreamWriter(plOzzArchiveData& ref_data);

  virtual bool opened() const override;

  virtual size_t Read(void* pBuffer, size_t uiSize) override;

  virtual size_t Write(const void* pBuffer, size_t uiSize) override;

  virtual int Seek(int iOffset, Origin origin) override;

  virtual int Tell() const override;

  virtual size_t Size() const override;

private:
  plMemoryStreamWriter m_Writer;
};

namespace plOzzUtils
{
  PL_RENDERERCORE_DLL void CopyAnimation(ozz::animation::Animation* pDst, const ozz::animation::Animation* pSrc);
  PL_RENDERERCORE_DLL void CopySkeleton(ozz::animation::Skeleton* pDst, const ozz::animation::Skeleton* pSrc);
} // namespace plOzzUtils
