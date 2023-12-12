#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

plMap<plString, plSet<plString>> plAssetFileExtensionWhitelist::s_ExtensionWhitelist;

void plAssetFileExtensionWhitelist::AddAssetFileExtension(const char* szAssetType, const char* szAllowedFileExtension)
{
  plStringBuilder sLowerType = szAssetType;
  sLowerType.ToLower();

  plStringBuilder sLowerExt = szAllowedFileExtension;
  sLowerExt.ToLower();

  s_ExtensionWhitelist[sLowerType].Insert(sLowerExt);
}


bool plAssetFileExtensionWhitelist::IsFileOnAssetWhitelist(const char* szAssetType, const char* szFile)
{
  plStringBuilder sLowerExt = plPathUtils::GetFileExtension(szFile);
  sLowerExt.ToLower();

  plStringBuilder sLowerType = szAssetType;
  sLowerType.ToLower();

  plHybridArray<plString, 16> Types;
  sLowerType.Split(false, Types, ";");

  for (const auto& filter : Types)
  {
    if (s_ExtensionWhitelist[filter].Contains(sLowerExt))
      return true;
  }

  return false;
}

const plSet<plString>& plAssetFileExtensionWhitelist::GetAssetFileExtensions(const char* szAssetType)
{
  return s_ExtensionWhitelist[szAssetType];
}
