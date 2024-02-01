#include <JoltCooking/JoltCookingPCH.h>

#include <Core/Graphics/ConvexHull.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/Progress.h>
#include <JoltCooking/JoltCooking.h>

#include <Foundation/IO/MemoryStream.h>
#include <Jolt/Core/StreamOut.h>
#include <Jolt/Geometry/IndexedTriangle.h>
#include <Jolt/Math/Float3.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <Physics/Collision/Shape/ConvexHullShape.h>

#define ENABLE_VHACD_IMPLEMENTATION 1
#include <VHACD/VHACD.h>
using namespace VHACD;

class plJoltStreamOut : public JPH::StreamOut
{
public:
  plJoltStreamOut(plStreamWriter* pPassThrough)
  {
    m_pWriter = pPassThrough;
  }

  virtual void WriteBytes(const void* pInData, size_t uiInNumBytes) override
  {
    if (m_pWriter->WriteBytes(pInData, uiInNumBytes).Failed())
      m_bFailed = true;
  }

  virtual bool IsFailed() const override
  {
    return m_bFailed;
  }


private:
  plStreamWriter* m_pWriter = nullptr;
  bool m_bFailed = false;
};

plResult plJoltCooking::CookTriangleMesh(const plJoltCookingMesh& mesh, plStreamWriter& ref_outputStream)
{
  if (JPH::Allocate == nullptr)
  {
    // make sure an allocator exists
    JPH::RegisterDefaultAllocator();
  }

  JPH::VertexList vertexList;
  JPH::IndexedTriangleList triangleList;

  // copy vertices
  {
    vertexList.resize(mesh.m_Vertices.GetCount());
    for (plUInt32 i = 0; i < mesh.m_Vertices.GetCount(); ++i)
    {
      vertexList[i] = plJoltConversionUtils::ToFloat3(mesh.m_Vertices[i]);
    }
  }

  plUInt32 uiMaxMaterialIndex = 0;

  // compute number of triangles
  {
    plUInt32 uiTriangles = 0;

    for (plUInt32 i = 0; i < mesh.m_VerticesInPolygon.GetCount(); ++i)
    {
      if (mesh.m_PolygonSurfaceID[i] == 0xFFFF)
        continue;

      uiTriangles += mesh.m_VerticesInPolygon[i] - 2;
    }

    triangleList.resize(uiTriangles);
  }

  // triangulate
  {
    plUInt32 uiIdxOffset = 0;
    plUInt32 uiTriIdx = 0;

    for (plUInt32 poly = 0; poly < mesh.m_VerticesInPolygon.GetCount(); ++poly)
    {
      const plUInt32 polyVerts = mesh.m_VerticesInPolygon[poly];

      if (mesh.m_PolygonSurfaceID[poly] != 0xFFFF)
      {
        for (plUInt32 tri = 0; tri < polyVerts - 2; ++tri)
        {
          const plUInt32 uiMaterialID = mesh.m_PolygonSurfaceID[poly];

          uiMaxMaterialIndex = plMath::Max(uiMaxMaterialIndex, uiMaterialID);

          const plUInt32 idx0 = mesh.m_PolygonIndices[uiIdxOffset + 0];
          const plUInt32 idx1 = mesh.m_PolygonIndices[uiIdxOffset + tri + 1];
          const plUInt32 idx2 = mesh.m_PolygonIndices[uiIdxOffset + tri + 2];

          if (idx0 == idx1 || idx0 == idx2 || idx1 == idx2)
          {
            // triangle is degenerate, remove it from the list
            triangleList.resize(triangleList.size() - 1);
            continue;
          }

          const plVec3 v0 = plJoltConversionUtils::ToVec3(vertexList[idx0]);
          const plVec3 v1 = plJoltConversionUtils::ToVec3(vertexList[idx1]);
          const plVec3 v2 = plJoltConversionUtils::ToVec3(vertexList[idx2]);

          if (v0.IsEqual(v1, 0.001f) || v0.IsEqual(v2, 0.001f) || v1.IsEqual(v2, 0.001f))
          {
            // triangle is degenerate, remove it from the list
            triangleList.resize(triangleList.size() - 1);
            continue;
          }

          triangleList[uiTriIdx].mMaterialIndex = uiMaterialID;
          triangleList[uiTriIdx].mIdx[0] = idx0;
          triangleList[uiTriIdx].mIdx[1] = idx1;
          triangleList[uiTriIdx].mIdx[2] = idx2;

          ++uiTriIdx;
        }
      }

      uiIdxOffset += polyVerts;
    }
  }

  // cook mesh (create Jolt shape, then save to binary stream)
  {
    JPH::MeshShapeSettings meshSettings(vertexList, triangleList);
    meshSettings.mMaterials.resize(uiMaxMaterialIndex + 1);

    auto shapeRes = meshSettings.Create();

    if (shapeRes.HasError())
    {
      plLog::Error("Cooking Jolt triangle mesh failed: {}", shapeRes.GetError().c_str());
      return PL_FAILURE;
    }

    plDefaultMemoryStreamStorage storage;
    plMemoryStreamWriter memWriter(&storage);

    plJoltStreamOut jOut(&memWriter);
    shapeRes.Get()->SaveBinaryState(jOut);

    ref_outputStream << storage.GetStorageSize32();
    storage.CopyToStream(ref_outputStream).AssertSuccess();

    const plUInt32 uiNumVertices = static_cast<plUInt32>(vertexList.size());
    ref_outputStream << uiNumVertices;

    const plUInt32 uiNumTriangles = shapeRes.Get()->GetStats().mNumTriangles;
    ref_outputStream << uiNumTriangles;
  }

  return PL_SUCCESS;
}

