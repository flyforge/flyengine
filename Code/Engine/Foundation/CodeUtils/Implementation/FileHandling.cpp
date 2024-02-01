#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/ConversionUtils.h>

using namespace plTokenParseUtils;

plMap<plString, plTokenizedFileCache::FileData>::ConstIterator plTokenizedFileCache::Lookup(const plString& sFileName) const
{
  PL_LOCK(m_Mutex);
  auto it = m_Cache.Find(sFileName);
  return it;
}

void plTokenizedFileCache::Remove(const plString& sFileName)
{
  PL_LOCK(m_Mutex);
  m_Cache.Remove(sFileName);
}

void plTokenizedFileCache::Clear()
{
  PL_LOCK(m_Mutex);
  m_Cache.Clear();
}

void plTokenizedFileCache::SkipWhitespace(plDeque<plToken>& Tokens, plUInt32& uiCurToken)
{
  while (uiCurToken < Tokens.GetCount() && (Tokens[uiCurToken].m_iType == plTokenType::BlockComment || Tokens[uiCurToken].m_iType == plTokenType::LineComment || Tokens[uiCurToken].m_iType == plTokenType::Newline || Tokens[uiCurToken].m_iType == plTokenType::Whitespace))
    ++uiCurToken;
}

const plTokenizer* plTokenizedFileCache::Tokenize(const plString& sFileName, plArrayPtr<const plUInt8> fileContent, const plTimestamp& fileTimeStamp, plLogInterface* pLog)
{
  PL_LOCK(m_Mutex);

  bool bExisted = false;
  auto it = m_Cache.FindOrAdd(sFileName, &bExisted);
  if (bExisted)
  {
    return &it.Value().m_Tokens;
  }

  auto& data = it.Value();

  data.m_Timestamp = fileTimeStamp;
  plTokenizer* pTokenizer = &data.m_Tokens;
  pTokenizer->Tokenize(fileContent, pLog);

  plDeque<plToken>& Tokens = pTokenizer->GetTokens();

  plHashedString sFile;
  sFile.Assign(sFileName);

  plInt32 iLineOffset = 0;

  for (plUInt32 i = 0; i + 1 < Tokens.GetCount(); ++i)
  {
    const plUInt32 uiCurLine = Tokens[i].m_uiLine;

    Tokens[i].m_File = sFile;
    Tokens[i].m_uiLine += iLineOffset;

    if (Tokens[i].m_iType == plTokenType::NonIdentifier && Tokens[i].m_DataView.IsEqual("#"))
    {
      plUInt32 uiNext = i + 1;

      SkipWhitespace(Tokens, uiNext);

      if (uiNext < Tokens.GetCount() && Tokens[uiNext].m_iType == plTokenType::Identifier && Tokens[uiNext].m_DataView.IsEqual("line"))
      {
        ++uiNext;
        SkipWhitespace(Tokens, uiNext);

        if (uiNext < Tokens.GetCount() && Tokens[uiNext].m_iType == plTokenType::Integer)
        {
          plInt32 iNextLine = 0;

          const plString sNumber = Tokens[uiNext].m_DataView;
          if (plConversionUtils::StringToInt(sNumber, iNextLine).Succeeded())
          {
            iLineOffset = (iNextLine - uiCurLine) - 1;

            ++uiNext;
            SkipWhitespace(Tokens, uiNext);

            if (uiNext < Tokens.GetCount())
            {
              if (Tokens[uiNext].m_iType == plTokenType::String1)
              {
                plStringBuilder sFileName2 = Tokens[uiNext].m_DataView;
                sFileName2.Shrink(1, 1); // remove surrounding "

                sFile.Assign(sFileName2);
              }
            }
          }
        }
      }
    }
  }

  return pTokenizer;
}


void plPreprocessor::SetLogInterface(plLogInterface* pLog)
{
  m_pLog = pLog;
}

void plPreprocessor::SetFileOpenFunction(FileOpenCB openAbsFileCB)
{
  m_FileOpenCallback = openAbsFileCB;
}

void plPreprocessor::SetFileLocatorFunction(FileLocatorCB locateAbsFileCB)
{
  m_FileLocatorCallback = locateAbsFileCB;
}

plResult plPreprocessor::DefaultFileLocator(plStringView sCurAbsoluteFile, plStringView sIncludeFile, plPreprocessor::IncludeType incType, plStringBuilder& out_sAbsoluteFilePath)
{
  plStringBuilder& s = out_sAbsoluteFilePath;

  if (incType == plPreprocessor::RelativeInclude)
  {
    s = sCurAbsoluteFile;
    s.PathParentDirectory();
    s.AppendPath(sIncludeFile);
    s.MakeCleanPath();
  }
  else
  {
    s = sIncludeFile;
    s.MakeCleanPath();
  }

  return PL_SUCCESS;
}

plResult plPreprocessor::DefaultFileOpen(plStringView sAbsoluteFile, plDynamicArray<plUInt8>& ref_fileContent, plTimestamp& out_fileModification)
{
  plFileReader r;
  if (r.Open(sAbsoluteFile).Failed())
    return PL_FAILURE;

#if PL_ENABLED(PL_SUPPORTS_FILE_STATS)
  plFileStats stats;
  if (plFileSystem::GetFileStats(sAbsoluteFile, stats).Succeeded())
    out_fileModification = stats.m_LastModificationTime;
#endif

  plUInt8 Temp[4096];

  while (plUInt64 uiRead = r.ReadBytes(Temp, 4096))
  {
    ref_fileContent.PushBackRange(plArrayPtr<plUInt8>(Temp, (plUInt32)uiRead));
  }

  return PL_SUCCESS;
}

