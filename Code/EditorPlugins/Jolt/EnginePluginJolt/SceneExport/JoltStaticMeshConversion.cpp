#include <EnginePluginJolt/EnginePluginJoltPCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <EnginePluginJolt/SceneExport/JoltStaticMeshConversion.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <JoltCooking/JoltCooking.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSceneExportModifier_JoltStaticMeshConversion, 1, plRTTIDefaultAllocator<plSceneExportModifier_JoltStaticMeshConversion>)
PL_END_DYNAMIC_REFLECTED_TYPE;

void plSceneExportModifier_JoltStaticMeshConversion::ModifyWorld(plWorld& ref_world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport)
{
  if (sDocumentType == "Prefab")
  {
    // the auto generated static meshes are needed in the prefab document, so that physical interactions for previewing purposes work
    // however, the scene also exports the static colmesh, including all the prefabs (with overridden materials)
    // in the final scene this would create double colmeshes in the same place, but the materials may differ
    // therefore we don't want to export the colmesh other than for preview purposes, so we ignore this, if 'bForExport' is true

    if (bForExport)
    {
      return;
    }
  }

  PL_LOCK(ref_world.GetWriteMarker());

  plSmcDescription desc;
  desc.m_Surfaces.PushBack(); // add a dummy empty material

  plMsgBuildStaticMesh msg;
  msg.m_pStaticMeshDescription = &desc;

  for (auto it = ref_world.GetObjects(); it.IsValid(); ++it)
  {
    if (!it->IsStatic())
      continue;

    it->SendMessage(msg);
  }

  if (desc.m_SubMeshes.IsEmpty() || desc.m_Vertices.IsEmpty() || desc.m_Triangles.IsEmpty())
    return;

  const plUInt32 uiNumVertices = desc.m_Vertices.GetCount();
  const plUInt32 uiNumTriangles = desc.m_Triangles.GetCount();
  const plUInt32 uiNumSubMeshes = desc.m_SubMeshes.GetCount();

  plJoltCookingMesh xMesh;
  xMesh.m_Vertices.SetCountUninitialized(uiNumVertices);

  for (plUInt32 i = 0; i < uiNumVertices; ++i)
  {
    xMesh.m_Vertices[i] = desc.m_Vertices[i];
  }

  xMesh.m_PolygonIndices.SetCountUninitialized(uiNumTriangles * 3);
  xMesh.m_VerticesInPolygon.SetCountUninitialized(uiNumTriangles);
  xMesh.m_PolygonSurfaceID.SetCount(uiNumTriangles);

  for (plUInt32 i = 0; i < uiNumTriangles; ++i)
  {
    xMesh.m_VerticesInPolygon[i] = 3;

    xMesh.m_PolygonIndices[i * 3 + 0] = desc.m_Triangles[i].m_uiVertexIndices[0];
    xMesh.m_PolygonIndices[i * 3 + 1] = desc.m_Triangles[i].m_uiVertexIndices[1];
    xMesh.m_PolygonIndices[i * 3 + 2] = desc.m_Triangles[i].m_uiVertexIndices[2];
  }

  plHybridArray<plString, 32> surfaces;

  // copy materials
  // we could collate identical materials here and merge meshes, but the mesh cooking will probably do the same already
  {
    for (plUInt32 i = 0; i < desc.m_Surfaces.GetCount(); ++i)
    {
      surfaces.PushBack(desc.m_Surfaces[i]);
    }

    for (plUInt32 i = 0; i < uiNumSubMeshes; ++i)
    {
      const plUInt32 uiLastTriangle = desc.m_SubMeshes[i].m_uiFirstTriangle + desc.m_SubMeshes[i].m_uiNumTriangles;
      const plUInt16 uiSurface = desc.m_SubMeshes[i].m_uiSurfaceIndex;

      for (plUInt32 t = desc.m_SubMeshes[i].m_uiFirstTriangle; t < uiLastTriangle; ++t)
      {
        xMesh.m_PolygonSurfaceID[t] = uiSurface;
      }
    }
  }

  plStringBuilder sDocGuid, sOutputFile;
  plConversionUtils::ToString(documentGuid, sDocGuid);

  sOutputFile.SetFormat(":project/AssetCache/Generated/{0}.plJoltMesh", sDocGuid);

  plDeferredFileWriter file;
  file.SetOutput(sOutputFile);

  plAssetFileHeader header;
  header.Write(file).IgnoreResult();

  plChunkStreamWriter chunk(file);
  chunk.BeginStream(1);

  if (plJoltCooking::WriteResourceToStream(chunk, xMesh, surfaces, plJoltCooking::MeshType::Triangle).LogFailure())
  {
    plLog::Error("Could not write to global collision mesh file");
    return;
  }

  chunk.EndStream();

  if (file.Close().Failed())
  {
    plLog::Error("Could not write to global collision mesh file");
    return;
  }

  {
    plGameObject* pGo;
    plGameObjectDesc god;
    god.m_sName.Assign("Greybox Collision Mesh");
    ref_world.CreateObject(god, pGo);

    auto* pCompMan = ref_world.GetOrCreateComponentManager<plJoltStaticActorComponentManager>();

    plJoltStaticActorComponent* pComp;
    pCompMan->CreateComponent(pGo, pComp);

    pComp->SetMeshFile(sOutputFile);
  }
}
