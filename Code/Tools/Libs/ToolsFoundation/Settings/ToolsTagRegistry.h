#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

struct PLASMA_TOOLSFOUNDATION_DLL plToolsTag
{
  plToolsTag() {}
  plToolsTag(const char* szCategory, const char* szName, bool bBuiltIn = false)
    : m_sCategory(szCategory)
    , m_sName(szName)
    , m_bBuiltInTag(bBuiltIn)
  {
  }

  plString m_sCategory;
  plString m_sName;
  bool m_bBuiltInTag = false; ///< If set to true, this is a tag created by code that the user is not allowed to remove
};

class PLASMA_TOOLSFOUNDATION_DLL plToolsTagRegistry
{
public:
  /// \brief Removes all tags that are not specified as 'built-in'
  static void Clear();

  static void WriteToDDL(plStreamWriter& stream);
  static plStatus ReadFromDDL(plStreamReader& stream);

  static bool AddTag(const plToolsTag& tag);
  static bool RemoveTag(const char* szName);

  static void GetAllTags(plHybridArray<const plToolsTag*, 16>& out_tags);
  static void GetTagsByCategory(const plArrayPtr<plStringView>& categories, plHybridArray<const plToolsTag*, 16>& out_tags);

private:
  static plMap<plString, plToolsTag> s_NameToTags;
};
