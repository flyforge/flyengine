#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Vec3.h>

/// \brief Provides functions to generate standard geometric shapes, such as boxes, spheres, cylinders, etc.
///
/// This class provides simple functions to create frequently used basic shapes. It allows to transform the shapes, merge them
/// into a single mesh, compute normals, etc.
/// It is meant for debug and editor geometry (gizmos, etc.). Vertices can have position, normal, color and 'shape index'.
class PLASMA_CORE_DLL plGeometry
{
public:
  /// \brief The data that is stored per vertex.
  struct Vertex
  {
    PLASMA_DECLARE_POD_TYPE();

    plVec3 m_vPosition;
    plVec3 m_vNormal;
    plVec3 m_vTangent;
    float m_fBiTangentSign;
    plVec2 m_vTexCoord;
    plColor m_Color;
    plVec4U16 m_BoneIndices;
    plColorLinearUB m_BoneWeights;

    bool operator<(const Vertex& rhs) const;
    bool operator==(const Vertex& rhs) const;
  };

  /// \brief Each polygon has a face normal and a set of indices, which vertices it references.
  struct Polygon
  {
    // Reverses the order of vertices.
    void FlipWinding();

    plVec3 m_vNormal;
    plHybridArray<plUInt32, 4> m_Vertices;
  };

  /// \brief A line only references two vertices.
  struct Line
  {
    PLASMA_DECLARE_POD_TYPE();

    plUInt32 m_uiStartVertex;
    plUInt32 m_uiEndVertex;
  };

  /// \brief Options shared among all geometry creation functions
  struct GeoOptions
  {
    PLASMA_DECLARE_POD_TYPE();

    GeoOptions() {}
    plColor m_Color = plColor(1, 1, 1, 1);         ///< The color of the entire geometric object
    plMat4 m_Transform = plMat4::MakeIdentity(); ///< An additional transform to apply to the geometry while adding it
    plUInt16 m_uiBoneIndex = 0;                    ///< Which bone should influence this geometry, for single-bone skinning.

    bool IsFlipWindingNecessary() const;
  };

  /// \brief Returns the entire vertex data.
  plDeque<Vertex>& GetVertices() { return m_Vertices; }

  /// \brief Returns the entire polygon data.
  plDeque<Polygon>& GetPolygons() { return m_Polygons; }

  /// \brief Returns the entire line data.
  plDeque<Line>& GetLines() { return m_Lines; }

  /// \brief Returns the entire vertex data.
  const plDeque<Vertex>& GetVertices() const { return m_Vertices; }

  /// \brief Returns the entire polygon data.
  const plDeque<Polygon>& GetPolygons() const { return m_Polygons; }

  /// \brief Returns the entire line data.
  const plDeque<Line>& GetLines() const { return m_Lines; }

  /// \brief Clears all data.
  void Clear();

  /// \brief Adds a vertex, returns the index to the added vertex.
  plUInt32 AddVertex(const plVec3& vPos, const plVec3& vNormal, const plVec2& vTexCoord, const plColor& color, const plVec4U16& boneIndices = plVec4U16::MakeZero(), const plColorLinearUB& boneWeights = plColorLinearUB(255, 0, 0, 0));

  /// \brief Adds a vertex, returns the index to the added vertex. Position and normal are transformed with the given matrix.
  plUInt32 AddVertex(const plVec3& vPos, const plVec3& vNormal, const plVec2& vTexCoord, const plColor& color, const plVec4U16& boneIndices, const plColorLinearUB& boneWeights, const plMat4& mTransform)
  {
    return AddVertex(mTransform.TransformPosition(vPos), mTransform.TransformDirection(vNormal).GetNormalized(), vTexCoord, color, boneIndices, boneWeights);
  }

  /// \brief Adds a vertex with a single bone index, returns the index to the added vertex. Position and normal are transformed with the given matrix.
  plUInt32 AddVertex(const plVec3& vPos, const plVec3& vNormal, const plVec2& vTexCoord, const plColor& color, const plUInt16 boneIndex, const plMat4& mTransform)
  {
    return AddVertex(mTransform.TransformPosition(vPos), mTransform.TransformDirection(vNormal).GetNormalized(), vTexCoord, color, plVec4U16(boneIndex, 0, 0, 0), plColorLinearUB(255, 0, 0, 0));
  }

  /// \brief Adds a vertex with a single bone index, returns the index to the added vertex. Position and normal are transformed with the given matrix.
  plUInt32 AddVertex(const plVec3& vPos, const plVec3& vNormal, const plVec2& vTexCoord, const GeoOptions& options)
  {
    return AddVertex(options.m_Transform.TransformPosition(vPos), options.m_Transform.TransformDirection(vNormal).GetNormalized(), vTexCoord, options.m_Color, plVec4U16(options.m_uiBoneIndex, 0, 0, 0), plColorLinearUB(255, 0, 0, 0));
  }

