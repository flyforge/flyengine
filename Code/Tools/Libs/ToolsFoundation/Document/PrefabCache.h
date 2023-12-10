#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/UniquePtr.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class plAbstractObjectGraph;

class PLASMA_TOOLSFOUNDATION_DLL plPrefabCache
{
  PLASMA_DECLARE_SINGLETON(plPrefabCache);

public:
  plPrefabCache();

  const plStringBuilder& GetCachedPrefabDocument(const plUuid& documentGuid);
  const plAbstractObjectGraph* GetCachedPrefabGraph(const plUuid& documentGuid);
  void LoadGraph(plAbstractObjectGraph& out_graph, plStringView sGraph);

private:
  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, plPrefabCache);

  struct PrefabData
  {
    PrefabData() = default;

    plUuid m_documentGuid;
    plString m_sAbsPath;

    plAbstractObjectGraph m_Graph;
    plStringBuilder m_sDocContent;
    plTimestamp m_fileModifiedTime;
  };
  PrefabData& GetOrCreatePrefabCache(const plUuid& documentGuid);
  void UpdatePrefabData(PrefabData& data);

  plMap<plUInt64, plUniquePtr<plAbstractObjectGraph>> m_CachedGraphs;
  plMap<plUuid, plUniquePtr<PrefabData>> m_PrefabData;
};
