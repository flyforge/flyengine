#include <ModelImporter2/ModelImporterPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <ModelImporter2/ImporterSourceBSP/ImporterSourceBSP.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Logging/Log.h>


namespace SourceBSP
{
  enum LumpTypes
  {
    LUMP_ENTITIES = 0,   // *
    LUMP_PLANES = 1,     // *
    LUMP_TEXDATA = 2,    // *
    LUMP_VERTEXES = 3,   // *
    LUMP_VISIBILITY = 4, // *
    LUMP_NODES = 5,      // *
    LUMP_TEXINFO = 6,    // *
    LUMP_FACES = 7,      // *
    LUMP_LIGHTING = 8,   // *
    LUMP_OCCLUSION = 9,
    LUMP_LEAFS = 10, // *
    LUMP_FACEIDS = 11,
    LUMP_EDGES = 12,       // *
    LUMP_SURFEDGES = 13,   // *
    LUMP_MODELS = 14,      // *
    LUMP_WORLDLIGHTS = 15, //
    LUMP_LEAFFACES = 16,   // *
    LUMP_LEAFBRUSHES = 17, // *
    LUMP_BRUSHES = 18,     // *
    LUMP_BRUSHSIDES = 19,  // *
    LUMP_AREAS = 20,       // *
    LUMP_AREAPORTALS = 21, // *
    LUMP_UNUSED0 = 22,
    LUMP_UNUSED1 = 23,
    LUMP_UNUSED2 = 24,
    LUMP_UNUSED3 = 25,
    LUMP_DISPINFO = 26,
    LUMP_ORIGINALFACES = 27,
    LUMP_PHYSDISP = 28,
    LUMP_PHYSCOLLIDE = 29,
    LUMP_VERTNORMALS = 30,
    LUMP_VERTNORMALINDICES = 31,
    LUMP_DISP_LIGHTMAP_ALPHAS = 32,
    LUMP_DISP_VERTS = 33,                     // CDispVerts
    LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS = 34, // For each displacement
                                              //     For each lightmap sample
                                              //         byte for index
                                              //         if 255, then index = next byte + 255
                                              //         3 bytes for barycentric coordinates
    // The game lump is a method of adding game-specific lumps
    // FIXME: Eventually, all lumps could use the game lump system
    LUMP_GAME_LUMP = 35,
    LUMP_LEAFWATERDATA = 36,
    LUMP_PRIMITIVES = 37,
    LUMP_PRIMVERTS = 38,
    LUMP_PRIMINDICES = 39,
    // A pak file can be embedded in a .bsp now, and the file system will search the pak
    //  file first for any referenced names, before deferring to the game directory
    //  file system/pak files and finally the base directory file system/pak files.
    LUMP_PAKFILE = 40,
    LUMP_CLIPPORTALVERTS = 41,
    // A map can have a number of cubemap entities in it which cause cubemap renders
    // to be taken after running vrad.
    LUMP_CUBEMAPS = 42,
    LUMP_TEXDATA_STRING_DATA = 43,
    LUMP_TEXDATA_STRING_TABLE = 44,
    LUMP_OVERLAYS = 45,
    LUMP_LEAFMINDISTTOWATER = 46,
    LUMP_FACE_MACRO_TEXTURE_INFO = 47,
    LUMP_DISP_TRIS = 48,
    LUMP_PHYSCOLLIDESURFACE = 49, // deprecated.  We no longer use win32-specific Havok compression on terrain
    LUMP_WATEROVERLAYS = 50,
    LUMP_LEAF_AMBIENT_INDEX_HDR = 51, // index of LUMP_LEAF_AMBIENT_LIGHTING_HDR
    LUMP_LEAF_AMBIENT_INDEX = 52,     // index of LUMP_LEAF_AMBIENT_LIGHTING

    // optional lumps for HDR
    LUMP_LIGHTING_HDR = 53,
    LUMP_WORLDLIGHTS_HDR = 54,
    LUMP_LEAF_AMBIENT_LIGHTING_HDR = 55, // NOTE: this data overrides part of the data stored in LUMP_LEAFS.
    LUMP_LEAF_AMBIENT_LIGHTING = 56,     // NOTE: this data overrides part of the data stored in LUMP_LEAFS.

