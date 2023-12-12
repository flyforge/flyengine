#pragma once

#include <Foundation/IO/OSFile.h>

class StreamComparer : public plStreamWriter
{
public:
  StreamComparer(const char* szExpectedData, bool bOnlyWriteResult = false)
  {
    m_bOnlyWriteResult = bOnlyWriteResult;
    m_szExpectedData = szExpectedData;
  }

  ~StreamComparer()
  {
    if (m_bOnlyWriteResult)
    {
      plOSFile f;
      f.Open("C:\\Code\\JSON.txt", plFileOpenMode::Write).IgnoreResult();
      f.Write(m_sResult.GetData(), m_sResult.GetElementCount()).IgnoreResult();
      f.Close();
    }
    else
      PLASMA_TEST_BOOL(*m_szExpectedData == '\0');
  }

  plResult WriteBytes(const void* pWriteBuffer, plUInt64 uiBytesToWrite)
  {
    if (m_bOnlyWriteResult)
      m_sResult.Append((const char*)pWriteBuffer);
    else
    {
      const char* szWritten = (const char*)pWriteBuffer;

      PLASMA_TEST_BOOL(plMemoryUtils::IsEqual(szWritten, m_szExpectedData, (plUInt32)uiBytesToWrite));
      m_szExpectedData += uiBytesToWrite;
    }

    return PLASMA_SUCCESS;
  }

private:
  bool m_bOnlyWriteResult;
  plStringBuilder m_sResult;
  const char* m_szExpectedData;
};


class StringStream : public plStreamReader
{
public:
  StringStream(const void* pData)
  {
    m_pData = pData;
    m_uiLength = plStringUtils::GetStringElementCount((const char*)pData);
  }

  virtual plUInt64 ReadBytes(void* pReadBuffer, plUInt64 uiBytesToRead)
  {
    uiBytesToRead = plMath::Min(uiBytesToRead, m_uiLength);
    m_uiLength -= uiBytesToRead;

    if (uiBytesToRead > 0)
    {
      plMemoryUtils::Copy((plUInt8*)pReadBuffer, (plUInt8*)m_pData, (size_t)uiBytesToRead);
      m_pData = plMemoryUtils::AddByteOffset(m_pData, (ptrdiff_t)uiBytesToRead);
    }

    return uiBytesToRead;
  }

private:
  const void* m_pData;
  plUInt64 m_uiLength;
};
