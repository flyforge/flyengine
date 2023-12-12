#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Physics/SurfaceResource.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/MemoryStream.h>
#include <Jolt/Core/StreamIn.h>
#include <Jolt/Physics/Collision/Shape/CompoundShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/Shapes/Implementation/JoltCustomShapeInfo.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <RendererCore/Meshes/CpuMeshResource.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

class plJoltStreamIn : public JPH::StreamIn
{
public:
  plStreamReader* m_pStream = nullptr;
  bool m_bEOF = false;

  virtual void ReadBytes(void* pData, size_t uiInNumBytes) override
  {
    if (m_pStream->ReadBytes(pData, uiInNumBytes) < uiInNumBytes)
      m_bEOF = true;
  }

  virtual bool IsEOF() const override
  {
    return m_bEOF;
  }

  virtual bool IsFailed() const override
  {
    return false;
  }
};

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plJoltMeshResource, 1, plRTTIDefaultAllocator<plJoltMeshResource>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plJoltMeshResource);
// clang-format on

plJoltMeshResource::plJoltMeshResource()
  : plResource(DoUpdate::OnMainThread, 1)
{
  m_Bounds = plBoundingBoxSphere(plVec3::ZeroVector(), plVec3::ZeroVector(), 0);

  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(plJoltMeshResource);
}

plJoltMeshResource::~plJoltMeshResource() = default;

plResourceLoadDesc plJoltMeshResource::UnloadData(Unload WhatToUnload)
{
  for (auto pMesh : m_ConvexMeshesData)
  {
    if (pMesh != nullptr)
    {
      PLASMA_DEFAULT_DELETE(pMesh);
    }
  }

  for (auto pMesh : m_ConvexMeshInstances)
  {
    if (pMesh != nullptr)
    {
      pMesh->Release();
    }
  }

  if (m_pTriangleMeshInstance)
  {
    m_pTriangleMeshInstance->Release();
    m_pTriangleMeshInstance = nullptr;
  }

  m_ConvexMeshesData.Clear();
  m_ConvexMeshInstances.Clear();

  m_TriangleMeshData.Clear();
  m_TriangleMeshData.Compact();

  m_uiNumTriangles = 0;
  m_uiNumVertices = 0;

  // we cannot compute this in UpdateMemoryUsage(), so we only read the data there, therefore we need to update this information here
  /// \todo Compute memory usage
  ModifyMemoryUsage().m_uiMemoryCPU = sizeof(plJoltMeshResource);

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  return res;
}

PLASMA_DEFINE_AS_POD_TYPE(JPH::Vec3);

static void ReadConvexMesh(plStreamReader& inout_stream, plDataBuffer* pBuffer)
{
  plUInt32 uiSize = 0;

  inout_stream >> uiSize;
  pBuffer->SetCountUninitialized(uiSize);
  PLASMA_VERIFY(inout_stream.ReadBytes(pBuffer->GetData(), uiSize) == uiSize, "Reading cooked convex mesh data failed.");
}

static void AddStats(plStreamReader& inout_stream, plUInt32& ref_uiVertices, plUInt32& ref_uiTriangles)
{
  plUInt32 verts = 0, tris = 0;

  inout_stream >> verts;
  inout_stream >> tris;

  ref_uiVertices += verts;
  ref_uiTriangles += tris;
}

