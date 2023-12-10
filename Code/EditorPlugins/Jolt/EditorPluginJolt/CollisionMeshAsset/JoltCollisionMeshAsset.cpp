#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAsset.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <JoltCooking/JoltCooking.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plJoltCollisionMeshAssetDocument, 8, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static plMat3 CalculateTransformationMatrix(const plJoltCollisionMeshAssetProperties* pProp)
{
  const float us = plMath::Clamp(pProp->m_fUniformScaling, 0.0001f, 10000.0f);

  const plBasisAxis::Enum forwardDir = plBasisAxis::GetOrthogonalAxis(pProp->m_RightDir, pProp->m_UpDir, !pProp->m_bFlipForwardDir);

  return plBasisAxis::CalculateTransformationMatrix(forwardDir, pProp->m_RightDir, pProp->m_UpDir, us);
}

plJoltCollisionMeshAssetDocument::plJoltCollisionMeshAssetDocument(plStringView sDocumentPath, bool bConvexMesh)
  : plSimpleAssetDocument<plJoltCollisionMeshAssetProperties>(sDocumentPath, plAssetDocEngineConnection::Simple)
{
  m_bIsConvexMesh = bConvexMesh;
}

void plJoltCollisionMeshAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  // this logic is for backwards compatibility, to sync the convex state with existing data
  if (m_bIsConvexMesh)
  {
    GetPropertyObject()->GetTypeAccessor().SetValue("IsConvexMesh", m_bIsConvexMesh);
  }
  else
  {
    m_bIsConvexMesh = GetPropertyObject()->GetTypeAccessor().GetValue("IsConvexMesh").ConvertTo<bool>();
  }

  // the GetProperties object seems distinct from the GetPropertyObject, so keep them in sync
  GetProperties()->m_bIsConvexMesh = m_bIsConvexMesh;
}


//////////////////////////////////////////////////////////////////////////


plTransformStatus plJoltCollisionMeshAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  plProgressRange range("Transforming Asset", 2, false);

  plJoltCollisionMeshAssetProperties* pProp = GetProperties();

  const plUInt8 uiVersion = 2;
  stream << uiVersion;

  plUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  plCompressedStreamWriterZstd compressor(&stream, 0, plCompressedStreamWriterZstd::Compression::Average);
  plChunkStreamWriter chunk(compressor);
#else
  plChunkStreamWriter chunk(stream);
#endif

  stream << uiCompressionMode;

  chunk.BeginStream(1);

  {
    range.BeginNextStep("Preparing Mesh");

    plJoltCookingMesh xMesh;

    if (!m_bIsConvexMesh || pProp->m_ConvexMeshType == plJoltConvexCollisionMeshType::ConvexHull || pProp->m_ConvexMeshType == plJoltConvexCollisionMeshType::ConvexDecomposition)
    {
      PLASMA_SUCCEED_OR_RETURN(CreateMeshFromFile(xMesh));
    }
    else
    {
      const plMat3 mTransformation = CalculateTransformationMatrix(pProp);

      xMesh.m_bFlipNormals = plGraphicsUtils::IsTriangleFlipRequired(mTransformation);

      plGeometry geom;
      plGeometry::GeoOptions opt;
      opt.m_Transform = plMat4(mTransformation, plVec3::MakeZero());

      if (pProp->m_ConvexMeshType == plJoltConvexCollisionMeshType::Cylinder)
      {
        geom.AddCylinderOnePiece(pProp->m_fRadius, pProp->m_fRadius2, pProp->m_fHeight * 0.5f, pProp->m_fHeight * 0.5f, plMath::Clamp<plUInt16>(pProp->m_uiDetail, 3, 32), opt);
      }

      PLASMA_SUCCEED_OR_RETURN(CreateMeshFromGeom(geom, xMesh));
    }

    range.BeginNextStep("Writing Result");
    PLASMA_SUCCEED_OR_RETURN(WriteToStream(chunk, xMesh, GetProperties()));
  }

  chunk.EndStream();

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  PLASMA_SUCCEED_OR_RETURN(compressor.FinishCompressedStream());

  plLog::Dev("Compressed collision mesh data from {0} KB to {1} KB ({2}%%)", plArgF((float)compressor.GetUncompressedSize() / 1024.0f, 1), plArgF((float)compressor.GetCompressedSize() / 1024.0f, 1), plArgF(100.0f * compressor.GetCompressedSize() / compressor.GetUncompressedSize(), 1));

