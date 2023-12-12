#include <Foundation/FoundationPCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/TranslationLookup.h>

bool plTranslator::s_bHighlightUntranslated = false;
plHybridArray<plTranslator*, 4> plTranslator::s_AllTranslators;

plTranslator::plTranslator()
{
  s_AllTranslators.PushBack(this);
}

plTranslator::~plTranslator()
{
  s_AllTranslators.RemoveAndSwap(this);
}

void plTranslator::Reset() {}

void plTranslator::Reload() {}

void plTranslator::ReloadAllTranslators()
{
  PLASMA_LOG_BLOCK("ReloadAllTranslators");

  for (plTranslator* pTranslator : s_AllTranslators)
  {
    pTranslator->Reload();
  }
}

void plTranslator::HighlightUntranslated(bool bHighlight)
{
  if (s_bHighlightUntranslated == bHighlight)
    return;

  s_bHighlightUntranslated = bHighlight;

  ReloadAllTranslators();
}

//////////////////////////////////////////////////////////////////////////

plHybridArray<plUniquePtr<plTranslator>, 16> plTranslationLookup::s_Translators;

void plTranslationLookup::AddTranslator(plUniquePtr<plTranslator> pTranslator)
{
  s_Translators.PushBack(std::move(pTranslator));
}


const char* plTranslationLookup::Translate(const char* szString, plUInt64 uiStringHash, plTranslationUsage usage)
{
  for (plUInt32 i = s_Translators.GetCount(); i > 0; --i)
  {
    const char* szResult = s_Translators[i - 1]->Translate(szString, uiStringHash, usage);

    if (szResult != nullptr)
      return szResult;
  }

  return szString;
}


void plTranslationLookup::Clear()
{
  s_Translators.Clear();
}

//////////////////////////////////////////////////////////////////////////

void plTranslatorFromFiles::AddTranslationFilesFromFolder(const char* szFolder)
{
  PLASMA_LOG_BLOCK("AddTranslationFilesFromFolder", szFolder);

  if (!m_Folders.Contains(szFolder))
  {
    m_Folders.PushBack(szFolder);
  }

#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_ITERATORS)
  plStringBuilder startPath;
  if (plFileSystem::ResolvePath(szFolder, &startPath, nullptr).Failed())
    return;

  plStringBuilder fullpath;

  plFileSystemIterator it;
  it.StartSearch(startPath, plFileSystemIteratorFlags::ReportFilesRecursive);


  while (it.IsValid())
  {
    fullpath = it.GetCurrentPath();
    fullpath.AppendPath(it.GetStats().m_sName);

    LoadTranslationFile(fullpath);

    it.Next();
  }

#endif
}

const char* plTranslatorFromFiles::Translate(const char* szString, plUInt64 uiStringHash, plTranslationUsage usage)
{
  return plTranslatorStorage::Translate(szString, uiStringHash, usage);
}

void plTranslatorFromFiles::Reload()
{
  plTranslatorStorage::Reload();

  for (const auto& sFolder : m_Folders)
  {
    AddTranslationFilesFromFolder(sFolder);
  }
}

void plTranslatorFromFiles::LoadTranslationFile(const char* szFullPath)
{
  PLASMA_LOG_BLOCK("LoadTranslationFile", szFullPath);

  plLog::Dev("Loading Localization File '{0}'", szFullPath);

  plFileReader file;
  if (file.Open(szFullPath).Failed())
  {
    plLog::Warning("Failed to open localization file '{0}'", szFullPath);
    return;
  }

  plStringBuilder sContent;
  sContent.ReadAll(file);

  plDeque<plStringView> Lines;
  sContent.Split(false, Lines, "\n");

  plHybridArray<plStringView, 4> entries;

  plStringBuilder sLine, sKey, sValue, sTooltip, sHelpUrl;
  for (const auto& line : Lines)
  {
    sLine = line;
    sLine.Trim(" \t\r\n");

    if (sLine.IsEmpty() || sLine.StartsWith("#"))
      continue;

    entries.Clear();
    sLine.Split(true, entries, ";");

    if (entries.GetCount() <= 1)
    {
      plLog::Error("Invalid line in translation file: '{0}'", sLine);
      continue;
    }

    sKey = entries[0];
    sValue = entries[1];

    sTooltip.Clear();
    sHelpUrl.Clear();

    if (entries.GetCount() >= 3)
      sTooltip = entries[2];
    if (entries.GetCount() >= 4)
      sHelpUrl = entries[3];

    sKey.Trim(" \t\r\n");
    sValue.Trim(" \t\r\n");
    sTooltip.Trim(" \t\r\n");
    sHelpUrl.Trim(" \t\r\n");

    if (GetHighlightUntranslated())
    {
      sValue.Prepend("# ");
      sValue.Append(" (@", sKey, ")");
    }

    StoreTranslation(sValue, plHashingUtils::StringHash(sKey), plTranslationUsage::Default);
    StoreTranslation(sTooltip, plHashingUtils::StringHash(sKey), plTranslationUsage::Tooltip);
    StoreTranslation(sHelpUrl, plHashingUtils::StringHash(sKey), plTranslationUsage::HelpURL);
  }
}