plResourceLoadDesc plJoltMeshResource::UpdateContent(plStreamReader* Stream)
{
  PLASMA_LOG_BLOCK("plJoltMeshResource::UpdateContent", GetResourceDescription().GetData());

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  m_uiNumTriangles = 0;
  m_uiNumVertices = 0;

  if (Stream == nullptr)
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    plStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  plAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  plUInt8 uiVersion = 1;
  plUInt8 uiCompressionMode = 0;

  if (AssetHash.GetFileVersion() >= 6) // asset document version, in version 6 the 'resource file format version' was added
  {
    *Stream >> uiVersion;
    *Stream >> uiCompressionMode;
  }

  plStreamReader* pCompressor = Stream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  plCompressedStreamReaderZstd decompressorZstd;
#endif

  switch (uiCompressionMode)
  {
    case 0:
      break;

    case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      decompressorZstd.SetInputStream(Stream);
      pCompressor = &decompressorZstd;
      break;
#else
      plLog::Error("Collision mesh is compressed with zstandard, but support for this compressor is not compiled in.");
      res.m_State = plResourceState::LoadedResourceMissing;
      return res;
#endif

    default:
      plLog::Error("Collision mesh is compressed with an unknown algorithm.");
      res.m_State = plResourceState::LoadedResourceMissing;
      return res;
  }

  // load and create the Jolt mesh
  {
    plChunkStreamReader chunk(*pCompressor);
    chunk.SetEndChunkFileMode(plChunkStreamReader::EndChunkFileMode::JustClose);

    chunk.BeginStream();

    // skip all chunks that we don't know
    while (chunk.GetCurrentChunk().m_bValid)
    {
      if (chunk.GetCurrentChunk().m_sChunkName == "Surfaces")
      {
        plUInt32 uiNumSurfaces = 0;
        chunk >> uiNumSurfaces;

        m_Surfaces.SetCount(uiNumSurfaces);
        plStringBuilder sTemp;

        for (plUInt32 surf = 0; surf < uiNumSurfaces; ++surf)
        {
          chunk >> sTemp;

          m_Surfaces[surf] = plResourceManager::LoadResource<plSurfaceResource>(sTemp);
        }
      }

      if (chunk.GetCurrentChunk().m_sChunkName == "Details")
      {
        chunk >> m_Bounds;
      }

      if (chunk.GetCurrentChunk().m_sChunkName == "TriangleMesh")
      {
        plUInt32 uiBufferSize = 0;
        chunk >> uiBufferSize;

        m_TriangleMeshData.SetCountUninitialized(uiBufferSize);
        chunk.ReadBytes(m_TriangleMeshData.GetData(), uiBufferSize);
        AddStats(chunk, m_uiNumVertices, m_uiNumTriangles);
      }

      if (chunk.GetCurrentChunk().m_sChunkName == "ConvexMesh")
      {
        m_ConvexMeshesData.PushBack(PLASMA_DEFAULT_NEW(plDataBuffer));
        m_ConvexMeshInstances.SetCount(1);
        ReadConvexMesh(chunk, m_ConvexMeshesData.PeekBack());
        AddStats(chunk, m_uiNumVertices, m_uiNumTriangles);
      }

      if (chunk.GetCurrentChunk().m_sChunkName == "ConvexDecompositionMesh")
      {
        plUInt16 uiNumParts = 0;
        chunk >> uiNumParts;

        m_ConvexMeshesData.Reserve(uiNumParts);
        m_ConvexMeshInstances.SetCount(uiNumParts);

        for (plUInt32 i = 0; i < uiNumParts; ++i)
        {
          m_ConvexMeshesData.PushBack(PLASMA_DEFAULT_NEW(plDataBuffer));
          ReadConvexMesh(chunk, m_ConvexMeshesData.PeekBack());
          AddStats(chunk, m_uiNumVertices, m_uiNumTriangles);
        }
      }

      chunk.NextChunk();
    }

    if (m_TriangleMeshData.IsEmpty() && m_ConvexMeshesData.IsEmpty())
    {
      plLog::Error("Could neither find a 'TriangleMesh' chunk, nor a 'ConvexMesh' chunk in the JoltMesh file '{0}'", GetResourceID());
    }

    chunk.EndStream();
  }

  res.m_State = plResourceState::Loaded;
  return res;
}

void plJoltMeshResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plJoltMeshResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;

  out_NewMemoryUsage.m_uiMemoryCPU += m_Surfaces.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryCPU += m_TriangleMeshData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryCPU += m_ConvexMeshesData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryCPU += m_ConvexMeshInstances.GetHeapMemoryUsage();

  for (const auto pConvex : m_ConvexMeshesData)
  {
    if (pConvex)
    {
      out_NewMemoryUsage.m_uiMemoryCPU += pConvex->GetHeapMemoryUsage();
    }
  }
}

PLASMA_RESOURCE_IMPLEMENT_CREATEABLE(plJoltMeshResource, plJoltMeshResourceDescriptor)
{
  // creates just an empty mesh

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  return res;
}