plResult plPreprocessor::OpenFile(plStringView sFile, const plTokenizer** pTokenizer)
{
  PL_ASSERT_DEV(m_FileOpenCallback.IsValid(), "OpenFile callback has not been set");
  PL_ASSERT_DEV(m_FileLocatorCallback.IsValid(), "File locator callback has not been set");

  *pTokenizer = nullptr;

  auto it = m_pUsedFileCache->Lookup(sFile);

  if (it.IsValid())
  {
    *pTokenizer = &it.Value().m_Tokens;
    return PL_SUCCESS;
  }

  plTimestamp stamp;

  plDynamicArray<plUInt8> Content;
  if (m_FileOpenCallback(sFile, Content, stamp).Failed())
  {
    plLog::Error(m_pLog, "Could not open file '{0}'", sFile);
    return PL_FAILURE;
  }

  plArrayPtr<const plUInt8> ContentView = Content;

  // the file open callback gives us raw data for the opened file
  // the tokenizer doesn't like the Utf8 BOM, so skip it here, if we detect it
  if (ContentView.GetCount() >= 3) // length of a BOM
  {
    const char* dataStart = reinterpret_cast<const char*>(ContentView.GetPtr());

    if (plUnicodeUtils::SkipUtf8Bom(dataStart))
    {
      ContentView = plArrayPtr<const plUInt8>((const plUInt8*)dataStart, Content.GetCount() - 3);
    }
  }

  *pTokenizer = m_pUsedFileCache->Tokenize(sFile, ContentView, stamp, m_pLog);

  return PL_SUCCESS;
}


plResult plPreprocessor::HandleInclude(const TokenStream& Tokens0, plUInt32 uiCurToken, plUInt32 uiDirectiveToken, TokenStream& TokenOutput)
{
  PL_ASSERT_DEV(m_FileLocatorCallback.IsValid(), "File locator callback has not been set");

  TokenStream Tokens;
  if (Expand(Tokens0, Tokens).Failed())
    return PL_FAILURE;

  SkipWhitespace(Tokens, uiCurToken);

  plStringBuilder sPath;

  IncludeType IncType = IncludeType::GlobalInclude;


  plUInt32 uiAccepted;
  if (Accept(Tokens, uiCurToken, plTokenType::String1, &uiAccepted))
  {
    IncType = IncludeType::RelativeInclude;
    sPath = Tokens[uiAccepted]->m_DataView;
    sPath.Shrink(1, 1); // remove " at start and end
  }
  else
  {
    // in global include paths (ie. <bla/blub.h>) we need to handle line comments special
    // because a path with two slashes will be a comment token, although it could be a valid path
    // so we concatenate just everything and then make sure it ends with a >

    if (Expect(Tokens, uiCurToken, "<", &uiAccepted).Failed())
      return PL_FAILURE;

    TokenStream PathTokens;

    while (uiCurToken < Tokens.GetCount())
    {
      if (Tokens[uiCurToken]->m_iType == plTokenType::Newline)
      {
        break;
      }

      PathTokens.PushBack(Tokens[uiCurToken]);
      ++uiCurToken;
    }

    CombineTokensToString(PathTokens, 0, sPath, false);

    // remove all whitespace at the end (this could be part of a comment, so not tokenized as whitespace)
    while (sPath.EndsWith(" ") || sPath.EndsWith("\t"))
      sPath.Shrink(0, 1);

    // there must always be a > at the end, although it could be a separate token or part of a comment
    // so we check the string, instead of the tokens
    if (sPath.EndsWith(">"))
      sPath.Shrink(0, 1);
    else
    {
      PP_LOG(Error, "Invalid include path '{0}'", Tokens[uiAccepted], sPath);
      return PL_FAILURE;
    }
  }

  if (ExpectEndOfLine(Tokens, uiCurToken).Failed())
  {
    PP_LOG0(Error, "Expected end-of-line", Tokens[uiCurToken]);
    return PL_FAILURE;
  }

  PL_ASSERT_DEV(!m_CurrentFileStack.IsEmpty(), "Implementation error.");

  plStringBuilder sOtherFile;

  if (m_FileLocatorCallback(m_CurrentFileStack.PeekBack().m_sFileName.GetData(), sPath, IncType, sOtherFile).Failed())
  {
    PP_LOG(Error, "#include file '{0}' could not be located", Tokens[uiAccepted], sPath);
    return PL_FAILURE;
  }

  const plTempHashedString sOtherFileHashed(sOtherFile);

  // if this has been included before, and contains a #pragma once, do not include it again
  if (m_PragmaOnce.Find(sOtherFileHashed).IsValid())
    return PL_SUCCESS;

  if (ProcessFile(sOtherFile, TokenOutput).Failed())
    return PL_FAILURE;

  if (uiCurToken < Tokens.GetCount() && (Tokens[uiCurToken]->m_iType == plTokenType::Newline || Tokens[uiCurToken]->m_iType == plTokenType::EndOfFile))
    TokenOutput.PushBack(Tokens[uiCurToken]);

  return PL_SUCCESS;
}


