#include <EnginePluginRecast/EnginePluginRecastPCH.h>

#include <EnginePluginRecast/NavMesh/NavMeshWorkerOp.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Utilities/Progress.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLongOpWorker_BuildNavMesh, 1, plRTTIDefaultAllocator<plLongOpWorker_BuildNavMesh>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plLongOpWorker_BuildNavMesh::InitializeExecution(plStreamReader& config, const plUuid& DocumentGuid)
{
  plEngineProcessDocumentContext* pDocContext = plEngineProcessDocumentContext::GetDocumentContext(DocumentGuid);

  if (pDocContext == nullptr)
    return PLASMA_FAILURE;

  config >> m_sOutputPath;
  PLASMA_SUCCEED_OR_RETURN(m_NavMeshConfig.Deserialize(config));

  PLASMA_SUCCEED_OR_RETURN(plRecastNavMeshBuilder::ExtractWorldGeometry(*pDocContext->GetWorld(), m_ExtractedObjects));

  return PLASMA_SUCCESS;
}

plResult plLongOpWorker_BuildNavMesh::Execute(plProgress& progress, plStreamWriter& proxydata)
{
  plProgressRange pgRange("Generating NavMesh", 2, true, &progress);
  pgRange.SetStepWeighting(0, 0.95f);
  pgRange.SetStepWeighting(1, 0.05f);

  plRecastNavMeshBuilder NavMeshBuilder;
  plRecastNavMeshResourceDescriptor desc;

  if (!pgRange.BeginNextStep("Building NavMesh"))
    return PLASMA_FAILURE;

  PLASMA_SUCCEED_OR_RETURN(NavMeshBuilder.Build(m_NavMeshConfig, m_ExtractedObjects, desc, progress));

  if (!pgRange.BeginNextStep("Writing Result"))
    return PLASMA_FAILURE;

  plFileWriter file;
  PLASMA_SUCCEED_OR_RETURN(file.Open(m_sOutputPath));

  // not really used for navmeshes, as they are not strictly linked to a specific version of a scene document
  // thus the scene hash and document version are irrelevant and should just stay static for now
  plAssetFileHeader header;
  header.SetFileHashAndVersion(1, 1);
  PLASMA_SUCCEED_OR_RETURN(header.Write(file));

  PLASMA_SUCCEED_OR_RETURN(desc.Serialize(file));

  return PLASMA_SUCCESS;
}