void RetrieveShapeTriangles(const JPH::Shape* pShape, plDynamicArray<plVec3>& ref_positions)
{
  const int iMaxTris = 256;

  plDynamicArray<plVec3> positionsTmp;
  positionsTmp.SetCountUninitialized(iMaxTris * 3);

  JPH::Shape::GetTrianglesContext ctxt;

  pShape->GetTrianglesStart(ctxt, JPH::AABox::sBiggest(), pShape->GetCenterOfMass(), JPH::Quat::sIdentity(), JPH::Vec3(1, 1, 1));

  while (true)
  {
    int found = pShape->GetTrianglesNext(ctxt, iMaxTris, reinterpret_cast<JPH::Float3*>(positionsTmp.GetData()), nullptr);

    ref_positions.PushBackRange(positionsTmp.GetArrayPtr().GetSubArray(0, found * 3));

    if (found == 0)
      return;
  }
}

void RetrieveShapeTriangles(JPH::ShapeSettings* pShapeOpt, plDynamicArray<plVec3>& ref_positions)
{
  auto res = pShapeOpt->Create();

  if (res.HasError())
    return;

  RetrieveShapeTriangles(res.Get(), ref_positions);
}

plCpuMeshResourceHandle plJoltMeshResource::ConvertToCpuMesh() const
{
  plStringBuilder sCpuMeshName = GetResourceID();
  sCpuMeshName.AppendFormat("-({})", GetCurrentResourceChangeCounter());

  plCpuMeshResourceHandle hCpuMesh = plResourceManager::GetExistingResource<plCpuMeshResource>(sCpuMeshName);
  if (hCpuMesh.IsValid())
    return hCpuMesh;

  plMeshResourceDescriptor desc;
  desc.MeshBufferDesc().AddStream(plGALVertexAttributeSemantic::Position, plGALResourceFormat::XYZFloat);

  plDynamicArray<plVec3> positions;
  positions.Reserve(256);

  const plUInt32 uiConvexParts = GetNumConvexParts();
  {
    for (plUInt32 i = 0; i < uiConvexParts; ++i)
    {
      auto pShape = InstantiateConvexPart(i, 0, nullptr, 1);
      RetrieveShapeTriangles(pShape, positions);
      pShape->Release();
    }
  }

  if (m_pTriangleMeshInstance != nullptr || !m_TriangleMeshData.IsEmpty())
  {
    auto pShape = InstantiateTriangleMesh(0, {});
    RetrieveShapeTriangles(pShape, positions);
    pShape->Release();
  }

  if (positions.IsEmpty())
    return {};

  desc.MeshBufferDesc().AllocateStreams(positions.GetCount(), plGALPrimitiveTopology::Triangles);
  desc.MeshBufferDesc().GetVertexBufferData().GetArrayPtr().CopyFrom(positions.GetByteArrayPtr());

  desc.AddSubMesh(desc.MeshBufferDesc().GetPrimitiveCount(), 0, 0);
  desc.ComputeBounds();

  return plResourceManager::GetOrCreateResource<plCpuMeshResource>(sCpuMeshName, std::move(desc), GetResourceDescription());
}

