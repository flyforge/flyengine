#include <Core/CorePCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Containers/Blob.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>

struct FileResourceLoadData
{
  plBlob m_Storage;
  plRawMemoryStreamReader m_Reader;
};

plResourceLoadData plResourceLoaderFromFile::OpenDataStream(const plResource* pResource)
{
  PL_PROFILE_SCOPE("ReadResourceFile");

  plResourceLoadData res;

  plFileReader File;
  if (File.Open(pResource->GetResourceID().GetData()).Failed())
    return res;

  res.m_sResourceDescription = File.GetFilePathRelative().GetData();

#if PL_ENABLED(PL_SUPPORTS_FILE_STATS)
  plFileStats stat;
  if (plFileSystem::GetFileStats(pResource->GetResourceID(), stat).Succeeded())
  {
    res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
  }

#endif

  FileResourceLoadData* pData = PL_DEFAULT_NEW(FileResourceLoadData);

  const plUInt64 uiFileSize = File.GetFileSize();

  const plUInt64 uiBlobCapacity = uiFileSize + File.GetFilePathAbsolute().GetElementCount() + 8; // +8 for the string overhead
  pData->m_Storage.SetCountUninitialized(uiBlobCapacity);

  plUInt8* pBlobPtr = pData->m_Storage.GetBlobPtr<plUInt8>().GetPtr();

  plRawMemoryStreamWriter w(pBlobPtr, uiBlobCapacity);

  // write the absolute path to the read file into the memory stream
  w << File.GetFilePathAbsolute();

  const plUInt64 uiOffset = w.GetNumWrittenBytes();

  File.ReadBytes(pBlobPtr + uiOffset, uiFileSize);

  pData->m_Reader.Reset(pBlobPtr, w.GetNumWrittenBytes() + uiFileSize);
  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

  return res;
}

void plResourceLoaderFromFile::CloseDataStream(const plResource* pResource, const plResourceLoadData& loaderData)
{
  FileResourceLoadData* pData = static_cast<FileResourceLoadData*>(loaderData.m_pCustomLoaderData);

  PL_DEFAULT_DELETE(pData);
}

bool plResourceLoaderFromFile::IsResourceOutdated(const plResource* pResource) const
{
  // if we cannot find the target file, there is no point in trying to reload it -> claim it's up to date
  if (plFileSystem::ResolvePath(pResource->GetResourceID(), nullptr, nullptr).Failed())
    return false;

#if PL_ENABLED(PL_SUPPORTS_FILE_STATS)

  if (pResource->GetLoadedFileModificationTime().IsValid())
  {
    plFileStats stat;
    if (plFileSystem::GetFileStats(pResource->GetResourceID(), stat).Failed())
      return false;

    return !stat.m_LastModificationTime.Compare(pResource->GetLoadedFileModificationTime(), plTimestamp::CompareMode::FileTimeEqual);
  }

#endif

  return true;
}

//////////////////////////////////////////////////////////////////////////

plResourceLoadData plResourceLoaderFromMemory::OpenDataStream(const plResource* pResource)
{
  m_Reader.SetStorage(&m_CustomData);
  m_Reader.SetReadPosition(0);

  plResourceLoadData res;

  res.m_sResourceDescription = m_sResourceDescription;
  res.m_LoadedFileModificationDate = m_ModificationTimestamp;
  res.m_pDataStream = &m_Reader;
  res.m_pCustomLoaderData = nullptr;

  return res;
}

void plResourceLoaderFromMemory::CloseDataStream(const plResource* pResource, const plResourceLoadData& loaderData)
{
  m_Reader.SetStorage(nullptr);
}

bool plResourceLoaderFromMemory::IsResourceOutdated(const plResource* pResource) const
{
  if (pResource->GetLoadedFileModificationTime().IsValid() && m_ModificationTimestamp.IsValid())
  {
    if (!m_ModificationTimestamp.Compare(pResource->GetLoadedFileModificationTime(), plTimestamp::CompareMode::FileTimeEqual))
      return true;

    return false;
  }

  return true;
}