    LUMP_XZIPPAKFILE = 57,   // deprecated. xbox 1: xzip version of pak file
    LUMP_FACES_HDR = 58,     // HDR maps may have different face data.
    LUMP_MAP_FLAGS = 59,     // extended level-wide flags. not present in all levels
    LUMP_OVERLAY_FADES = 60, // Fade distances for overlays
  };

  const int VBSP_HEADER = (('P' << 24) + ('S' << 16) + ('B' << 8) + 'V');

  const int HEADER_LUMPS = 64;

  struct Lump_t
  {
    plInt32 fileofs;
    plInt32 filelen;
    plInt32 version;
    plUInt8 fourCC[4];

    bool isUsed() const { return fileofs != 0 || filelen != 0; }
  };

  struct Header_t
  {
    int ident;
    int version;
    Lump_t lumps[HEADER_LUMPS];
    int mapRevision;
  };

  enum TexInfoFlags_e
  {
    SURF_LIGHT = 0x0001, // value will hold the light strength
    SURF_SKY2D = 0x0002, // don't draw, indicates we should skylight + draw 2d sky but not draw the 3D skybox
    SURF_SKY = 0x0004,   // don't draw, but add to skybox
    SURF_WARP = 0x0008,  // turbulent water warp
    SURF_TRANS = 0x0010,
    SURF_NOPORTAL = 0x0020,  // the surface can not have a portal placed on it
    SURF_TRIGGER = 0x0040,   // FIXME: This is an xbox hack to work around elimination of trigger surfaces, which breaks occluders
    SURF_NODRAW = 0x0080,    // don't bother referencing the texture
    SURF_HINT = 0x0100,      // make a primary bsp splitter
    SURF_SKIP = 0x0200,      // completely ignore, allowing non-closed brushes
    SURF_NOLIGHT = 0x0400,   // Don't calculate light
    SURF_BUMPLIGHT = 0x0800, // calculate three lightmaps for the surface for bumpmapping
    SURF_NOSHADOWS = 0x1000, // Don't receive shadows
    SURF_NODECALS = 0x2000,  // Don't receive decals
    SURF_NOCHOP = 0x4000,    // Don't subdivide patches on this surface
    SURF_HITBOX = 0x8000     // surface is part of a hitbox
  };

  struct Vertex_t
  {
    float x;
    float y;
    float z;

    Vertex_t()
      : x(0)
      , y(0)
      , z(0)
    {
    }

    Vertex_t(float fX, float fY, float fZ)
      : x(fX)
      , y(fY)
      , z(fZ)
    {
    }

    bool operator==(const Vertex_t& other) const
    {
      const float xDiff = x - other.x;
      const float yDiff = y - other.y;
      const float zDiff = z - other.z;

      const float epsilon = 0.01f;

      return plMath::Abs(xDiff) < epsilon && plMath::Abs(yDiff) < epsilon && plMath::Abs(zDiff) < epsilon;
    }

    bool operator!=(const Vertex_t& other) const { return !(*this == other); }
  };

  struct Plane_t
  {
    Vertex_t normal; // normal vector
    float dist;      // distance from origin
    plInt32 type;    // plane axis identifier
  };

  struct Edge_t
  {
    plUInt16 v[2]; // vertex indices
  };

  struct Face_t
  {
    plUInt16 planenum;                      // the plane number
    plUInt8 side;                           // faces opposite to the node's plane direction
    plUInt8 onNode;                         // 1 of on node, 0 if in leaf
    plInt32 firstedge;                      // index into surfedges
    plInt16 numedges;                       // number of surfedges
    plInt16 texinfo;                        // texture info
    plInt16 dispinfo;                       // displacement info
    plInt16 surfaceFogVolumeID;             // ?
    plUInt8 styles[4];                      // switchable lighting info
    plInt32 lightofs;                       // offset into lightmap lump
    float area;                             // face area in units^2
    plInt32 LightmapTextureMinsInLuxels[2]; // texture lighting info
    plInt32 LightmapTextureSizeInLuxels[2]; // texture lighting info
    plInt32 origFace;                       // original face this was split from
    plUInt16 numPrims;                      // primitives
    plUInt16 firstPrimID;
    plUInt32 smoothingGroups; // lightmap smoothing group
  };