plResult plJoltCooking::CookConvexMesh(const plJoltCookingMesh& mesh0, plStreamWriter& ref_outputStream)
{
  plProgressRange range("Cooking Convex Mesh", 2, false);

  range.BeginNextStep("Computing Convex Hull");

  plJoltCookingMesh mesh;
  PL_SUCCEED_OR_RETURN(ComputeConvexHull(mesh0, mesh));

  range.BeginNextStep("Cooking Convex Hull");

  PL_SUCCEED_OR_RETURN(CookSingleConvexJoltMesh(mesh, ref_outputStream));

  return PL_SUCCESS;
}

PL_DEFINE_AS_POD_TYPE(JPH::Vec3);

plResult plJoltCooking::CookSingleConvexJoltMesh(const plJoltCookingMesh& mesh, plStreamWriter& OutputStream)
{
  if (JPH::Allocate == nullptr)
  {
    // make sure an allocator exists
    JPH::RegisterDefaultAllocator();
  }

  plHybridArray<JPH::Vec3, 256> verts;
  verts.SetCountUninitialized(mesh.m_Vertices.GetCount());

  for (plUInt32 i = 0; i < verts.GetCount(); ++i)
  {
    plVec3 v = mesh.m_Vertices[i];
    verts[i] = JPH::Vec3(v.x, v.y, v.z);
  }

  JPH::ConvexHullShapeSettings shapeSettings(verts.GetData(), (int)verts.GetCount());

  auto shapeRes = shapeSettings.Create();

  if (shapeRes.HasError())
  {
    plLog::Error("Cooking convex Jolt mesh failed: {}", shapeRes.GetError().c_str());
    return PL_FAILURE;
  }

  plDefaultMemoryStreamStorage storage;
  plMemoryStreamWriter memWriter(&storage);

  plJoltStreamOut jOut(&memWriter);
  shapeRes.Get()->SaveBinaryState(jOut);

  OutputStream << storage.GetStorageSize32();
  storage.CopyToStream(OutputStream).AssertSuccess();

  const plUInt32 uiNumVertices = verts.GetCount();
  OutputStream << uiNumVertices;

  const plUInt32 uiNumTriangles = shapeRes.Get()->GetStats().mNumTriangles;
  OutputStream << uiNumTriangles;

  return PL_SUCCESS;
}

