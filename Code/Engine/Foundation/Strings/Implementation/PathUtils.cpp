#include <Foundation/FoundationPCH.h>

#include <Foundation/Strings/Implementation/StringIterator.h>
#include <Foundation/Strings/StringBuilder.h>

const char* plPathUtils::FindPreviousSeparator(const char* szPathStart, const char* szStartSearchAt)
{
  if (plStringUtils::IsNullOrEmpty(szPathStart))
    return nullptr;

  while (szStartSearchAt > szPathStart)
  {
    plUnicodeUtils::MoveToPriorUtf8(szStartSearchAt, szPathStart).AssertSuccess();

    if (IsPathSeparator(*szStartSearchAt))
      return szStartSearchAt;
  }

  return nullptr;
}

bool plPathUtils::HasAnyExtension(plStringView sPath)
{
  const char* szDot = plStringUtils::FindLastSubString(sPath.GetStartPointer(), ".", nullptr, sPath.GetEndPointer());

  if (szDot == nullptr)
    return false;

  // find the last separator in the string
  const char* szSeparator = FindPreviousSeparator(sPath.GetStartPointer(), sPath.GetEndPointer());

  return (szSeparator < szDot);
}

bool plPathUtils::HasExtension(plStringView sPath, plStringView sExtension)
{
  if (plStringUtils::StartsWith(sExtension.GetStartPointer(), ".", sExtension.GetEndPointer()))
    return plStringUtils::EndsWith_NoCase(sPath.GetStartPointer(), sExtension.GetStartPointer(), sPath.GetEndPointer(), sExtension.GetEndPointer());

  plStringBuilder sExt;
  sExt.Append(".", sExtension);

  return plStringUtils::EndsWith_NoCase(sPath.GetStartPointer(), sExt.GetData(), sPath.GetEndPointer());
}

plStringView plPathUtils::GetFileExtension(plStringView sPath)
{
  const char* szDot = plStringUtils::FindLastSubString(sPath.GetStartPointer(), ".", nullptr, sPath.GetEndPointer());

  if (szDot == nullptr)
    return plStringView(nullptr);

  // find the last separator in the string
  const char* szSeparator = FindPreviousSeparator(sPath.GetStartPointer(), sPath.GetEndPointer());

  if (szSeparator > szDot)
    return plStringView(nullptr);

  return plStringView(szDot + 1, sPath.GetEndPointer());
}

plStringView plPathUtils::GetFileNameAndExtension(plStringView sPath)
{
  const char* szSeparator = FindPreviousSeparator(sPath.GetStartPointer(), sPath.GetEndPointer());

  if (szSeparator == nullptr)
    return sPath;

  return plStringView(szSeparator + 1, sPath.GetEndPointer());
}

plStringView plPathUtils::GetFileName(plStringView sPath)
{
  const char* szSeparator = FindPreviousSeparator(sPath.GetStartPointer(), sPath.GetEndPointer());

  const char* szDot = plStringUtils::FindLastSubString(sPath.GetStartPointer(), ".", sPath.GetEndPointer());

  if (szDot < szSeparator) // includes (szDot == nullptr), szSeparator will never be nullptr here -> no extension
  {
    return plStringView(szSeparator + 1, sPath.GetEndPointer());
  }

  if (szSeparator == nullptr)
  {
    if (szDot == nullptr) // no folder, no extension -> the entire thing is just a name
      return sPath;

    return plStringView(sPath.GetStartPointer(), szDot); // no folder, but an extension -> remove the extension
  }

  // now: there is a separator AND an extension

  return plStringView(szSeparator + 1, szDot);
}

plStringView plPathUtils::GetFileDirectory(plStringView sPath)
{
  auto it = rbegin(sPath);

  // if it already ends in a path separator, do not return a different directory
  if (IsPathSeparator(it.GetCharacter()))
    return sPath;

  // find the last separator in the string
  const char* szSeparator = FindPreviousSeparator(sPath.GetStartPointer(), sPath.GetEndPointer());

  // no path separator -> root dir -> return the empty path
  if (szSeparator == nullptr)
    return plStringView(nullptr);

  return plStringView(sPath.GetStartPointer(), szSeparator + 1);
}

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
const char plPathUtils::OsSpecificPathSeparator = '\\';
#elif PL_ENABLED(PL_PLATFORM_LINUX) || PL_ENABLED(PL_PLATFORM_ANDROID)
const char plPathUtils::OsSpecificPathSeparator = '/';
#elif PL_ENABLED(PL_PLATFORM_OSX)
const char plPathUtils::OsSpecificPathSeparator = '/';
#else
#  error "Unknown platform."
#endif

bool plPathUtils::IsAbsolutePath(plStringView sPath)
{
  if (sPath.GetElementCount() < 2)
    return false;

  const char* szPath = sPath.GetStartPointer();

  // szPath[0] will not be \0 -> so we can access szPath[1] without problems

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
  /// if it is an absolute path, character 0 must be ASCII (A - Z)
  /// checks for local paths, i.e. 'C:\stuff' and UNC paths, i.e. '\\server\stuff'
  /// not sure if we should handle '//' identical to '\\' (currently we do)
  return ((szPath[1] == ':') || (IsPathSeparator(szPath[0]) && IsPathSeparator(szPath[1])));
#elif PL_ENABLED(PL_PLATFORM_LINUX) || PL_ENABLED(PL_PLATFORM_ANDROID)
  return (szPath[0] == '/');
#elif PL_ENABLED(PL_PLATFORM_OSX)
  return (szPath[0] == '/');
#else
#  error "Unknown platform."
#endif
}

