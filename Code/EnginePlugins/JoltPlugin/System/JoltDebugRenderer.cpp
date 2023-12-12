#include <JoltPlugin/JoltPluginPCH.h>

#include <JoltPlugin/System/JoltDebugRenderer.h>

#ifdef JPH_DEBUG_RENDERER

#  include <Jolt/Renderer/DebugRenderer.h>

void plJoltDebugRenderer::TriangleBatch::AddRef()
{
  ++m_iRefCount;
}

void plJoltDebugRenderer::TriangleBatch::Release()
{
  --m_iRefCount;

  if (m_iRefCount == 0)
  {
    auto* pThis = this;
    PLASMA_DEFAULT_DELETE(pThis);
  }
}

plJoltDebugRenderer::plJoltDebugRenderer()
{
  Initialize();
}

void plJoltDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
{
  auto& l = m_Lines.ExpandAndGetRef();
  l.m_start = plJoltConversionUtils::ToVec3(inFrom);
  l.m_end = plJoltConversionUtils::ToVec3(inTo);
  l.m_startColor = plJoltConversionUtils::ToColor(inColor);
  l.m_endColor = l.m_startColor;
}


void plJoltDebugRenderer::DrawTriangle(JPH::Vec3Arg inV1, JPH::Vec3Arg inV2, JPH::Vec3Arg inV3, JPH::ColorArg inColor)
{
  auto& t = m_Triangles.ExpandAndGetRef();
  t.m_position[0] = plJoltConversionUtils::ToVec3(inV1);
  t.m_position[1] = plJoltConversionUtils::ToVec3(inV2);
  t.m_position[2] = plJoltConversionUtils::ToVec3(inV3);
  t.m_color = plJoltConversionUtils::ToColor(inColor);
}


JPH::DebugRenderer::Batch plJoltDebugRenderer::CreateTriangleBatch(const JPH::DebugRenderer::Triangle* pInTriangles, int iInTriangleCount)
{
  TriangleBatch* pBatch = PLASMA_DEFAULT_NEW(TriangleBatch);
  pBatch->m_Triangles.Reserve(iInTriangleCount);

  for (int i = 0; i < iInTriangleCount; ++i)
  {
    auto& t = pBatch->m_Triangles.ExpandAndGetRef();
    t.m_position[0] = plJoltConversionUtils::ToVec3(pInTriangles[i].mV[0].mPosition);
    t.m_position[1] = plJoltConversionUtils::ToVec3(pInTriangles[i].mV[1].mPosition);
    t.m_position[2] = plJoltConversionUtils::ToVec3(pInTriangles[i].mV[2].mPosition);
    t.m_color = plJoltConversionUtils::ToColor(pInTriangles[i].mV[0].mColor);
  }

  return pBatch;
}


JPH::DebugRenderer::Batch plJoltDebugRenderer::CreateTriangleBatch(const JPH::DebugRenderer::Vertex* pInVertices, int iInVertexCount, const JPH::uint32* pInIndices, int iInIndexCount)
{
  const plUInt32 numTris = iInIndexCount / 3;

  TriangleBatch* pBatch = PLASMA_DEFAULT_NEW(TriangleBatch);
  pBatch->m_Triangles.Reserve(numTris);

  plUInt32 index = 0;

  for (plUInt32 i = 0; i < numTris; ++i)
  {
    auto& t = pBatch->m_Triangles.ExpandAndGetRef();
    t.m_position[0] = plJoltConversionUtils::ToVec3(pInVertices[pInIndices[index + 0]].mPosition);
    t.m_position[1] = plJoltConversionUtils::ToVec3(pInVertices[pInIndices[index + 1]].mPosition);
    t.m_position[2] = plJoltConversionUtils::ToVec3(pInVertices[pInIndices[index + 2]].mPosition);
    t.m_color = plJoltConversionUtils::ToColor(pInVertices[pInIndices[index + 0]].mColor);

    index += 3;
  }

  return pBatch;
}


void plJoltDebugRenderer::DrawGeometry(JPH::Mat44Arg modelMatrix, const JPH::AABox& worldSpaceBounds, float fInLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef& geometry, ECullMode inCullMode /*= ECullMode::CullBackFace*/, ECastShadow inCastShadow /*= ECastShadow::On*/, EDrawMode inDrawMode /*= EDrawMode::Solid*/)
{
  if (geometry == nullptr)
    return;

  plUInt32 uiLod = 0;
  if (geometry->mLODs.size() > 1)
    uiLod = 1;
  if (geometry->mLODs.size() > 2)
    uiLod = 2;

  const TriangleBatch* pBatch = static_cast<const TriangleBatch*>(geometry->mLODs[uiLod].mTriangleBatch.GetPtr());

  const plMat4 trans = reinterpret_cast<const plMat4&>(modelMatrix);
  const plColor color = plJoltConversionUtils::ToColor(inModelColor);

  if (inDrawMode == JPH::DebugRenderer::EDrawMode::Solid)
  {
    m_Triangles.Reserve(m_Triangles.GetCount() + pBatch->m_Triangles.GetCount() * ((inCullMode == JPH::DebugRenderer::ECullMode::Off) ? 2 : 1));

    if (inCullMode == JPH::DebugRenderer::ECullMode::CullBackFace || inCullMode == JPH::DebugRenderer::ECullMode::Off)
    {
      for (plUInt32 t = 0; t < pBatch->m_Triangles.GetCount(); ++t)
      {
        auto& tri = m_Triangles.ExpandAndGetRef();
        tri.m_color = pBatch->m_Triangles[t].m_color * color;
        tri.m_position[0] = trans * pBatch->m_Triangles[t].m_position[0];
        tri.m_position[1] = trans * pBatch->m_Triangles[t].m_position[1];
        tri.m_position[2] = trans * pBatch->m_Triangles[t].m_position[2];
      }
    }

    if (inCullMode == JPH::DebugRenderer::ECullMode::CullFrontFace || inCullMode == JPH::DebugRenderer::ECullMode::Off)
    {
      for (plUInt32 t = 0; t < pBatch->m_Triangles.GetCount(); ++t)
      {
        auto& tri = m_Triangles.ExpandAndGetRef();
        tri.m_color = pBatch->m_Triangles[t].m_color * color;
        tri.m_position[0] = trans * pBatch->m_Triangles[t].m_position[0];
        tri.m_position[1] = trans * pBatch->m_Triangles[t].m_position[2];
        tri.m_position[2] = trans * pBatch->m_Triangles[t].m_position[1];
      }
    }
  }
  else
  {
    m_Lines.Reserve(m_Lines.GetCount() + pBatch->m_Triangles.GetCount() * 3);

    for (plUInt32 t = 0; t < pBatch->m_Triangles.GetCount(); ++t)
    {
      const auto& inTri = pBatch->m_Triangles[t];
      const plColor col = pBatch->m_Triangles[t].m_color * color;

      const plVec3 v0 = trans * inTri.m_position[0];
      const plVec3 v1 = trans * inTri.m_position[1];
      const plVec3 v2 = trans * inTri.m_position[2];

      m_Lines.PushBack({v0, v1, col});
      m_Lines.PushBack({v1, v2, col});
      m_Lines.PushBack({v2, v0, col});
    }
  }
}

#endif


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_JoltDebugRenderer);