#endif

  return plStatus(PLASMA_SUCCESS);
}

plStatus plJoltCollisionMeshAssetDocument::CreateMeshFromFile(plJoltCookingMesh& outMesh)
{
  plJoltCollisionMeshAssetProperties* pProp = GetProperties();

  plStringBuilder sAbsFilename = pProp->m_sMeshFile;
  if (!plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
  {
    return plStatus(plFmt("Couldn't make path absolute: '{0};", sAbsFilename));
  }

  plUniquePtr<plModelImporter2::Importer> pImporter = plModelImporter2::RequestImporterForFileType(sAbsFilename);
  if (pImporter == nullptr)
    return plStatus("No known importer for this file type.");

  plMeshResourceDescriptor meshDesc;

  plModelImporter2::ImportOptions opt;
  opt.m_sSourceFile = sAbsFilename;
  opt.m_pMeshOutput = &meshDesc;
  opt.m_RootTransform = CalculateTransformationMatrix(pProp);

  if (pImporter->Import(opt).Failed())
    return plStatus("Model importer was unable to read this asset.");

  const auto& meshBuffer = meshDesc.MeshBufferDesc();

  const plUInt32 uiNumTriangles = meshBuffer.GetPrimitiveCount();
  const plUInt32 uiNumVertices = meshBuffer.GetVertexCount();

  outMesh.m_PolygonSurfaceID.SetCountUninitialized(uiNumTriangles);
  outMesh.m_VerticesInPolygon.SetCountUninitialized(uiNumTriangles);

  for (plUInt32 uiTriangle = 0; uiTriangle < uiNumTriangles; ++uiTriangle)
  {
    outMesh.m_PolygonSurfaceID[uiTriangle] = 0;  // default value, will be updated below when extracting materials.
    outMesh.m_VerticesInPolygon[uiTriangle] = 3; // Triangles!
  }

  // Extract vertices
  {
    const plUInt8* pVertexData = meshDesc.MeshBufferDesc().GetVertexData(0, 0).GetPtr();
    const plUInt32 uiVertexSize = meshBuffer.GetVertexDataSize();

    outMesh.m_Vertices.SetCountUninitialized(uiNumVertices);
    for (plUInt32 v = 0; v < uiNumVertices; ++v)
    {
      outMesh.m_Vertices[v] = *reinterpret_cast<const plVec3*>(pVertexData + v * uiVertexSize);
    }
  }

  // Extract indices
  {
    outMesh.m_PolygonIndices.SetCountUninitialized(uiNumTriangles * 3);

    if (meshBuffer.Uses32BitIndices())
    {
      const plUInt32* pIndices = reinterpret_cast<const plUInt32*>(meshBuffer.GetIndexBufferData().GetPtr());

      for (plUInt32 tri = 0; tri < uiNumTriangles * 3; ++tri)
      {
        outMesh.m_PolygonIndices[tri] = pIndices[tri];
      }
    }
    else
    {
      const plUInt16* pIndices = reinterpret_cast<const plUInt16*>(meshBuffer.GetIndexBufferData().GetPtr());

      for (plUInt32 tri = 0; tri < uiNumTriangles * 3; ++tri)
      {
        outMesh.m_PolygonIndices[tri] = pIndices[tri];
      }
    }
  }

  // Extract Material Information
  if (m_bIsConvexMesh)
  {
    meshDesc.CollapseSubMeshes();
    pProp->m_Slots.SetCount(1);
    pProp->m_Slots[0].m_sLabel = "Convex";
    pProp->m_Slots[0].m_sResource = pProp->m_sConvexMeshSurface;

    const auto subMeshInfo = meshDesc.GetSubMeshes()[0];

    for (plUInt32 tri = 0; tri < subMeshInfo.m_uiPrimitiveCount; ++tri)
    {
      outMesh.m_PolygonSurfaceID[subMeshInfo.m_uiFirstPrimitive + tri] = 0;
    }
  }
  else
  {
    pProp->m_Slots.SetCount(meshDesc.GetSubMeshes().GetCount());

    for (plUInt32 matIdx = 0; matIdx < pImporter->m_OutputMaterials.GetCount(); ++matIdx)
    {
      const plInt32 subMeshIdx = pImporter->m_OutputMaterials[matIdx].m_iReferencedByMesh;
      if (subMeshIdx < 0)
        continue;

      pProp->m_Slots[subMeshIdx].m_sLabel = pImporter->m_OutputMaterials[matIdx].m_sName;

      const auto subMeshInfo = meshDesc.GetSubMeshes()[subMeshIdx];

      if (pProp->m_Slots[subMeshIdx].m_bExclude)
      {
        // update the triangle material information
        for (plUInt32 tri = 0; tri < subMeshInfo.m_uiPrimitiveCount; ++tri)
        {
          outMesh.m_PolygonSurfaceID[subMeshInfo.m_uiFirstPrimitive + tri] = 0xFFFF;
        }
      }
      else
      {
        // update the triangle material information
        for (plUInt32 tri = 0; tri < subMeshInfo.m_uiPrimitiveCount; ++tri)
        {
          outMesh.m_PolygonSurfaceID[subMeshInfo.m_uiFirstPrimitive + tri] = subMeshIdx;
        }
      }
    }

    ApplyNativePropertyChangesToObjectManager();
  }

  return plStatus(PLASMA_SUCCESS);
}

plStatus plJoltCollisionMeshAssetDocument::CreateMeshFromGeom(plGeometry& geom, plJoltCookingMesh& outMesh)
{
  plJoltCollisionMeshAssetProperties* pProp = GetProperties();

  // Material setup.
  {
    // Ensure there is just one slot.
    if (pProp->m_Slots.GetCount() != 1)
    {
      GetObjectAccessor()->StartTransaction("Update Mesh Material Info");

      pProp->m_Slots.SetCount(1);
      pProp->m_Slots[0].m_sLabel = "Default";

      ApplyNativePropertyChangesToObjectManager();
      GetObjectAccessor()->FinishTransaction();

      // Need to reacquire pProp pointer since it might be reallocated.
      pProp = GetProperties();
      PLASMA_IGNORE_UNUSED(pProp);
    }
  }

  // copy vertex positions
  {
    outMesh.m_Vertices.SetCountUninitialized(geom.GetVertices().GetCount());
    for (plUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
    {
      outMesh.m_Vertices[v] = geom.GetVertices()[v].m_vPosition;
    }
  }

  // Copy Polygon Data
  {
    outMesh.m_PolygonSurfaceID.SetCountUninitialized(geom.GetPolygons().GetCount());
    outMesh.m_VerticesInPolygon.SetCountUninitialized(geom.GetPolygons().GetCount());
    outMesh.m_PolygonIndices.Reserve(geom.GetPolygons().GetCount() * 4);

    for (plUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
    {
      const auto& poly = geom.GetPolygons()[p];
      outMesh.m_VerticesInPolygon[p] = poly.m_Vertices.GetCount();
      outMesh.m_PolygonSurfaceID[p] = 0;

      for (plUInt32 posIdx : poly.m_Vertices)
      {
        outMesh.m_PolygonIndices.PushBack(posIdx);
      }
    }
  }

  return plStatus(PLASMA_SUCCESS);
}

plStatus plJoltCollisionMeshAssetDocument::WriteToStream(plChunkStreamWriter& inout_stream, const plJoltCookingMesh& mesh, const plJoltCollisionMeshAssetProperties* pProp)
{
  plHybridArray<plString, 32> surfaces;

  for (const auto& slot : pProp->m_Slots)
  {
    surfaces.PushBack(slot.m_sResource);
  }

  plJoltCooking::MeshType meshType = plJoltCooking::MeshType::Triangle;

  if (pProp->m_bIsConvexMesh)
  {
    if (pProp->m_ConvexMeshType == plJoltConvexCollisionMeshType::ConvexDecomposition)
    {
      meshType = plJoltCooking::MeshType::ConvexDecomposition;
    }
    else
    {
      meshType = plJoltCooking::MeshType::ConvexHull;
    }
  }

  return plJoltCooking::WriteResourceToStream(inout_stream, mesh, surfaces, meshType, pProp->m_uiMaxConvexPieces);
}

plTransformStatus plJoltCollisionMeshAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  plStatus status = plAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

void plJoltCollisionMeshAssetDocument::UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  if (GetProperties()->m_ConvexMeshType != plJoltConvexCollisionMeshType::ConvexHull)
  {
    // remove the mesh file dependency, if it is not actually used
    const auto& sMeshFile = GetProperties()->m_sMeshFile;
    pInfo->m_TransformDependencies.Remove(sMeshFile);
  }
}

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plJoltCollisionMeshAssetDocumentGenerator, 1, plRTTIDefaultAllocator<plJoltCollisionMeshAssetDocumentGenerator>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plJoltCollisionMeshAssetDocumentGenerator::plJoltCollisionMeshAssetDocumentGenerator()
{
  AddSupportedFileType("obj");
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

plJoltCollisionMeshAssetDocumentGenerator::~plJoltCollisionMeshAssetDocumentGenerator() = default;

void plJoltCollisionMeshAssetDocumentGenerator::GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    plAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = plAssetDocGeneratorPriority::DefaultPriority;
    info.m_sName = "Jolt_Colmesh_Triangle";
    info.m_sIcon = ":/AssetIcons/Jolt_Collision_Mesh.svg";
  }
}

plStatus plJoltCollisionMeshAssetDocumentGenerator::Generate(plStringView sInputFileAbs, plStringView sMode, plDocument*& out_pGeneratedDocument)
{
  plStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());
  plOSFile::FindFreeFilename(sOutFile);

  auto pApp = plQtEditorApp::GetSingleton();

  plStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  out_pGeneratedDocument = pApp->CreateDocument(sOutFile, plDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return plStatus("Could not create target document");

  plJoltCollisionMeshAssetDocument* pAssetDoc = plDynamicCast<plJoltCollisionMeshAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return plStatus("Target document is not a valid plJoltCollisionMeshAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", sInputFileRel.GetView());

  return plStatus(PLASMA_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plJoltConvexCollisionMeshAssetDocumentGenerator, 1, plRTTIDefaultAllocator<plJoltConvexCollisionMeshAssetDocumentGenerator>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plJoltConvexCollisionMeshAssetDocumentGenerator::plJoltConvexCollisionMeshAssetDocumentGenerator()
{
  AddSupportedFileType("obj");
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

plJoltConvexCollisionMeshAssetDocumentGenerator::~plJoltConvexCollisionMeshAssetDocumentGenerator() = default;

void plJoltConvexCollisionMeshAssetDocumentGenerator::GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    plAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = plAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "Jolt_Colmesh_Convex";
    info.m_sIcon = ":/AssetIcons/Jolt_Collision_Mesh_Convex.svg";
  }
}

plStatus plJoltConvexCollisionMeshAssetDocumentGenerator::Generate(plStringView sInputFileAbs, plStringView sMode, plDocument*& out_pGeneratedDocument)
{
  plStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());
  plOSFile::FindFreeFilename(sOutFile);

  auto pApp = plQtEditorApp::GetSingleton();

  plStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  out_pGeneratedDocument = pApp->CreateDocument(sOutFile, plDocumentFlags::None);
  if (out_pGeneratedDocument == nullptr)
    return plStatus("Could not create target document");

  plJoltCollisionMeshAssetDocument* pAssetDoc = plDynamicCast<plJoltCollisionMeshAssetDocument*>(out_pGeneratedDocument);
  if (pAssetDoc == nullptr)
    return plStatus("Target document is not a valid plJoltCollisionMeshAssetDocument");

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("MeshFile", sInputFileRel.GetView());

  return plStatus(PLASMA_SUCCESS);
}
