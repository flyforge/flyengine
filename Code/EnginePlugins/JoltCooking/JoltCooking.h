#pragma once

#include <Foundation/Types/Status.h>
#include <JoltCooking/JoltCookingDLL.h>

class plStreamWriter;
class plChunkStreamWriter;

struct PLASMA_JOLTCOOKING_DLL plJoltCookingMesh
{
  bool m_bFlipNormals = false;
  plDynamicArray<plVec3> m_Vertices;
  plDynamicArray<plUInt8> m_VerticesInPolygon;
  plDynamicArray<plUInt32> m_PolygonIndices;
  plDynamicArray<plUInt16> m_PolygonSurfaceID;
};

class PLASMA_JOLTCOOKING_DLL plJoltCooking
{
public:
  enum class MeshType
  {
    Triangle,
    ConvexHull,
    ConvexDecomposition
  };

  static plResult CookTriangleMesh(const plJoltCookingMesh& mesh, plStreamWriter& ref_outputStream);
  static plResult CookConvexMesh(const plJoltCookingMesh& mesh, plStreamWriter& ref_outputStream);
  static plResult ComputeConvexHull(const plJoltCookingMesh& mesh, plJoltCookingMesh& out_mesh);
  static plStatus WriteResourceToStream(plChunkStreamWriter& inout_stream, const plJoltCookingMesh& mesh, const plArrayPtr<plString>& surfaces, MeshType meshType, plUInt32 uiMaxConvexPieces = 1);
  static plResult CookDecomposedConvexMesh(const plJoltCookingMesh& mesh, plStreamWriter& ref_outputStream, plUInt32 uiMaxConvexPieces);

private:
  static plResult CookSingleConvexJoltMesh(const plJoltCookingMesh& mesh, plStreamWriter& OutputStream);
};
