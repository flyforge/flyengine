#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

plMap<plString, plSet<plString>> plAssetFileExtensionWhitelist::s_ExtensionWhitelist;

void plAssetFileExtensionWhitelist::AddAssetFileExtension(plStringView sAssetType, plStringView sAllowedFileExtension)
{
  plStringBuilder sLowerType = sAssetType;
  sLowerType.ToLower();

  plStringBuilder sLowerExt = sAllowedFileExtension;
  sLowerExt.ToLower();

  s_ExtensionWhitelist[sLowerType].Insert(sLowerExt);
}


bool plAssetFileExtensionWhitelist::IsFileOnAssetWhitelist(plStringView sAssetType, plStringView sFile)
{
  plStringBuilder sLowerExt = plPathUtils::GetFileExtension(sFile);
  sLowerExt.ToLower();

  plStringBuilder sLowerType = sAssetType;
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

const plSet<plString>& plAssetFileExtensionWhitelist::GetAssetFileExtensions(plStringView sAssetType)
{
  return s_ExtensionWhitelist[sAssetType];
}
