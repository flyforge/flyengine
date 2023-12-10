#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#include <RendererCore/Rasterizer/Thirdparty/VectorMath.h>

plMutex plRasterizerObject::s_Mutex;
plMap<plString, plSharedPtr<plRasterizerObject>> plRasterizerObject::s_Objects;

plRasterizerObject::plRasterizerObject() = default;
plRasterizerObject::~plRasterizerObject() = default;

#if PLASMA_ENABLED(PLASMA_RASTERIZER_SUPPORTED)

// needed for plHybridArray below
PLASMA_DEFINE_AS_POD_TYPE(__m128);

void plRasterizerObject::CreateMesh(const plGeometry& geo)
{
  plHybridArray<__m128, 64, plAlignedAllocatorWrapper> vertices;
  vertices.Reserve(geo.GetPolygons().GetCount() * 4);

  Aabb bounds;

  auto addVtx = [&](plVec3 vtxPos) {
    plSimdVec4f v;
    v.Load<4>(vtxPos.GetAsPositionVec4().GetData());
    vertices.PushBack(v.m_v);
  };

  for (const auto& poly : geo.GetPolygons())
  {
    const plUInt32 uiNumVertices = poly.m_Vertices.GetCount();
    plUInt32 uiQuadVtx = 0;

    // ignore complex polygons entirely
    if (uiNumVertices > 4)
      continue;

    for (plUInt32 i = 0; i < uiNumVertices; ++i)
    {
      if (uiQuadVtx == 4)
      {
        // TODO: restart next quad (also flip this one's front face)
        break;
      }

      const plUInt32 vtxIdx = poly.m_Vertices[i];

      addVtx(geo.GetVertices()[vtxIdx].m_vPosition);

      bounds.include(vertices.PeekBack());
      ++uiQuadVtx;
    }

    // if the polygon is a triangle, duplicate the last vertex to make it a degenerate quad
    if (uiQuadVtx == 3)
    {
      vertices.PushBack(vertices.PeekBack());
      ++uiQuadVtx;
    }

    if (uiQuadVtx == 4)
    {
      const plUInt32 n = vertices.GetCount();

      // swap two vertices in the quad to flip the front face (different convention between PLASMA and the rasterizer)
      plMath::Swap(vertices[n - 1], vertices[n - 3]);
    }

    PLASMA_ASSERT_DEV(uiQuadVtx == 4, "Degenerate polygon encountered");
  }

  // pad vertices to 32 for proper alignment during baking
  while (vertices.GetCount() % 32 != 0)
  {
    vertices.PushBack(vertices[0]);
  }

  m_Occluder.bake(vertices.GetData(), vertices.GetCount(), bounds.m_min, bounds.m_max);
}

plSharedPtr<const plRasterizerObject> plRasterizerObject::GetObject(plStringView sUniqueName)
{
  PLASMA_LOCK(s_Mutex);

  auto it = s_Objects.Find(sUniqueName);

  if (it.IsValid())
    return it.Value();

  return nullptr;
}

plSharedPtr<const plRasterizerObject> plRasterizerObject::CreateBox(const plVec3& vFullExtents)
{
  PLASMA_LOCK(s_Mutex);

  plStringBuilder sName;
  sName.Format("Box-{}-{}-{}", vFullExtents.x, vFullExtents.y, vFullExtents.z);

  plSharedPtr<plRasterizerObject>& pObj = s_Objects[sName];

  if (pObj == nullptr)
  {
    pObj = PLASMA_NEW(plFoundation::GetAlignedAllocator(), plRasterizerObject);

    plGeometry geometry;
    geometry.AddBox(vFullExtents, false, {});

    pObj->CreateMesh(geometry);
  }

  return pObj;
}

plSharedPtr<const plRasterizerObject> plRasterizerObject::CreateMesh(plStringView sUniqueName, const plGeometry& geometry)
{
  PLASMA_LOCK(s_Mutex);

  plSharedPtr<plRasterizerObject>& pObj = s_Objects[sUniqueName];

  if (pObj == nullptr)
  {
    pObj = PLASMA_NEW(plFoundation::GetAlignedAllocator(), plRasterizerObject);

    pObj->CreateMesh(geometry);
  }

  return pObj;
}

#else

void plRasterizerObject::CreateMesh(const plGeometry& geo)
{
}

plSharedPtr<const plRasterizerObject> plRasterizerObject::GetObject(plStringView sUniqueName)
{
  return nullptr;
}

plSharedPtr<const plRasterizerObject> plRasterizerObject::CreateBox(const plVec3& vFullExtents)
{
  return nullptr;
}

plSharedPtr<const plRasterizerObject> plRasterizerObject::CreateMesh(plStringView sUniqueName, const plGeometry& geometry)
{
  return nullptr;
}

#endif


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Rasterizer_Implementation_RasterizerObject);