  /// \brief Adds a polygon that consists of all the referenced vertices. No face normal is computed at this point.
  void AddPolygon(const plArrayPtr<plUInt32>& Vertices, bool bFlipWinding);

  /// \brief Adds a line.
  void AddLine(plUInt32 uiStartVertex, plUInt32 uiEndVertex);

  /// \brief Triangulates all polygons that have more than \a uiMaxVerticesInPolygon vertices.
  ///
  /// Set \a uiMaxVerticesInPolygon to 4, if you want to keep quads unchanged.
  void TriangulatePolygons(plUInt32 uiMaxVerticesInPolygon = 3);

  /// \brief Computes normals for all polygons from the current vertex positions. Call this when you do not intend to make further
  /// modifications.
  void ComputeFaceNormals();

  /// \brief Computes smooth (averaged) normals for each vertex. Requires that face normals are computed.
  ///
  /// This only yields smooth normals for vertices that are shared among multiple polygons, otherwise a vertex will have the same normal
  /// as the one face that it is used in.
  void ComputeSmoothVertexNormals();

  /// \brief Computes tangents. This function can increase or reduce vertex count.
  ///
  /// The tangent generation is done by Morten S. Mikkelsen's tangent space generation code.
  void ComputeTangents();

  /// \brief Checks if present tangents are meaningful and resetting them if necessary
  ///
  /// Checks if the tangents are approximately orthogonal to the vertex normal and
  /// of unit length. If this is not the case the respective tangent will be zeroed.
  /// The caller can provide a custom floating point comparison epsilon
  void ValidateTangents(float epsilon = 0.01f);

  /// \brief Returns the number of triangles that the polygons are made up of
  plUInt32 CalculateTriangleCount() const;

  /// \brief Changes the bone indices for all vertices (starting at vertex \a uiFirstVertex).
  void SetAllVertexBoneIndices(const plVec4U16& boneIndices, plUInt32 uiFirstVertex = 0);

  /// \brief Changes the color for all vertices (starting at vertex \a uiFirstVertex).
  void SetAllVertexColor(const plColor& color, plUInt32 uiFirstVertex = 0);

  /// \brief Changes the texture coordinates for all vertices (starting at vertex \a uiFirstVertex).
  void SetAllVertexTexCoord(const plVec2& texCoord, plUInt32 uiFirstVertex = 0);

  /// \brief Transforms all vertices by the given transform.
  ///
  /// When \a bTransformPolyNormals is true, the polygon normals are transformed, as well.
  /// Set this to false when face normals are going to be computed later anyway.
  void Transform(const plMat4& mTransform, bool bTransformPolyNormals);

  /// \brief Merges the given mesh into this one. Use this to composite multiple shapes into one.
  void Merge(const plGeometry& other);

  /// \brief Adds a rectangle shape in the XY plane, with the front in positive Z direction.
  /// It is centered at the origin, extending half size.x and half size.y into direction +X, -X, +Y and -Y.
  /// Optionally tessellates the rectangle for more detail.
  void AddRectXY(const plVec2& size, plUInt32 uiTesselationX = 1, plUInt32 uiTesselationY = 1, const GeoOptions& options = GeoOptions());

  /// \brief Adds a box.
  /// If bExtraVerticesForTexturing is false, 8 shared vertices are added.
  /// If bExtraVerticesForTexturing is true, 24 separate vertices with UV coordinates are added.
  void AddBox(const plVec3& vFullExtents, bool bExtraVerticesForTexturing, const GeoOptions& options = GeoOptions());

  /// \brief Adds box out of lines (8 vertices).
  void AddLineBox(const plVec3& size, const GeoOptions& options = GeoOptions());

  /// \brief Adds the 8 corners of a box as lines.
  ///
  /// fCornerFraction must be between 1.0 and 0.0, with 1 making it a completely closed box and 0 no lines at all.
  void AddLineBoxCorners(const plVec3& size, float fCornerFraction, const GeoOptions& options = GeoOptions());

  /// \brief Adds a pyramid. This is different to a low-res cone in that the corners are placed differently (like on a box).
  ///
  /// The origin is at the center of the base quad.size.z is the height of the pyramid.
  void AddPyramid(const plVec3& size, bool bCap, const GeoOptions& options = GeoOptions());

  /// \brief Adds a geodesic sphere with radius 1 at the origin.
  ///
  /// When uiSubDivisions is zero, the sphere will have 20 triangles.\n
  /// For each subdivision step the number of triangles quadruples.\n
  /// 0 =   20 triangles,   12 vertices\n
  /// 1 =   80 triangles,   42 vertices\n
  /// 2 =  320 triangles,  162 vertices\n
  /// 3 = 1280 triangles,  642 vertices\n
  /// 4 = 5120 triangles, 2562 vertices\n
  /// ...\n
  void AddGeodesicSphere(float fRadius, plUInt8 uiSubDivisions, const GeoOptions& options = GeoOptions());

