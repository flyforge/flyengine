#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Logging/Log.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

struct TagComparer
{
  PLASMA_ALWAYS_INLINE bool Less(const plToolsTag* a, const plToolsTag* b) const
  {
    if (a->m_sCategory != b->m_sCategory)
      return a->m_sCategory < b->m_sCategory;

    return a->m_sName < b->m_sName;
    ;
  }
};
////////////////////////////////////////////////////////////////////////
// plToolsTagRegistry public functions
////////////////////////////////////////////////////////////////////////

plMap<plString, plToolsTag> plToolsTagRegistry::s_NameToTags;

void plToolsTagRegistry::Clear()
{
  for (auto it = s_NameToTags.GetIterator(); it.IsValid();)
  {
    if (!it.Value().m_bBuiltInTag)
    {
      it = s_NameToTags.Remove(it);
    }
    else
    {
      ++it;
    }
  }
}

void plToolsTagRegistry::WriteToDDL(plStreamWriter& stream)
{
  plOpenDdlWriter writer;
  writer.SetOutputStream(&stream);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(plOpenDdlWriter::TypeStringMode::ShortenedUnsignedInt);

  for (auto it = s_NameToTags.GetIterator(); it.IsValid(); ++it)
  {
    writer.BeginObject("Tag");

    writer.BeginPrimitiveList(plOpenDdlPrimitiveType::String, "Name");
    writer.WriteString(it.Value().m_sName);
    writer.EndPrimitiveList();

    writer.BeginPrimitiveList(plOpenDdlPrimitiveType::String, "Category");
    writer.WriteString(it.Value().m_sCategory);
    writer.EndPrimitiveList();

    writer.EndObject();
  }
}

plStatus plToolsTagRegistry::ReadFromDDL(plStreamReader& stream)
{
  plOpenDdlReader reader;
  if (reader.ParseDocument(stream).Failed())
  {
    return plStatus("Failed to read data from ToolsTagRegistry stream!");
  }

  // Makes sure not to remove the built-in tags
  Clear();

  const plOpenDdlReaderElement* pRoot = reader.GetRootElement();

  for (const plOpenDdlReaderElement* pTags = pRoot->GetFirstChild(); pTags != nullptr; pTags = pTags->GetSibling())
  {
    if (!pTags->IsCustomType("Tag"))
      continue;

    const plOpenDdlReaderElement* pName = pTags->FindChildOfType(plOpenDdlPrimitiveType::String, "Name");
    const plOpenDdlReaderElement* pCategory = pTags->FindChildOfType(plOpenDdlPrimitiveType::String, "Category");

    if (!pName || !pCategory)
    {
      plLog::Error("Incomplete tag declaration!");
      continue;
    }

    plToolsTag tag;
    tag.m_sName = pName->GetPrimitivesString()[0];
    tag.m_sCategory = pCategory->GetPrimitivesString()[0];

    if (!plToolsTagRegistry::AddTag(tag))
    {
      plLog::Error("Failed to add tag '{0}'", tag.m_sName);
    }
  }

  return plStatus(PLASMA_SUCCESS);
}

bool plToolsTagRegistry::AddTag(const plToolsTag& tag)
{
  if (tag.m_sName.IsEmpty())
    return false;

  auto it = s_NameToTags.Find(tag.m_sName);
  if (it.IsValid())
  {
    if (tag.m_bBuiltInTag)
    {
      // Make sure to pass this on, as it is not stored in the DDL file (because we don't want to rely on that)
      it.Value().m_bBuiltInTag = true;
    }

    return true;
  }
  else
  {
    s_NameToTags[tag.m_sName] = tag;
    return true;
  }
}

bool plToolsTagRegistry::RemoveTag(const char* szName)
{
  auto it = s_NameToTags.Find(szName);
  if (it.IsValid())
  {
    s_NameToTags.Remove(it);
    return true;
  }
  else
  {
    return false;
  }
}

void plToolsTagRegistry::GetAllTags(plHybridArray<const plToolsTag*, 16>& out_tags)
{
  out_tags.Clear();
  for (auto it = s_NameToTags.GetIterator(); it.IsValid(); ++it)
  {
    out_tags.PushBack(&it.Value());
  }

  out_tags.Sort(TagComparer());
}

void plToolsTagRegistry::GetTagsByCategory(const plArrayPtr<plStringView>& categories, plHybridArray<const plToolsTag*, 16>& out_tags)
{
  out_tags.Clear();
  for (auto it = s_NameToTags.GetIterator(); it.IsValid(); ++it)
  {
    if (std::any_of(cbegin(categories), cend(categories), [&it](const plStringView& cat) { return it.Value().m_sCategory == cat; }))
    {
      out_tags.PushBack(&it.Value());
    }
  }
  out_tags.Sort(TagComparer());
}
