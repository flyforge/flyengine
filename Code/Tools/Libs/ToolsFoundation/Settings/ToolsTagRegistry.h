#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

struct PL_TOOLSFOUNDATION_DLL plToolsTag
{
  plToolsTag() = default;
  plToolsTag(plStringView sCategory, plStringView sName, bool bBuiltIn = false)
    : m_sCategory(sCategory)
    , m_sName(sName)
    , m_bBuiltInTag(bBuiltIn)
  {
  }

  plString m_sCategory;
  plString m_sName;
  bool m_bBuiltInTag = false; ///< If set to true, this is a tag created by code that the user is not allowed to remove
};

class PL_TOOLSFOUNDATION_DLL plToolsTagRegistry
{
public:
  /// \brief Removes all tags that are not specified as 'built-in'
  static void Clear();

  static void WriteToDDL(plStreamWriter& inout_stream);
  static plStatus ReadFromDDL(plStreamReader& inout_stream);

  static bool AddTag(const plToolsTag& tag);
  static bool RemoveTag(plStringView sName);

  static void GetAllTags(plHybridArray<const plToolsTag*, 16>& out_tags);
  static void GetTagsByCategory(const plArrayPtr<plStringView>& categories, plHybridArray<const plToolsTag*, 16>& out_tags);

private:
  static plMap<plString, plToolsTag> s_NameToTags;
};