  /// \brief Adds a cylinder.
  ///
  /// If fPositiveLength == fNegativeLength, the origin is at the center.
  /// If fNegativeLength is zero, the origin is at the bottom and so on.
  ///
  /// uiSegments is the detail around the up axis, must be at least 3.
  /// The top or bottom caps can be removed using \a bCapTop and \a bCapBottom.
  /// When \a fraction is set to any value below 360 degree, a pie / pacman shaped cylinder is created.
  void AddCylinder(float fRadiusTop, float fRadiusBottom, float fPositiveLength, float fNegativeLength, bool bCapTop, bool bCapBottom, plUInt16 uiSegments, const GeoOptions& options = GeoOptions(), plAngle fraction = plAngle::MakeFromDegree(360.0f));

  /// \brief Same as AddCylinder(), but always adds caps and does not generate separate vertices for the caps.
  ///
  /// This is a more compact representation, but does not allow as good texturing.
  void AddCylinderOnePiece(float fRadiusTop, float fRadiusBottom, float fPositiveLength, float fNegativeLength, plUInt16 uiSegments, const GeoOptions& options = GeoOptions());

  /// \brief Adds a cone. The origin is at the center of the bottom.
  ///
  /// uiSegments is the detail around the up axis, must be at least 3.
  void AddCone(float fRadius, float fHeight, bool bCap, plUInt16 uiSegments, const GeoOptions& options = GeoOptions());

  /// \brief Adds a sphere.
  ///
  /// uiSegments is the detail around the up axis, must be at least 3.
  /// uiStacks is the detail of the rounded top and bottom, must be at least 2.
  void AddSphere(float fRadius, plUInt16 uiSegments, plUInt16 uiStacks, const GeoOptions& options = GeoOptions());

  /// \brief Adds half a sphere.
  ///
  /// The origin is at the 'full sphere center', ie. at the center of the cap.
  /// uiSegments is the detail around the up axis, must be at least 3.
  /// uiStacks is the detail of the rounded top and bottom, must be at least 1.
  void AddHalfSphere(float fRadius, plUInt16 uiSegments, plUInt16 uiStacks, bool bCap, const GeoOptions& options = GeoOptions());

  /// \brief Adds a capsule.
  ///
  /// The origin is at the center of the capsule.
  /// Radius and height are added to get the total height of the capsule.
  /// uiSegments is the detail around the up axis, must be at least 3.
  /// uiStacks is the detail of the rounded top and bottom, must be at least 1.
  void AddCapsule(float fRadius, float fHeight, plUInt16 uiSegments, plUInt16 uiStacks, const GeoOptions& options = GeoOptions());

  /// \brief Adds a full torus.
  ///
  /// The origin is at the center of the torus.
  /// \param fInnerRadius is the radius from the center to where the torus ring starts.
  /// \param fOuterRadius is the radius to where the torus ring stops.
  /// \param uiSegments is the detail around the up (Z) axis.
  /// \param uiSegmentDetail is the number of segments around the torus ring (ie. the cylinder detail)
  /// \param bExtraVerticesForTexturing specifies whether the torus should be one closed piece or have additional vertices at the seams, such that texturing works better.
  void AddTorus(float fInnerRadius, float fOuterRadius, plUInt16 uiSegments, plUInt16 uiSegmentDetail, bool bExtraVerticesForTexturing, const GeoOptions& options = GeoOptions());

  /// \brief Adds a ramp that has UV coordinates set.
  void AddTexturedRamp(const plVec3& size, const GeoOptions& options = GeoOptions());

  /// \brief Generates a straight stair mesh along the X axis. The number of steps determines the step height and depth.
  void AddStairs(const plVec3& size, plUInt32 uiNumSteps, plAngle curvature, bool bSmoothSloped, const GeoOptions& options = GeoOptions());

  void AddArch(const plVec3& size, plUInt32 uiNumSegments, float fThickness, plAngle angle, bool bMakeSteps, bool bSmoothBottom, bool bSmoothTop, bool bCapTopAndBottom, const GeoOptions& options = GeoOptions());

  /// \todo GeomUtils improvements:
  // ThickLine
  // Part of a Torus
  // Arc
  // Circle
  // Curved cone (spotlight)
  // flat arc / circle (ie. UE4 gizmo)
  // ....

  // Compounds:
  // Arrow
  // Cross ?
  //


private:
  void TransformVertices(const plMat4& mTransform, plUInt32 uiFirstVertex);

  plDeque<Vertex> m_Vertices;
  plDeque<Polygon> m_Polygons;
  plDeque<Line> m_Lines;
};
