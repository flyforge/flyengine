#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/DependencyFile.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

enum class plDependencyFileVersion : plUInt8
{
  Version0 = 0,
  Version1,
  Version2, ///< added 'sum' time

  ENUM_COUNT,
  Current = ENUM_COUNT - 1,
};

plMap<plString, plDependencyFile::FileCheckCache> plDependencyFile::s_FileTimestamps;

plDependencyFile::plDependencyFile()
{
  Clear();
}

void plDependencyFile::Clear()
{
  m_iMaxTimeStampStored = 0;
  m_uiSumTimeStampStored = 0;
  m_AssetTransformDependencies.Clear();
}

void plDependencyFile::AddFileDependency(plStringView sFile)
{
  if (sFile.IsEmpty())
    return;

  m_AssetTransformDependencies.PushBack(sFile);
}

void plDependencyFile::StoreCurrentTimeStamp()
{
  PLASMA_LOG_BLOCK("plDependencyFile::StoreCurrentTimeStamp");

  m_iMaxTimeStampStored = 0;
  m_uiSumTimeStampStored = 0;

#if PLASMA_DISABLED(PLASMA_SUPPORTS_FILE_STATS)
  plLog::Warning("Trying to retrieve file time stamps on a platform that does not support it");
  return;
#endif

  for (const auto& sFile : m_AssetTransformDependencies)
  {
    plTimestamp ts;
    if (RetrieveFileTimeStamp(sFile, ts).Failed())
      continue;

    const plInt64 time = ts.GetInt64(plSIUnitOfTime::Second);
    m_iMaxTimeStampStored = plMath::Max<plInt64>(m_iMaxTimeStampStored, time);
    m_uiSumTimeStampStored += (plUInt64)time;
  }
}

bool plDependencyFile::HasAnyFileChanged() const
{
#if PLASMA_DISABLED(PLASMA_SUPPORTS_FILE_STATS)
  plLog::Warning("Trying to retrieve file time stamps on a platform that does not support it");
  return true;
#endif

  plUInt64 uiSumTs = 0;

  for (const auto& sFile : m_AssetTransformDependencies)
  {
    plTimestamp ts;
    if (RetrieveFileTimeStamp(sFile, ts).Failed())
      continue;

    const plInt64 time = ts.GetInt64(plSIUnitOfTime::Second);

    if (time > m_iMaxTimeStampStored)
    {
      plLog::Dev("Detected file change in '{0}' (TimeStamp {1} > MaxTimeStamp {2})", plArgSensitive(sFile, "File"),
        ts.GetInt64(plSIUnitOfTime::Second), m_iMaxTimeStampStored);
      return true;
    }

    uiSumTs += (plUInt64)time;
  }

  if (uiSumTs != m_uiSumTimeStampStored)
  {
    plLog::Dev("Detected file change, but exact file is not known.");
    return true;
  }

  return false;
}

plResult plDependencyFile::WriteDependencyFile(plStreamWriter& inout_stream) const
{
  inout_stream << (plUInt8)plDependencyFileVersion::Current;

  inout_stream << m_iMaxTimeStampStored;
  inout_stream << m_uiSumTimeStampStored;
  inout_stream << m_AssetTransformDependencies.GetCount();

  for (const auto& sFile : m_AssetTransformDependencies)
    inout_stream << sFile;

  return PLASMA_SUCCESS;
}

plResult plDependencyFile::ReadDependencyFile(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = (plUInt8)plDependencyFileVersion::Version0;
  inout_stream >> uiVersion;

  if (uiVersion > (plUInt8)plDependencyFileVersion::Current)
  {
    plLog::Error("Dependency file has incorrect file version ({0})", uiVersion);
    return PLASMA_FAILURE;
  }

  PLASMA_ASSERT_DEV(uiVersion <= (plUInt8)plDependencyFileVersion::Current, "Invalid file version {0}", uiVersion);

  inout_stream >> m_iMaxTimeStampStored;

  if (uiVersion >= (plUInt8)plDependencyFileVersion::Version2)
  {
    inout_stream >> m_uiSumTimeStampStored;
  }

  plUInt32 count = 0;
  inout_stream >> count;
  m_AssetTransformDependencies.SetCount(count);

  for (plUInt32 i = 0; i < m_AssetTransformDependencies.GetCount(); ++i)
    inout_stream >> m_AssetTransformDependencies[i];

  return PLASMA_SUCCESS;
}

plResult plDependencyFile::RetrieveFileTimeStamp(plStringView sFile, plTimestamp& out_Result)
{
#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_STATS)

  bool bExisted = false;
  auto it = s_FileTimestamps.FindOrAdd(sFile, &bExisted);

  if (!bExisted || it.Value().m_LastCheck + plTime::MakeFromSeconds(2.0) < plTime::Now())
  {
    it.Value().m_LastCheck = plTime::Now();

    plFileStats stats;
    if (plFileSystem::GetFileStats(sFile, stats).Failed())
    {
      plLog::Error("Could not query the file stats for '{0}'", plArgSensitive(sFile, "File"));
      return PLASMA_FAILURE;
    }

    it.Value().m_FileTimestamp = stats.m_LastModificationTime;
  }

  out_Result = it.Value().m_FileTimestamp;

#else

  out_Result = plTimestamp::MakeFromInt(0, plSIUnitOfTime::Second);
  plLog::Warning("Trying to retrieve a file time stamp on a platform that does not support it (file: '{0}')", plArgSensitive(sFile, "File"));

#endif

  return out_Result.IsValid() ? PLASMA_SUCCESS : PLASMA_FAILURE;
}

plResult plDependencyFile::WriteDependencyFile(plStringView sFile) const
{
  PLASMA_LOG_BLOCK("plDependencyFile::WriteDependencyFile", sFile);

  plFileWriter file;
  if (file.Open(sFile).Failed())
    return PLASMA_FAILURE;

  return WriteDependencyFile(file);
}

plResult plDependencyFile::ReadDependencyFile(plStringView sFile)
{
  PLASMA_LOG_BLOCK("plDependencyFile::ReadDependencyFile", sFile);

  plFileReader file;
  if (file.Open(sFile).Failed())
    return PLASMA_FAILURE;

  return ReadDependencyFile(file);
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_DependencyFile);