  struct TexInfo_t
  {
    float textureVecsTexelsPerWorldUnits[2][4];  // [s/t][xyz offset]
    float lightmapVecsLuxelsPerWorldUnits[2][4]; // [s/t][xyz offset] - length is in units of texels/area
    plInt32 flags;                               // miptex flags + overrides
    plInt32 texdata;                             // Pointer to texture name, size, etc.
  };

  struct TexData_t
  {
    Vertex_t reflectivity;
    plInt32 nameStringTableID;       // index into g_StringTable for the texture name
    plInt32 width, height;           // source image
    plInt32 view_width, view_height; //
  };

  struct Brush_t
  {
    plInt32 firstside;
    plInt32 numsides;
    plInt32 contents;
  };

  struct BrushSide_t
  {
    plUInt16 planenum; // facing out of the leaf
    plInt16 texinfo;
    plInt16 dispinfo; // displacement info (BSPVERSION 7)
    plInt16 bevel;    // is the side a bevel plane? (BSPVERSION 7)
  };

  struct CDispSubNeighbor
  {
    plUInt16 m_iNeighbor; // This indexes into ddispinfos.
                          // 0xFFFF if there is no neighbor here.

    plUInt8 m_NeighborOrientation; // (CCW) rotation of the neighbor wrt this displacement.

    // These use the NeighborSpan type.
    plUInt8 m_Span;         // Where the neighbor fits onto this side of our displacement.
    plUInt8 m_NeighborSpan; // Where we fit onto our neighbor.
  };

  struct CDispNeighbor
  {
    CDispSubNeighbor m_SubNeighbors[2];
  };

  struct CDispCornerNeighbors
  {
    plUInt16 m_Neighbors[4]; // indices of neighbors.
    plUInt8 m_nNeighbors;
  };

  struct DispInfo_t
  {
    Vertex_t startPosition;                  // start position used for orientation
    plInt32 DispVertStart;                   // Index into LUMP_DISP_VERTS.
    plInt32 DispTriStart;                    // Index into LUMP_DISP_TRIS.
    plInt32 power;                           // power - indicates size of surface (2^power	1)
    plInt32 minTess;                         // minimum tesselation allowed
    float smoothingAngle;                    // lighting smoothing angle
    plInt32 contents;                        // surface contents
    plUInt16 MapFace;                        // Which map face this displacement comes from.
    plInt32 LightmapAlphaStart;              // Index into ddisplightmapalpha.
    plInt32 LightmapSamplePositionStart;     // Index into LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS.
    CDispNeighbor EdgeNeighbors[4];          // Indexed by NEIGHBOREDGE_ defines.
    CDispCornerNeighbors CornerNeighbors[4]; // Indexed by CORNER_ defines.
    plUInt32 AllowedVerts[10];               // active verticies
  };

  struct DispVertex_t
  {
    Vertex_t vector;
    float m_distance;
    float m_alpha;
  };

  struct File // NOLINT(*Padding): We don't care about excessive padding here, because this struct is rarely used.
  {
    File(plArrayPtr<plUInt8> memory);

    Header_t* header = nullptr;

    Vertex_t* vertices = nullptr;
    plUInt32 numVertices = 0;

    Plane_t* planes = nullptr;
    plUInt32 numPlanes = 0;

    Edge_t* edges = nullptr;
    plUInt32 numEdges = 0;

    Face_t* faces = nullptr;
    plUInt32 numFaces = 0;

    TexInfo_t* texInfos = nullptr;
    plUInt32 numTexInfos = 0;

    TexData_t* texDatas = nullptr;
    plUInt32 numTexDatas = 0;

    Brush_t* brushes = nullptr;
    plUInt32 numBrushes = 0;

    BrushSide_t* brushSides = nullptr;
    plUInt32 numBrushSides = 0;

    plInt32* surfEdges = nullptr;
    plUInt32 numSurfEdges = 0;


    char* texDataStrings = nullptr;
    plInt32* texDataStringOffsets = nullptr;
    plUInt32 numTexDataStringOffsets = 0;