bool plPathUtils::IsRelativePath(plStringView sPath)
{
  if (sPath.IsEmpty())
    return true;

  // if it starts with a separator, it is not a relative path, ever
  if (plPathUtils::IsPathSeparator(*sPath.GetStartPointer()))
    return false;

  return !IsAbsolutePath(sPath) && !IsRootedPath(sPath);
}

bool plPathUtils::IsRootedPath(plStringView sPath)
{
  return !sPath.IsEmpty() && *sPath.GetStartPointer() == ':';
}

void plPathUtils::GetRootedPathParts(plStringView sPath, plStringView& ref_sRoot, plStringView& ref_sRelPath)
{
  ref_sRoot = plStringView();
  ref_sRelPath = sPath;

  if (!IsRootedPath(sPath))
    return;

  const char* szStart = sPath.GetStartPointer();
  const char* szPathEnd = sPath.GetEndPointer();

  do
  {
    plUnicodeUtils::MoveToNextUtf8(szStart, szPathEnd).AssertSuccess();

    if (*szStart == '\0')
      return;

  } while (IsPathSeparator(*szStart));

  const char* szEnd = szStart;
  plUnicodeUtils::MoveToNextUtf8(szEnd, szPathEnd).AssertSuccess();

  while (*szEnd != '\0' && !IsPathSeparator(*szEnd))
    plUnicodeUtils::MoveToNextUtf8(szEnd, szPathEnd).AssertSuccess();

  ref_sRoot = plStringView(szStart, szEnd);
  if (*szEnd == '\0')
  {
    ref_sRelPath = plStringView();
  }
  else
  {
    // skip path separator for the relative path
    plUnicodeUtils::MoveToNextUtf8(szEnd, szPathEnd).AssertSuccess();
    ref_sRelPath = plStringView(szEnd, szPathEnd);
  }
}

plStringView plPathUtils::GetRootedPathRootName(plStringView sPath)
{
  plStringView root, relPath;
  GetRootedPathParts(sPath, root, relPath);
  return root;
}

bool plPathUtils::IsValidFilenameChar(plUInt32 uiCharacter)
{
  /// \test Not tested yet

  // Windows: https://msdn.microsoft.com/library/windows/desktop/aa365247(v=vs.85).aspx
  // Unix: https://en.wikipedia.org/wiki/Filename#Reserved_characters_and_words
  // Details can be more complicated (there might be reserved names depending on the filesystem), but in general all platforms behave like
  // this:
  static const plUInt32 forbiddenFilenameChars[] = {'<', '>', ':', '"', '|', '?', '*', '\\', '/', '\t', '\b', '\n', '\r', '\0'};

  for (int i = 0; i < PL_ARRAY_SIZE(forbiddenFilenameChars); ++i)
  {
    if (forbiddenFilenameChars[i] == uiCharacter)
      return false;
  }

  return true;
}

bool plPathUtils::ContainsInvalidFilenameChars(plStringView sPath)
{
  /// \test Not tested yet

  plStringIterator it = sPath.GetIteratorFront();

  for (; it.IsValid(); ++it)
  {
    if (!IsValidFilenameChar(it.GetCharacter()))
      return true;
  }

  return false;
}

void plPathUtils::MakeValidFilename(plStringView sFilename, plUInt32 uiReplacementCharacter, plStringBuilder& out_sFilename)
{
  PL_ASSERT_DEBUG(IsValidFilenameChar(uiReplacementCharacter), "Given replacement character is not allowed for filenames.");

  out_sFilename.Clear();

  for (auto it = sFilename.GetIteratorFront(); it.IsValid(); ++it)
  {
    plUInt32 currentChar = it.GetCharacter();

    if (IsValidFilenameChar(currentChar) == false)
      out_sFilename.Append(uiReplacementCharacter);
    else
      out_sFilename.Append(currentChar);
  }
}

bool plPathUtils::IsSubPath(plStringView sPrefixPath, plStringView sFullPath)
{
  /// \test this is new

  plStringBuilder tmp = sPrefixPath;
  tmp.MakeCleanPath();
  tmp.AppendPath("");

  if (sFullPath.StartsWith(tmp))
  {
    if (tmp.GetElementCount() == sFullPath.GetElementCount())
      return true;

    return sFullPath.GetStartPointer()[tmp.GetElementCount()] == '/';
  }

  return false;
}

bool plPathUtils::IsSubPath_NoCase(plStringView sPrefixPath, plStringView sFullPath)
{
  plStringBuilder tmp = sPrefixPath;
  tmp.MakeCleanPath();
  tmp.AppendPath("");

  if (sFullPath.StartsWith_NoCase(tmp))
  {
    if (tmp.GetElementCount() == sFullPath.GetElementCount())
      return true;

    return sFullPath.GetStartPointer()[tmp.GetElementCount()] == '/';
  }

  return false;
}


