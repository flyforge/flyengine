#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>

/// \brief Computes convex hulls for 3D meshes.
///
/// By default it will also simplify the result to a reasonable degree,
/// to reduce complexity and vertex/triangle count.
///
/// Currently there is an upper limit of 16384 vertices to accept meshes.
/// Everything larger than that will not be processed.
class PLASMA_CORE_DLL plConvexHullGenerator
{
public:
  struct Face
  {
    PLASMA_DECLARE_POD_TYPE();

    plUInt16 m_uiVertexIdx[3];
  };

  plConvexHullGenerator();
  ~plConvexHullGenerator();

  /// \brief Used to remove degenerate and unnecessary triangles that have corners with very little angle change.
  /// Ie. specifying 10 degree, means that all triangle corners must have at least a 10 degree change (and inner angle of 170 degree).
  /// Default is 22 degree.
  void SetSimplificationMinTriangleAngle(plAngle angle) { m_MinTriangleAngle = angle; }

  /// \brief Used to remove vertices that do not contribute much to the silhouette.
  /// Vertices whose adjacent triangle normals do not differ by more than angle, will be discarded.
  /// Default is 5 degree.
  void SetSimplificationFlatVertexNormalThreshold(plAngle angle) { m_FlatVertexNormalThreshold = angle; }

  /// \brief The minimum triangle edge length. Every edge shorter than this will be discarded and replaced by a single vertex at the
  /// average position.
  /// \note The length is not in 'mesh space' coordinates, but instead in 'unit cube space'.
  /// That means, every mesh is scaled to fit into a cube of size [-1; +1] for each axis. Thus the exact scale of the mesh does not matter
  /// when setting this value. Default is 0.05.
  void SetSimplificationMinTriangleEdgeLength(double len) { m_fMinTriangleEdgeLength = len; }

  /// \brief Generates the convex hull. Simplifies the mesh according to the previously specified parameters.
  plResult Build(const plArrayPtr<const plVec3> vertices);

  /// \brief When Build() was successful this can be called to retrieve the resulting vertices and triangles.
  void Retrieve(plDynamicArray<plVec3>& out_Vertices, plDynamicArray<Face>& out_Faces);

  /// \brief Same as Retrieve() but only returns the vertices.
  void RetrieveVertices(plDynamicArray<plVec3>& out_Vertices);

private:
  plResult ComputeCenterAndScale(const plArrayPtr<const plVec3> vertices);
  plResult StoreNormalizedVertices(const plArrayPtr<const plVec3> vertices);
  void StoreTriangle(plUInt16 i, plUInt16 j, plUInt16 k);
  plResult InitializeHull();
  plResult ComputeHull();
  bool IsInside(plUInt32 vtxId) const;
  void RemoveVisibleFaces(plUInt32 vtxId);
  void PatchHole(plUInt32 vtxId);
  bool PruneFlatVertices(double fNormalThreshold);
  bool PruneDegenerateTriangles(double fMaxCosAngle);
  bool PruneSmallTriangles(double fMaxEdgeLen);
  plResult ProcessVertices(const plArrayPtr<const plVec3> vertices);

  struct TwoSet
  {
    PLASMA_ALWAYS_INLINE TwoSet()
    {
      a = 0xFFFF;
      b = 0xFFFF;
    }
    PLASMA_ALWAYS_INLINE void Add(plUInt16 x) { (a == 0xFFFF ? a : b) = x; }
    PLASMA_ALWAYS_INLINE bool Contains(plUInt16 x) { return a == x || b == x; }
    PLASMA_ALWAYS_INLINE void Remove(plUInt16 x) { (a == x ? a : b) = 0xFFFF; }
    PLASMA_ALWAYS_INLINE int GetSize() { return (a != 0xFFFF) + (b != 0xFFFF); }

    plUInt16 a, b;
  };

  struct Triangle
  {
    plVec3d m_vNormal;
    double m_fPlaneDistance;
    plUInt16 m_uiVertexIdx[3];
    bool m_bFlip;
    bool m_bIsDegenerate;
  };

  // used for mesh simplification
  plAngle m_MinTriangleAngle = plAngle::MakeFromDegree(22.0f);
  plAngle m_FlatVertexNormalThreshold = plAngle::MakeFromDegree(5);
  double m_fMinTriangleEdgeLength = 0.05;

  plVec3d m_vCenter;
  double m_fScale;

  plVec3d m_vInside;

  // all the 'good' vertices (no duplicates)
  // normalized to be within a unit-cube
  plDynamicArray<plVec3d> m_Vertices;

  // Will be resized to Square(m_Vertices.GetCount())
  // Index [i * m_Vertices.GetCount() + j] indicates which (up to two) other points
  // combine with the edge i and j to make a triangle in the hull.  Only defined when i < j.
  plDynamicArray<TwoSet> m_Edges;

  plDeque<Triangle> m_Triangles;
};