    char* entityData = nullptr;

    DispInfo_t* dispInfos = nullptr;
    plUInt32 numDispInfos = 0;

    DispVertex_t* dispVertices = nullptr;
    plUInt32 numDispVertices = 0;

    const char* getTexDataString(plUInt32 uiIndex) const;

    bool m_valid = false;
  };

  File::File(plArrayPtr<plUInt8> fileContent)
  {
    if (fileContent.GetCount() < static_cast<plUInt32>(sizeof(Header_t)))
    {
      m_valid = false;
      return;
    }

    plUInt8* memory = fileContent.GetPtr();

    header = reinterpret_cast<Header_t*>(memory);

    if (header->ident != SourceBSP::VBSP_HEADER)
    {
      m_valid = false;
      return;
    }

    vertices = reinterpret_cast<Vertex_t*>(memory + header->lumps[LUMP_VERTEXES].fileofs);
    numVertices = header->lumps[LUMP_VERTEXES].filelen / sizeof(Vertex_t);

    planes = reinterpret_cast<Plane_t*>(memory + header->lumps[LUMP_PLANES].fileofs);
    numPlanes = header->lumps[LUMP_PLANES].filelen / sizeof(Plane_t);

    edges = reinterpret_cast<Edge_t*>(memory + header->lumps[LUMP_EDGES].fileofs);
    numEdges = header->lumps[LUMP_EDGES].filelen / sizeof(Edge_t);

    faces = reinterpret_cast<Face_t*>(memory + header->lumps[LUMP_FACES].fileofs);
    numFaces = header->lumps[LUMP_FACES].filelen / sizeof(Face_t);

    texInfos = reinterpret_cast<TexInfo_t*>(memory + header->lumps[LUMP_TEXINFO].fileofs);
    numTexInfos = header->lumps[LUMP_TEXINFO].filelen / sizeof(TexInfo_t);

    texDatas = reinterpret_cast<TexData_t*>(memory + header->lumps[LUMP_TEXDATA].fileofs);
    numTexDatas = header->lumps[LUMP_TEXDATA].filelen / sizeof(TexData_t);

    brushes = reinterpret_cast<Brush_t*>(memory + header->lumps[LUMP_BRUSHES].fileofs);
    numBrushes = header->lumps[LUMP_BRUSHES].filelen / sizeof(Brush_t);

    brushSides = reinterpret_cast<BrushSide_t*>(memory + header->lumps[LUMP_BRUSHSIDES].fileofs);
    numBrushSides = header->lumps[LUMP_BRUSHSIDES].filelen / sizeof(BrushSide_t);

    surfEdges = reinterpret_cast<plInt32*>(memory + header->lumps[LUMP_SURFEDGES].fileofs);
    numSurfEdges = header->lumps[LUMP_SURFEDGES].filelen / sizeof(plInt32);

    /*
    char* texData;
    i32* texDataOffsets;
    u32 numTexDataOffsets;*/

    texDataStrings = reinterpret_cast<char*>(memory + header->lumps[LUMP_TEXDATA_STRING_DATA].fileofs);
    texDataStringOffsets = reinterpret_cast<plInt32*>(memory + header->lumps[LUMP_TEXDATA_STRING_TABLE].fileofs);
    numTexDataStringOffsets = header->lumps[LUMP_TEXDATA_STRING_TABLE].filelen / sizeof(plInt32);


    entityData = reinterpret_cast<char*>(memory + header->lumps[LUMP_ENTITIES].fileofs);

    dispInfos = reinterpret_cast<DispInfo_t*>(memory + header->lumps[LUMP_DISPINFO].fileofs);
    numDispInfos = header->lumps[LUMP_DISPINFO].filelen / sizeof(DispInfo_t);

    dispVertices = reinterpret_cast<DispVertex_t*>(memory + header->lumps[LUMP_DISP_VERTS].fileofs);
    numDispVertices = header->lumps[LUMP_DISP_VERTS].filelen / sizeof(DispVertex_t);

    m_valid = true;
  }

  const char* File::getTexDataString(plUInt32 uiIndex) const
  {
    PL_ASSERT_ALWAYS(uiIndex < numTexDataStringOffsets, "BSP file tex data string out of bounds.");

    return texDataStrings + texDataStringOffsets[uiIndex];
  }


