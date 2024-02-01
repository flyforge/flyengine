#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// \brief A global whitelist for file extension that may be used as certain asset types
///
/// UI elements etc. may use this whitelist to detect whether a selected file is a valid candidate for an asset slot
class PL_TOOLSFOUNDATION_DLL plAssetFileExtensionWhitelist
{
public:
  static void AddAssetFileExtension(plStringView sAssetType, plStringView sAllowedFileExtension);

  static bool IsFileOnAssetWhitelist(plStringView sAssetType, plStringView sFile);

  static const plSet<plString>& GetAssetFileExtensions(plStringView sAssetType);

private:
  static plMap<plString, plSet<plString>> s_ExtensionWhitelist;
};
