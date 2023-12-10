#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Project/ToolsProject.h>

PLASMA_IMPLEMENT_SINGLETON(plPrefabCache);

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, plPrefabCache)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    PLASMA_DEFAULT_NEW(plPrefabCache);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plPrefabCache* pDummy = plPrefabCache::GetSingleton();
    PLASMA_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

plPrefabCache::plPrefabCache()
  : m_SingletonRegistrar(this)
{
}

const plStringBuilder& plPrefabCache::GetCachedPrefabDocument(const plUuid& documentGuid)
{
  PrefabData& data = plPrefabCache::GetOrCreatePrefabCache(documentGuid);
  return data.m_sDocContent;
}

const plAbstractObjectGraph* plPrefabCache::GetCachedPrefabGraph(const plUuid& documentGuid)
{
  PrefabData& data = plPrefabCache::GetOrCreatePrefabCache(documentGuid);
  if (data.m_sAbsPath.IsEmpty())
    return nullptr;
  return &data.m_Graph;
}

void plPrefabCache::LoadGraph(plAbstractObjectGraph& out_graph, plStringView sGraph)
{
  plUInt64 uiHash = plHashingUtils::xxHash64(sGraph.GetStartPointer(), sGraph.GetElementCount());
  auto it = m_CachedGraphs.Find(uiHash);
  if (!it.IsValid())
  {
    it = m_CachedGraphs.Insert(uiHash, plUniquePtr<plAbstractObjectGraph>(PLASMA_DEFAULT_NEW(plAbstractObjectGraph)));

    plRawMemoryStreamReader stringReader(sGraph.GetStartPointer(), sGraph.GetElementCount());
    plUniquePtr<plAbstractObjectGraph> header;
    plUniquePtr<plAbstractObjectGraph> types;
    plAbstractGraphDdlSerializer::ReadDocument(stringReader, header, it.Value(), types, true).IgnoreResult();
  }

  it.Value()->Clone(out_graph);
}

plPrefabCache::PrefabData& plPrefabCache::GetOrCreatePrefabCache(const plUuid& documentGuid)
{
  auto it = m_PrefabData.Find(documentGuid);
  if (it.IsValid())
  {
    plFileStats Stats;
    if (plOSFile::GetFileStats(it.Value()->m_sAbsPath, Stats).Succeeded() && !Stats.m_LastModificationTime.Compare(it.Value()->m_fileModifiedTime, plTimestamp::CompareMode::FileTimeEqual))
    {
      UpdatePrefabData(*it.Value().Borrow());
    }
  }
  else
  {
    it = m_PrefabData.Insert(documentGuid, plUniquePtr<PrefabData>(PLASMA_DEFAULT_NEW(PrefabData)));

    it.Value()->m_documentGuid = documentGuid;
    it.Value()->m_sAbsPath = plToolsProject::GetSingleton()->GetPathForDocumentGuid(documentGuid);
    if (it.Value()->m_sAbsPath.IsEmpty())
    {
      plStringBuilder sGuid;
      plConversionUtils::ToString(documentGuid, sGuid);
      plLog::Error("Can't resolve prefab document guid '{0}'. The resolved path is empty", sGuid);
    }
    else
      UpdatePrefabData(*it.Value().Borrow());
  }

  return *it.Value().Borrow();
}

void plPrefabCache::UpdatePrefabData(PrefabData& data)
{
  if (data.m_sAbsPath.IsEmpty())
  {
    data.m_sAbsPath = plToolsProject::GetSingleton()->GetPathForDocumentGuid(data.m_documentGuid);
    if (data.m_sAbsPath.IsEmpty())
    {
      plStringBuilder sGuid;
      plConversionUtils::ToString(data.m_documentGuid, sGuid);
      plLog::Error("Can't resolve prefab document guid '{0}'. The resolved path is empty", sGuid);
      return;
    }
  }

  plFileStats Stats;
  bool bStat = plOSFile::GetFileStats(data.m_sAbsPath, Stats).Succeeded();

  if (!bStat)
  {
    plLog::Error("Can't update prefab file '{0}', the file can't be opened.", data.m_sAbsPath);
    return;
  }

  data.m_sDocContent = plPrefabUtils::ReadDocumentAsString(data.m_sAbsPath);

  if (data.m_sDocContent.IsEmpty())
    return;

  data.m_fileModifiedTime = Stats.m_LastModificationTime;
  data.m_Graph.Clear();
  plPrefabUtils::LoadGraph(data.m_Graph, data.m_sDocContent);
}