  void calculateUV(const SourceBSP::Vertex_t& worldPos, const SourceBSP::TexInfo_t& texInfo, const SourceBSP::TexData_t& texData, float& u, float& v)
  {
    u = worldPos.x * texInfo.textureVecsTexelsPerWorldUnits[0][0] + worldPos.y * texInfo.textureVecsTexelsPerWorldUnits[0][1] +
        worldPos.z * texInfo.textureVecsTexelsPerWorldUnits[0][2] + texInfo.textureVecsTexelsPerWorldUnits[0][3];
    v = worldPos.x * texInfo.textureVecsTexelsPerWorldUnits[1][0] + worldPos.y * texInfo.textureVecsTexelsPerWorldUnits[1][1] +
        worldPos.z * texInfo.textureVecsTexelsPerWorldUnits[1][2] + texInfo.textureVecsTexelsPerWorldUnits[1][3];

    u /= texData.width;
    v /= texData.height;
  }

  struct TempVertex
  {
    PL_DECLARE_POD_TYPE();

    float x, y, z;
    float nx, ny, nz;
    float u, v;
  };

  // 1 meter equals 64 units in Source (default player height is 96 units)
  constexpr float bspToMetricScale = 1.0f / 64.0f;

#if 0

  plResult ConvertBSPGeometryToMesh(SourceBSP::File& bspFile, plModelImporter::Mesh* pMesh, plModelImporter::Scene* pScene)
  {
    plMap<plString, plModelImporter::MaterialHandle> alreadyCreatedMaterials;

    plDynamicArray<plUInt32> indices;
    plDynamicArray<TempVertex> vertices;

    for (plUInt32 faceIndex = 0; faceIndex < bspFile.numFaces; ++faceIndex)
    {
      const SourceBSP::Face_t& current = bspFile.faces[faceIndex];

      SourceBSP::Plane_t& plane = bspFile.planes[current.planenum];

      if (current.texinfo == -1)
        continue;

      SourceBSP::TexInfo_t& texInfo = bspFile.texInfos[current.texinfo];
      SourceBSP::TexData_t& texData = bspFile.texDatas[texInfo.texdata];


      // Skip triggers, no draw and sky surfaces
      if ((texInfo.flags & SourceBSP::SURF_NODRAW) || (texInfo.flags & SourceBSP::SURF_SKIP) || (texInfo.flags & SourceBSP::SURF_HINT) ||
          (texInfo.flags & SourceBSP::SURF_HITBOX) || (texInfo.flags & SourceBSP::SURF_SKY) || (texInfo.flags & SourceBSP::SURF_SKY2D) ||
          (texInfo.flags & SourceBSP::SURF_TRIGGER))
        continue;


      const char* material = bspFile.getTexDataString(texInfo.texdata);

      if (!strcmp(material, "TOOLS/TOOLSCLIP"))
        continue;

      // TODO: Consider importing this for collision geometry
      if (!strcmp(material, "TOOLS/TOOLSPLAYERCLIP"))
        continue;

      if (!strcmp(material, "TOOLS/FOGVOLUME"))
        continue;

      plModelImporter::MaterialHandle materialHandle;

      if (alreadyCreatedMaterials.Contains(material))
      {
        materialHandle = *alreadyCreatedMaterials.GetValue(material);
      }
      else
      {
        plUniquePtr<plModelImporter::Material> newMat(PL_DEFAULT_NEW(plModelImporter::Material));
        newMat->m_Name = material;

        materialHandle = pScene->AddMaterial(std::move(newMat));

        alreadyCreatedMaterials.Insert(material, materialHandle);
      }

      plUInt32 partFirstIndex = vertices.GetCount();
      plUInt32 indexOffset = indices.GetCount();

      // No displacement surface? Create a bunch of triangles from the edges
      if (current.dispinfo == -1)
      {
        // Build vertices
        for (plInt32 edgeIndex = current.firstedge; edgeIndex < current.firstedge + (current.numedges - 1); ++edgeIndex)
        {
          plInt32 surfEdge = bspFile.surfEdges[edgeIndex];

          SourceBSP::Edge_t& edge = bspFile.edges[(surfEdge < 0) ? -surfEdge : surfEdge];

          SourceBSP::Vertex_t& v1 = bspFile.vertices[edge.v[(surfEdge < 0) ? 1 : 0]];
          SourceBSP::Vertex_t& v2 = bspFile.vertices[edge.v[(surfEdge < 0) ? 0 : 1]];

          TempVertex vert;
          vert.x = v1.x * bspToMetricScale;
          vert.y = v1.z * bspToMetricScale;
          vert.z = v1.y * bspToMetricScale;
          vert.nx = plane.normal.x;
          vert.ny = plane.normal.z;
          vert.nz = plane.normal.y;

          calculateUV(v1, texInfo, texData, vert.u, vert.v);

          // Only add first vertex for first edge, other edges reuse the previous vertex
          if (edgeIndex == current.firstedge)
          {
            vertices.PushBack(vert);
          }

          vert.x = v2.x * bspToMetricScale;
          vert.y = v2.z * bspToMetricScale;
          vert.z = v2.y * bspToMetricScale;
          calculateUV(v2, texInfo, texData, vert.u, vert.v);

          vertices.PushBack(vert);
        }

        // Build indices for face
        for (plInt32 edge = 1; edge < current.numedges - 1; ++edge)
        {
          indices.PushBack(partFirstIndex);
          indices.PushBack(partFirstIndex + edge);
          indices.PushBack(partFirstIndex + edge + 1);
        }

        plModelImporter::SubMesh subMesh;
        subMesh.m_Material = materialHandle;
        subMesh.m_uiFirstTriangle = indexOffset / 3;
        subMesh.m_uiTriangleCount = (current.numedges - 2);

        pMesh->AddSubMesh(subMesh);
      }
      else
      {
        SourceBSP::DispInfo_t& displacementInfo = bspFile.dispInfos[current.dispinfo];

        // This should never happen - but just to be on the safe side
        if (current.numedges != 4)
          continue;

        // Each edge has 2 ^ power + 1 vertices
        plUInt32 numVerticesPerEdge = ((1u << displacementInfo.power) + 1);
        plUInt32 numVertices = numVerticesPerEdge * numVerticesPerEdge;

        // Get the corner vertices for the original face
        // This is the base of the displacement vertices later on
        SourceBSP::Vertex_t cornerVertices[4];

        for (plInt32 edge = 0; edge < 4; ++edge)
        {
          plInt32 surfEdge = bspFile.surfEdges[current.firstedge + edge];
          SourceBSP::Edge_t& dispEdge = bspFile.edges[(surfEdge < 0) ? -surfEdge : surfEdge];

          cornerVertices[edge] = bspFile.vertices[dispEdge.v[(surfEdge < 0) ? 1 : 0]];
        }

        // If the first corner vertex is not equal to the start position we need to search the original corner vertex
        if (cornerVertices[0] != displacementInfo.startPosition)
        {
          plInt32 offset = 0;
          for (plInt32 vertIdx = 1; vertIdx < 4; ++vertIdx)
          {
            if (cornerVertices[vertIdx] == displacementInfo.startPosition)
            {
              offset = vertIdx;
              break;
            }
          }

          SourceBSP::Vertex_t origCornerVertices[4];
          plMemoryUtils::Copy(origCornerVertices, cornerVertices, 4);

          for (plInt32 vertIdx = 0; vertIdx < 4; ++vertIdx)
          {
            cornerVertices[vertIdx] = origCornerVertices[(vertIdx + offset) % 4];
          }
        }

        plVec3 uvs[4];

        for (int i = 0; i < 4; ++i)
        {
          float u, v;
          calculateUV(cornerVertices[i], texInfo, texData, u, v);
          uvs[i].Set(u, v, 0.0f);
        }

        plVec3 xStep_03 = (plVec3(cornerVertices[3].x, cornerVertices[3].y, cornerVertices[3].z) -
                           plVec3(cornerVertices[0].x, cornerVertices[0].y, cornerVertices[0].z));
        xStep_03 /= float(numVerticesPerEdge - 1);

        plVec3 uvStep_03 = uvs[3] - uvs[1];
        uvStep_03 /= float(numVerticesPerEdge - 1);

        plVec3 xStep_12 = (plVec3(cornerVertices[2].x, cornerVertices[2].y, cornerVertices[2].z) -
                           plVec3(cornerVertices[1].x, cornerVertices[1].y, cornerVertices[1].z));
        xStep_12 /= float(numVerticesPerEdge - 1);

        plVec3 uvStep_12 = uvs[2] - uvs[1];
        uvStep_12 /= float(numVerticesPerEdge - 1);

        for (plUInt32 y = 0; y < numVerticesPerEdge; ++y)
        {
          for (plUInt32 x = 0; x < numVerticesPerEdge; ++x)
          {
            plVec3 x03 = plVec3(cornerVertices[0].x, cornerVertices[0].y, cornerVertices[0].z) + xStep_03 * (float)x;
            plVec3 x12 = plVec3(cornerVertices[1].x, cornerVertices[1].y, cornerVertices[1].z) + xStep_12 * (float)x;

            plVec3 currentBase = x03 + ((x12 - x03) * ((float)y / (numVerticesPerEdge - 1)));

            plUInt32 linearIndex = y * numVerticesPerEdge + x;

            plVec3 offset = plVec3(bspFile.dispVertices[displacementInfo.DispVertStart + linearIndex].vector.x,
              bspFile.dispVertices[displacementInfo.DispVertStart + linearIndex].vector.y,
              bspFile.dispVertices[displacementInfo.DispVertStart + linearIndex].vector.z);
            offset *= bspFile.dispVertices[displacementInfo.DispVertStart + linearIndex].m_distance;

            currentBase += offset;

            plVec3 uv03 = uvs[0] + uvStep_03 * (float)x;
            plVec3 uv12 = uvs[1] + uvStep_12 * (float)x;

            plVec3 uv = uv03 + ((uv12 - uv03) * ((float)y / (numVerticesPerEdge - 1)));

            TempVertex vert;
            vert.x = currentBase.x * bspToMetricScale;
            vert.y = currentBase.z * bspToMetricScale;
            vert.z = currentBase.y * bspToMetricScale;
            vert.nx = plane.normal.x;
            vert.ny = plane.normal.z;
            vert.nz = plane.normal.y;
            vert.u = uv.x;
            vert.v = uv.y;

            vertices.PushBack(vert);
          }
        }

        // Build triangles from these vertices
        for (plUInt32 x = 1; x < numVerticesPerEdge; ++x)
        {
          for (plUInt32 y = 1; y < numVerticesPerEdge; ++y)
          {
            indices.PushBack(y * numVerticesPerEdge + x + partFirstIndex);
            indices.PushBack((y - 1) * numVerticesPerEdge + x + partFirstIndex);
            indices.PushBack((y - 1) * numVerticesPerEdge + (x - 1) + partFirstIndex);

            indices.PushBack(y * numVerticesPerEdge + x + partFirstIndex);
            indices.PushBack((y - 1) * numVerticesPerEdge + (x - 1) + partFirstIndex);
            indices.PushBack(y * numVerticesPerEdge + (x - 1) + partFirstIndex);
          }
        }

        plModelImporter::SubMesh subMesh;
        subMesh.m_Material = materialHandle;
        subMesh.m_uiFirstTriangle = indexOffset / 3;
        subMesh.m_uiTriangleCount = 2 * (numVerticesPerEdge - 1) * (numVerticesPerEdge - 1);

        pMesh->AddSubMesh(subMesh);
      }
    }

    if ((indices.GetCount() % 3) != 0)
    {
      plLog::Error("Index count of BSP import are not divisible by 3, can't build triangles.");
      return PL_FAILURE;
    }

    // Add vertices and triangles
    plHybridArray<plModelImporter::VertexDataStream*, 3> streams;

    auto positionDataStream = pMesh->GetDataStream(plGALVertexAttributeSemantic::Position);
    auto normalDataStream = pMesh->GetDataStream(plGALVertexAttributeSemantic::Normal);
    auto texCoordDataStream = pMesh->GetDataStream(plGALVertexAttributeSemantic::TexCoord0);

    streams.PushBack(positionDataStream);
    streams.PushBack(normalDataStream);
    streams.PushBack(texCoordDataStream);

    for (auto& vertex : vertices)
    {
      positionDataStream->AddValues(plArrayPtr<plUInt8>(reinterpret_cast<plUInt8*>(&vertex.x), 3 * sizeof(float)));
      normalDataStream->AddValues(plArrayPtr<plUInt8>(reinterpret_cast<plUInt8*>(&vertex.nx), 3 * sizeof(float)));
      texCoordDataStream->AddValues(plArrayPtr<plUInt8>(reinterpret_cast<plUInt8*>(&vertex.u), 2 * sizeof(float)));
    }

    pMesh->AddTriangles(indices.GetCount() / 3);

    plArrayPtr<plModelImporter::Mesh::Triangle> triangleList = pMesh->GetTriangles();
    for (plModelImporter::VertexDataStream* stream : streams)
    {
      plUInt32 uiAttributeSize = stream->GetAttributeSize();
      for (plUInt32 i = 0; i < triangleList.GetCount(); ++i)
      {
        stream->SetDataIndex(triangleList[i].m_Vertices[0], indices[i * 3 + 0] * uiAttributeSize);
        stream->SetDataIndex(triangleList[i].m_Vertices[1], indices[i * 3 + 1] * uiAttributeSize);
        stream->SetDataIndex(triangleList[i].m_Vertices[2], indices[i * 3 + 2] * uiAttributeSize);
      }
    }

    return PL_SUCCESS;
  }

#endif
} // namespace SourceBSP


