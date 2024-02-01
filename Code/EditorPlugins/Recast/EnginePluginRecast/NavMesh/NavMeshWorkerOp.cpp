#include <EnginePluginRecast/EnginePluginRecastPCH.h>

#include <EnginePluginRecast/NavMesh/NavMeshWorkerOp.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Utilities/Progress.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLongOpWorker_BuildNavMesh, 1, plRTTIDefaultAllocator<plLongOpWorker_BuildNavMesh>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plLongOpWorker_BuildNavMesh::InitializeExecution(plStreamReader& ref_config, const plUuid& documentGuid)
{
  plEngineProcessDocumentContext* pDocContext = plEngineProcessDocumentContext::GetDocumentContext(documentGuid);

  if (pDocContext == nullptr)
    return PL_FAILURE;

  ref_config >> m_sOutputPath;
  PL_SUCCEED_OR_RETURN(m_NavMeshConfig.Deserialize(ref_config));

  PL_SUCCEED_OR_RETURN(plRecastNavMeshBuilder::ExtractWorldGeometry(*pDocContext->GetWorld(), m_ExtractedObjects));

  return PL_SUCCESS;
}

plResult plLongOpWorker_BuildNavMesh::Execute(plProgress& ref_progress, plStreamWriter& ref_proxydata)
{
  plProgressRange pgRange("Generating NavMesh", 2, true, &ref_progress);
  pgRange.SetStepWeighting(0, 0.95f);
  pgRange.SetStepWeighting(1, 0.05f);

  plRecastNavMeshBuilder NavMeshBuilder;
  plRecastNavMeshResourceDescriptor desc;

  if (!pgRange.BeginNextStep("Building NavMesh"))
    return PL_FAILURE;

  PL_SUCCEED_OR_RETURN(NavMeshBuilder.Build(m_NavMeshConfig, m_ExtractedObjects, desc, ref_progress));

  if (!pgRange.BeginNextStep("Writing Result"))
    return PL_FAILURE;

  plFileWriter file;
  PL_SUCCEED_OR_RETURN(file.Open(m_sOutputPath));

  // not really used for navmeshes, as they are not strictly linked to a specific version of a scene document
  // thus the scene hash and document version are irrelevant and should just stay static for now
  plAssetFileHeader header;
  header.SetFileHashAndVersion(1, 1);
  PL_SUCCEED_OR_RETURN(header.Write(file));

  PL_SUCCEED_OR_RETURN(desc.Serialize(file));

  return PL_SUCCESS;
}
