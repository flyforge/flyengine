#pragma once

#include <Foundation/Strings/StringBuilder.h>

inline plDataDirectoryReaderWriterBase::plDataDirectoryReaderWriterBase(plInt32 iDataDirUserData, bool bIsReader)
{
  m_iDataDirUserData = iDataDirUserData;
  m_pDataDirectory = nullptr;
  m_bIsReader = bIsReader;
}

inline plResult plDataDirectoryReaderWriterBase::Open(plStringView sFile, plDataDirectoryType* pDataDirectory, plFileShareMode::Enum fileShareMode)
{
  m_pDataDirectory = pDataDirectory;
  m_sFilePath = sFile;

  return InternalOpen(fileShareMode);
}

inline const plString128& plDataDirectoryReaderWriterBase::GetFilePath() const
{
  return m_sFilePath;
}

inline plDataDirectoryType* plDataDirectoryReaderWriterBase::GetDataDirectory() const
{
  return m_pDataDirectory;
}