namespace plModelImporter2
{
  ImporterSourceBSP::ImporterSourceBSP() = default;
  ImporterSourceBSP::~ImporterSourceBSP() = default;

  plResult ImporterSourceBSP::DoImport()
  {
    PL_ASSERT_NOT_IMPLEMENTED;
    return PL_FAILURE;

// TODO: adapt BSP import code to new model importer
#if 0

    const char* szFileName = m_Options.m_sSourceFile;

    plDynamicArray<plUInt8> fileContent;
    fileContent.Reserve(1024 * 1024);

    // Read the whole file into memory since we map BSP data structures directly to memory content
    {
      plFileReader fileReader;

      if (fileReader.Open(szFileName, 1024 * 1024).Failed())
      {
        plLog::Error("Couldn't open '{}' for BSP import.", szFileName);
        return PL_FAILURE;
      }

      plUInt8 Temp[1024 * 4];

      while (plUInt64 uiRead = fileReader.ReadBytes(Temp, PL_ARRAY_SIZE(Temp)))
      {
        fileContent.PushBackRange(plArrayPtr<plUInt8>(Temp, (plUInt32)uiRead));
      }
    }

    SourceBSP::File bspFile(fileContent);

    if (!bspFile.m_valid)
    {
      plLog::Error("BSP header not valid for file '{}'.", szFileName);
      return PL_FAILURE;
    }



    // Import the complete BSP geometry as a single mesh
    plSharedPtr<Scene> outScene = PL_DEFAULT_NEW(Scene);
    plUniquePtr<Mesh> mesh(PL_DEFAULT_NEW(Mesh));
    mesh->m_Name = "BSP Geometry";

    mesh->AddDataStream(plGALVertexAttributeSemantic::Position, 3, VertexElementType::FLOAT);
    mesh->AddDataStream(plGALVertexAttributeSemantic::Normal, 3, VertexElementType::FLOAT);
    mesh->AddDataStream(plGALVertexAttributeSemantic::TexCoord0, 2, VertexElementType::FLOAT);

    if (SourceBSP::ConvertBSPGeometryToMesh(bspFile, mesh.Borrow(), outScene.Borrow()).Failed())
    {
      plLog::Error("Couldn't convert BSP geometry to mesh for file '{}'.", szFileName);
      return nullptr;
    }

    // Merge sub meshes with the same materials as the BSP splits would otherwise create enormously
    // many draw calls.
    mesh->MergeSubMeshesWithSameMaterials();

    mesh->ComputeTangents();

    outScene->AddMesh(std::move(mesh));

    return outScene;
#endif
  }
} // namespace plModelImporter2