plResult plJoltCooking::ComputeConvexHull(const plJoltCookingMesh& mesh, plJoltCookingMesh& out_mesh)
{
  plStopwatch timer;

  out_mesh.m_bFlipNormals = mesh.m_bFlipNormals;


  plConvexHullGenerator gen;
  if (gen.Build(mesh.m_Vertices).Failed())
  {
    plLog::Error("Computing the convex hull failed.");
    return PL_FAILURE;
  }

  plDynamicArray<plConvexHullGenerator::Face> faces;
  gen.Retrieve(out_mesh.m_Vertices, faces);

  if (faces.GetCount() >= 255)
  {
    plConvexHullGenerator gen2;
    gen2.SetSimplificationMinTriangleAngle(plAngle::MakeFromDegree(30));
    gen2.SetSimplificationFlatVertexNormalThreshold(plAngle::MakeFromDegree(10));
    gen2.SetSimplificationMinTriangleEdgeLength(0.08f);

    if (gen2.Build(out_mesh.m_Vertices).Failed())
    {
      plLog::Error("Computing the convex hull failed (second try).");
      return PL_FAILURE;
    }

    gen2.Retrieve(out_mesh.m_Vertices, faces);
  }


  for (const auto& face : faces)
  {
    out_mesh.m_VerticesInPolygon.ExpandAndGetRef() = 3;
    out_mesh.m_PolygonSurfaceID.ExpandAndGetRef() = 0;

    for (int vert = 0; vert < 3; ++vert)
      out_mesh.m_PolygonIndices.ExpandAndGetRef() = face.m_uiVertexIdx[vert];
  }

  plLog::Dev("Computed the convex hull in {0} milliseconds", plArgF(timer.GetRunningTotal().GetMilliseconds(), 1));
  return PL_SUCCESS;
}

plStatus plJoltCooking::WriteResourceToStream(plChunkStreamWriter& inout_stream, const plJoltCookingMesh& mesh, const plArrayPtr<plString>& surfaces, MeshType meshType, plUInt32 uiMaxConvexPieces)
{
  plResult resCooking = PL_FAILURE;

  {
    inout_stream.BeginChunk("Surfaces", 1);

    inout_stream << surfaces.GetCount();

    for (const auto& slot : surfaces)
    {
      inout_stream << slot;
    }

    inout_stream.EndChunk();
  }

  {
    inout_stream.BeginChunk("Details", 1);

    plBoundingBoxSphere aabb = plBoundingBoxSphere::MakeFromPoints(mesh.m_Vertices.GetData(), mesh.m_Vertices.GetCount());

    inout_stream << aabb;

    inout_stream.EndChunk();
  }

  if (meshType == MeshType::Triangle)
  {
    inout_stream.BeginChunk("TriangleMesh", 1);

    plStopwatch timer;
    resCooking = plJoltCooking::CookTriangleMesh(mesh, inout_stream);
    plLog::Dev("Triangle Mesh Cooking time: {0}s", plArgF(timer.GetRunningTotal().GetSeconds(), 2));

    inout_stream.EndChunk();
  }
  else
  {
    if (meshType == MeshType::ConvexDecomposition)
    {
      inout_stream.BeginChunk("ConvexDecompositionMesh", 1);

      plStopwatch timer;
      resCooking = plJoltCooking::CookDecomposedConvexMesh(mesh, inout_stream, uiMaxConvexPieces);
      plLog::Dev("Decomposed Convex Mesh Cooking time: {0}s", plArgF(timer.GetRunningTotal().GetSeconds(), 2));

      inout_stream.EndChunk();
    }

    if (meshType == MeshType::ConvexHull)
    {
      inout_stream.BeginChunk("ConvexMesh", 1);

      plStopwatch timer;
      resCooking = plJoltCooking::CookConvexMesh(mesh, inout_stream);
      plLog::Dev("Convex Mesh Cooking time: {0}s", plArgF(timer.GetRunningTotal().GetSeconds(), 2));

      inout_stream.EndChunk();
    }
  }

  if (resCooking.Failed())
    return plStatus("Cooking the collision mesh failed.");


  return plStatus(PL_SUCCESS);
}