JPH::Shape* plJoltMeshResource::InstantiateTriangleMesh(plUInt64 uiUserData, const plDynamicArray<const plJoltMaterial*>& materials) const
{
  if (m_pTriangleMeshInstance == nullptr)
  {
    PLASMA_ASSERT_DEV(!m_TriangleMeshData.IsEmpty(), "Jolt mesh resource doesn't contain a triangle mesh.");

    plRawMemoryStreamReader memReader(m_TriangleMeshData);

    plJoltStreamIn jStream;
    jStream.m_pStream = &memReader;

    auto shapeRes = JPH::Shape::sRestoreFromBinaryState(jStream);

    if (shapeRes.HasError())
    {
      PLASMA_REPORT_FAILURE("Failed to instantiate Jolt triangle mesh: {}", shapeRes.GetError().c_str());
      return nullptr;
    }

    plHybridArray<JPH::PhysicsMaterialRefC, 32> materials;
    materials.SetCount(m_Surfaces.GetCount());

    for (plUInt32 i = 0; i < m_Surfaces.GetCount(); ++i)
    {
      if (!m_Surfaces[i].IsValid())
        continue;

      plResourceLock pSurf(m_Surfaces[i], plResourceAcquireMode::BlockTillLoaded_NeverFail);

      if (pSurf.GetAcquireResult() != plResourceAcquireResult::None)
      {
        const plJoltMaterial* pMat = static_cast<const plJoltMaterial*>(pSurf->m_pPhysicsMaterialJolt);

        materials[i] = pMat;
      }
      else
      {
        plLog::Warning("Surface resource '{}' not available.", m_Surfaces[i].GetResourceID());
      }
    }

    shapeRes.Get()->RestoreMaterialState(materials.GetData(), materials.GetCount());

    m_pTriangleMeshInstance = shapeRes.Get();
    m_pTriangleMeshInstance->AddRef();

    // not needed anymore
    m_TriangleMeshData.Clear();
    m_TriangleMeshData.Compact();
  }

  {
    plJoltCustomShapeInfo* pShapeDeco = new plJoltCustomShapeInfo(m_pTriangleMeshInstance);
    pShapeDeco->SetUserData(uiUserData);

    // only override the materials, if they differ
    if (materials.GetCount() == m_Surfaces.GetCount())
    {
      pShapeDeco->m_CustomMaterials.SetCount(materials.GetCount());
      for (plUInt32 i = 0; i < materials.GetCount(); ++i)
      {
        pShapeDeco->m_CustomMaterials[i] = materials[i];
      }
    }

    pShapeDeco->AddRef();
    return pShapeDeco;
  }
}

JPH::Shape* plJoltMeshResource::InstantiateConvexPart(plUInt32 uiPartIdx, plUInt64 uiUserData, const plJoltMaterial* pMaterial, float fDensity) const
{
  if (m_ConvexMeshInstances[uiPartIdx] == nullptr)
  {
    PLASMA_ASSERT_DEV(!m_ConvexMeshesData.IsEmpty(), "Jolt mesh resource doesn't contain any convex mesh.");

    plRawMemoryStreamReader memReader(*m_ConvexMeshesData[uiPartIdx]);

    plJoltStreamIn jStream;
    jStream.m_pStream = &memReader;

    auto shapeRes = JPH::Shape::sRestoreFromBinaryState(jStream);

    if (shapeRes.HasError())
    {
      PLASMA_REPORT_FAILURE("Failed to instantiate Jolt triangle mesh: {}", shapeRes.GetError().c_str());
      return nullptr;
    }

    JPH::ConvexShape* pConvexShape = static_cast<JPH::ConvexShape*>(shapeRes.Get().GetPtr());
    pConvexShape->SetDensity(1.0f); // density will be multiplied by the decoration shape, so set the base value to 1

    plHybridArray<JPH::PhysicsMaterialRefC, 1> materials;
    materials.SetCount(m_Surfaces.GetCount());

    for (plUInt32 i = 0; i < m_Surfaces.GetCount(); ++i)
    {
      if (!m_Surfaces[i].IsValid())
        continue;

      plResourceLock pSurf(m_Surfaces[i], plResourceAcquireMode::BlockTillLoaded);
      const plJoltMaterial* pMat = static_cast<const plJoltMaterial*>(pSurf->m_pPhysicsMaterialJolt);

      materials[i] = pMat;
    }


    PLASMA_ASSERT_DEBUG(materials.GetCount() <= 1, "Convex meshes should only have a single material. '{}' has {}", GetResourceDescription(), materials.GetCount());
    shapeRes.Get()->RestoreMaterialState(materials.GetData(), materials.GetCount());


    m_ConvexMeshInstances[uiPartIdx] = shapeRes.Get();
    m_ConvexMeshInstances[uiPartIdx]->AddRef();

    PLASMA_DEFAULT_DELETE(m_ConvexMeshesData[uiPartIdx]);
  }

  {
    plJoltCustomShapeInfo* pShapeDeco = new plJoltCustomShapeInfo(m_ConvexMeshInstances[uiPartIdx]);
    pShapeDeco->SetUserData(uiUserData);
    pShapeDeco->m_fDensity = fDensity;

    if (pMaterial && pMaterial->m_pSurface != nullptr)
    {
      pShapeDeco->m_CustomMaterials.SetCount(1);
      pShapeDeco->m_CustomMaterials[0] = pMaterial;
    }

    pShapeDeco->AddRef();
    return pShapeDeco;
  }
}


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Resources_JoltMeshResource);