//////////////////////////////////////////////////////////////////////////

void plTranslatorStorage::StoreTranslation(const char* szString, plUInt64 uiStringHash, plTranslationUsage usage)
{
  m_Translations[(plUInt32)usage][uiStringHash] = szString;
}

const char* plTranslatorStorage::Translate(const char* szString, plUInt64 uiStringHash, plTranslationUsage usage)
{
  auto it = m_Translations[(plUInt32)usage].Find(uiStringHash);
  if (it.IsValid())
    return it.Value().GetData();

  return nullptr;
}

void plTranslatorStorage::Reset()
{
  for (plUInt32 i = 0; i < (plUInt32)plTranslationUsage::ENUM_COUNT; ++i)
  {
    m_Translations[i].Clear();
  }
}

void plTranslatorStorage::Reload()
{
  Reset();
}

//////////////////////////////////////////////////////////////////////////

bool plTranslatorLogMissing::s_bActive = true;

const char* plTranslatorLogMissing::Translate(const char* szString, plUInt64 uiStringHash, plTranslationUsage usage)
{
  if (!plTranslatorLogMissing::s_bActive && !GetHighlightUntranslated())
    return nullptr;

  if (usage != plTranslationUsage::Default)
    return nullptr;

  const char* szResult = plTranslatorStorage::Translate(szString, uiStringHash, usage);

  if (szResult == nullptr)
  {
    plLog::Warning("Missing translation: {0};", szString);

    StoreTranslation(szString, uiStringHash, usage);
  }

  return nullptr;
}

const char* plTranslatorMakeMoreReadable::Translate(const char* szString, plUInt64 uiStringHash, plTranslationUsage usage)
{
  const char* szResult = plTranslatorStorage::Translate(szString, uiStringHash, usage);

  if (szResult != nullptr)
    return szResult;


  plStringBuilder result;
  plStringBuilder tmp = szString;
  tmp.Trim(" _-");
  tmp.TrimWordStart("pl");
  tmp.TrimWordEnd("Component");

  auto IsUpper = [](plUInt32 c) { return c == plStringUtils::ToUpperChar(c); };
  auto IsNumber = [](plUInt32 c) { return c >= '0' && c <= '9'; };

  plUInt32 uiPrev = ' ';
  plUInt32 uiCur = ' ';
  plUInt32 uiNext = ' ';

  bool bContinue = true;

  for (auto it = tmp.GetIteratorFront(); bContinue; ++it)
  {
    uiPrev = uiCur;
    uiCur = uiNext;

    if (it.IsValid())
    {
      uiNext = it.GetCharacter();
    }
    else
    {
      uiNext = ' ';
      bContinue = false;
    }

    if (uiCur == '_')
      uiCur = ' ';

    if (uiCur == ':')
    {
      result.Clear();
      continue;
    }

    if (IsNumber(uiPrev) != IsNumber(uiCur))
    {
      result.Append(" ");
      result.Append(uiCur);
      continue;
    }

    if (IsNumber(uiPrev) && IsNumber(uiCur))
    {
      result.Append(uiCur);
      continue;
    }

    if (IsUpper(uiPrev) && IsUpper(uiCur) && !IsUpper(uiNext))
    {
      result.Append(" ");
      result.Append(uiCur);
      continue;
    }

    if (!IsUpper(uiCur) && IsUpper(uiNext))
    {
      result.Append(uiCur);
      result.Append(" ");
      continue;
    }

    result.Append(uiCur);
  }

  result.Trim(" ");
  while (result.ReplaceAll("  ", " ") > 0)
  {
    // remove double whitespaces
  }

  if (GetHighlightUntranslated())
  {
    result.Append(" (@", szString, ")");
  }

  StoreTranslation(result, uiStringHash, usage);

  return plTranslatorStorage::Translate(szString, uiStringHash, usage);
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_TranslationLookup);