plResult plJoltCooking::CookDecomposedConvexMesh(const plJoltCookingMesh& mesh, plStreamWriter& ref_outputStream, plUInt32 uiMaxConvexPieces)
{
  PL_LOG_BLOCK("Decomposing Mesh");

  IVHACD* pConDec = CreateVHACD();
  IVHACD::Parameters params;
  params.m_maxConvexHulls = plMath::Max(1u, uiMaxConvexPieces);

  if (uiMaxConvexPieces <= 2)
  {
    params.m_resolution = 10 * 10 * 10;
  }
  else if (uiMaxConvexPieces <= 5)
  {
    params.m_resolution = 20 * 20 * 20;
  }
  else if (uiMaxConvexPieces <= 10)
  {
    params.m_resolution = 40 * 40 * 40;
  }
  else if (uiMaxConvexPieces <= 25)
  {
    params.m_resolution = 60 * 60 * 60;
  }
  else if (uiMaxConvexPieces <= 50)
  {
    params.m_resolution = 80 * 80 * 80;
  }
  else
  {
    params.m_resolution = 100 * 100 * 100;
  }

  if (!pConDec->Compute(mesh.m_Vertices.GetData()->GetData(), mesh.m_Vertices.GetCount(), mesh.m_PolygonIndices.GetData(), mesh.m_VerticesInPolygon.GetCount(), params))
  {
    plLog::Error("Failed to compute convex decomposition");
    return PL_FAILURE;
  }

  plUInt16 uiNumParts = 0;

  for (plUInt32 i = 0; i < pConDec->GetNConvexHulls(); ++i)
  {
    IVHACD::ConvexHull ch;
    pConDec->GetConvexHull(i, ch);

    if (ch.m_triangles.empty())
      continue;

    ++uiNumParts;
  }

  plLog::Dev("Convex mesh parts: {}", uiNumParts);

  ref_outputStream << uiNumParts;

  for (plUInt32 i = 0; i < pConDec->GetNConvexHulls(); ++i)
  {
    IVHACD::ConvexHull ch;
    pConDec->GetConvexHull(i, ch);

    if (ch.m_triangles.empty())
      continue;

    plJoltCookingMesh chm;

    chm.m_Vertices.SetCount((plUInt32)ch.m_points.size());

    for (plUInt32 v = 0; v < (plUInt32)ch.m_points.size(); ++v)
    {
      chm.m_Vertices[v].Set((float)ch.m_points[v].mX, (float)ch.m_points[v].mY, (float)ch.m_points[v].mZ);
    }

    chm.m_VerticesInPolygon.SetCount((plUInt32)ch.m_triangles.size());
    chm.m_PolygonSurfaceID.SetCount((plUInt32)ch.m_triangles.size());
    chm.m_PolygonIndices.SetCount((plUInt32)ch.m_triangles.size() * 3);

    for (plUInt32 t = 0; t < (plUInt32)ch.m_triangles.size(); ++t)
    {
      chm.m_VerticesInPolygon[t] = 3;
      chm.m_PolygonSurfaceID[t] = 0;

      chm.m_PolygonIndices[t * 3 + 0] = ch.m_triangles[t].mI0;
      chm.m_PolygonIndices[t * 3 + 1] = ch.m_triangles[t].mI1;
      chm.m_PolygonIndices[t * 3 + 2] = ch.m_triangles[t].mI2;
    }

    PL_SUCCEED_OR_RETURN(CookSingleConvexJoltMesh(chm, ref_outputStream));
  }

  return PL_SUCCESS;
}

PL_STATICLINK_FILE(JoltCooking, JoltCooking_JoltCooking);
