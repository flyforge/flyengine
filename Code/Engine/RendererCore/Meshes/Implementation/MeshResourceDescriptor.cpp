#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

plMeshResourceDescriptor::plMeshResourceDescriptor()
{
  m_Bounds = plBoundingBoxSphere::MakeInvalid();
}

void plMeshResourceDescriptor::Clear()
{
  m_Bounds = plBoundingBoxSphere::MakeInvalid();
  m_hMeshBuffer.Invalidate();
  m_Materials.Clear();
  m_MeshBufferDescriptor.Clear();
  m_SubMeshes.Clear();
}

plMeshBufferResourceDescriptor& plMeshResourceDescriptor::MeshBufferDesc()
{
  return m_MeshBufferDescriptor;
}

const plMeshBufferResourceDescriptor& plMeshResourceDescriptor::MeshBufferDesc() const
{
  return m_MeshBufferDescriptor;
}

void plMeshResourceDescriptor::UseExistingMeshBuffer(const plMeshBufferResourceHandle& hBuffer)
{
  m_hMeshBuffer = hBuffer;
}

const plMeshBufferResourceHandle& plMeshResourceDescriptor::GetExistingMeshBuffer() const
{
  return m_hMeshBuffer;
}

plArrayPtr<const plMeshResourceDescriptor::Material> plMeshResourceDescriptor::GetMaterials() const
{
  return m_Materials;
}

plArrayPtr<const plMeshResourceDescriptor::SubMesh> plMeshResourceDescriptor::GetSubMeshes() const
{
  return m_SubMeshes;
}

void plMeshResourceDescriptor::CollapseSubMeshes()
{
  for (plUInt32 idx = 1; idx < m_SubMeshes.GetCount(); ++idx)
  {
    m_SubMeshes[0].m_uiFirstPrimitive = plMath::Min(m_SubMeshes[0].m_uiFirstPrimitive, m_SubMeshes[idx].m_uiFirstPrimitive);
    m_SubMeshes[0].m_uiPrimitiveCount += m_SubMeshes[idx].m_uiPrimitiveCount;

    if (m_SubMeshes[0].m_Bounds.IsValid() && m_SubMeshes[idx].m_Bounds.IsValid())
    {
      m_SubMeshes[0].m_Bounds.ExpandToInclude(m_SubMeshes[idx].m_Bounds);
    }
  }

  m_SubMeshes.SetCount(1);
  m_SubMeshes[0].m_uiMaterialIndex = 0;

  m_Materials.SetCount(1);
}

const plBoundingBoxSphere& plMeshResourceDescriptor::GetBounds() const
{
  return m_Bounds;
}

void plMeshResourceDescriptor::AddSubMesh(plUInt32 uiPrimitiveCount, plUInt32 uiFirstPrimitive, plUInt32 uiMaterialIndex)
{
  SubMesh p;
  p.m_uiFirstPrimitive = uiFirstPrimitive;
  p.m_uiPrimitiveCount = uiPrimitiveCount;
  p.m_uiMaterialIndex = uiMaterialIndex;
  p.m_Bounds = plBoundingBoxSphere::MakeInvalid();

  m_SubMeshes.PushBack(p);
}

void plMeshResourceDescriptor::SetMaterial(plUInt32 uiMaterialIndex, const char* szPathToMaterial)
{
  m_Materials.EnsureCount(uiMaterialIndex + 1);

  m_Materials[uiMaterialIndex].m_sPath = szPathToMaterial;
}

plResult plMeshResourceDescriptor::Save(const char* szFile)
{
  PL_LOG_BLOCK("plMeshResourceDescriptor::Save", szFile);

  plFileWriter file;
  if (file.Open(szFile, 1024 * 1024).Failed())
  {
    plLog::Error("Failed to open file '{0}'", szFile);
    return PL_FAILURE;
  }

  Save(file);
  return PL_SUCCESS;
}

void plMeshResourceDescriptor::Save(plStreamWriter& inout_stream)
{
  plUInt8 uiVersion = 7;
  inout_stream << uiVersion;

  plUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  plCompressedStreamWriterZstd compressor(&inout_stream, 0, plCompressedStreamWriterZstd::Compression::Average);
  plChunkStreamWriter chunk(compressor);
#else
  plChunkStreamWriter chunk(stream);
#endif

  inout_stream << uiCompressionMode;

  chunk.BeginStream(1);

  {
    chunk.BeginChunk("Materials", 1);

    // number of materials
    chunk << m_Materials.GetCount();

    // each material
    for (plUInt32 idx = 0; idx < m_Materials.GetCount(); ++idx)
    {
      chunk << idx;                      // Material Index
      chunk << m_Materials[idx].m_sPath; // Material Path (data directory relative)
      /// \todo Material Path (relative to mesh file)
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("SubMeshes", 1);

    // number of sub-meshes
    chunk << m_SubMeshes.GetCount();

    for (plUInt32 idx = 0; idx < m_SubMeshes.GetCount(); ++idx)
    {
      chunk << idx;                                // Sub-Mesh index
      chunk << m_SubMeshes[idx].m_uiMaterialIndex; // The material to use
      chunk << m_SubMeshes[idx].m_uiFirstPrimitive;
      chunk << m_SubMeshes[idx].m_uiPrimitiveCount;
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("MeshInfo", 4);

    // Number of vertices
    chunk << m_MeshBufferDescriptor.GetVertexCount();

    // Number of triangles
    chunk << m_MeshBufferDescriptor.GetPrimitiveCount();

    // Whether any index buffer is used
    chunk << m_MeshBufferDescriptor.HasIndexBuffer();

    // Whether the indices are 16 or 32 Bit, always false, if no index buffer is used
    chunk << (m_MeshBufferDescriptor.HasIndexBuffer() && m_MeshBufferDescriptor.Uses32BitIndices());

    // Number of vertex streams
    chunk << m_MeshBufferDescriptor.GetVertexDeclaration().m_VertexStreams.GetCount();

    // Version 3: Topology
    chunk << (plUInt8)m_MeshBufferDescriptor.GetTopology();

    for (plUInt32 idx = 0; idx < m_MeshBufferDescriptor.GetVertexDeclaration().m_VertexStreams.GetCount(); ++idx)
    {
      const auto& vs = m_MeshBufferDescriptor.GetVertexDeclaration().m_VertexStreams[idx];

      chunk << idx; // Vertex stream index
      chunk << (plInt32)vs.m_Format;
      chunk << (plInt32)vs.m_Semantic;
      chunk << vs.m_uiElementSize; // not needed, but can be used to check that memory layout has not changed
      chunk << vs.m_uiOffset;      // not needed, but can be used to check that memory layout has not changed
    }

    // Version 2
    if (!m_Bounds.IsValid())
    {
      ComputeBounds();
    }

    chunk << m_Bounds.m_vCenter;
    chunk << m_Bounds.m_vBoxHalfExtends;
    chunk << m_Bounds.m_fSphereRadius;
    // Version 4
    chunk << m_fMaxBoneVertexOffset;

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("VertexBuffer", 1);

    // size in bytes
    chunk << m_MeshBufferDescriptor.GetVertexBufferData().GetCount();

    if (!m_MeshBufferDescriptor.GetVertexBufferData().IsEmpty())
    {
      chunk.WriteBytes(m_MeshBufferDescriptor.GetVertexBufferData().GetData(), m_MeshBufferDescriptor.GetVertexBufferData().GetCount()).IgnoreResult();
    }

    chunk.EndChunk();
  }

  // always write the index buffer chunk, even if it is empty
  {
    chunk.BeginChunk("IndexBuffer", 1);

    // size in bytes
    chunk << m_MeshBufferDescriptor.GetIndexBufferData().GetCount();

    if (!m_MeshBufferDescriptor.GetIndexBufferData().IsEmpty())
    {
      chunk.WriteBytes(m_MeshBufferDescriptor.GetIndexBufferData().GetData(), m_MeshBufferDescriptor.GetIndexBufferData().GetCount()).IgnoreResult();
    }

    chunk.EndChunk();
  }

  if (!m_Bones.IsEmpty())
  {
    chunk.BeginChunk("BindPose", 1);

    chunk.WriteHashTable(m_Bones).IgnoreResult();

    chunk.EndChunk();
  }

  if (m_hDefaultSkeleton.IsValid())
  {
    chunk.BeginChunk("Skeleton", 1);

    chunk << m_hDefaultSkeleton;

    chunk.EndChunk();
  }

  chunk.EndStream();

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  compressor.FinishCompressedStream().IgnoreResult();

  plLog::Dev("Compressed mesh data from {0} KB to {1} KB ({2}%%)", plArgF((float)compressor.GetUncompressedSize() / 1024.0f, 1), plArgF((float)compressor.GetCompressedSize() / 1024.0f, 1), plArgF(100.0f * compressor.GetCompressedSize() / compressor.GetUncompressedSize(), 1));
#endif
}

plResult plMeshResourceDescriptor::Load(const char* szFile)
{
  PL_LOG_BLOCK("plMeshResourceDescriptor::Load", szFile);

  plFileReader file;
  if (file.Open(szFile, 1024 * 1024).Failed())
  {
    plLog::Error("Failed to open file '{0}'", szFile);
    return PL_FAILURE;
  }

  // skip asset header
  plAssetFileHeader assetHeader;
  PL_SUCCEED_OR_RETURN(assetHeader.Read(file));

  return Load(file);
}

plResult plMeshResourceDescriptor::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  // version 4 and below is broken
  if (uiVersion <= 4)
    return PL_FAILURE;

  plUInt8 uiCompressionMode = 0;
  if (uiVersion >= 6)
  {
    inout_stream >> uiCompressionMode;
  }

  plStreamReader* pCompressor = &inout_stream;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  plCompressedStreamReaderZstd decompressorZstd;
#endif

  switch (uiCompressionMode)
  {
    case 0:
      break;

    case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      decompressorZstd.SetInputStream(&inout_stream);
      pCompressor = &decompressorZstd;
      break;
#else
      plLog::Error("Mesh is compressed with zstandard, but support for this compressor is not compiled in.");
      return PL_FAILURE;
#endif

    default:
      plLog::Error("Mesh is compressed with an unknown algorithm.");
      return PL_FAILURE;
  }

  plChunkStreamReader chunk(*pCompressor);
  chunk.BeginStream();

  plUInt32 count;
  bool bHasIndexBuffer = false;
  bool b32BitIndices = false;
  bool bCalculateBounds = true;

  while (chunk.GetCurrentChunk().m_bValid)
  {
    const auto& ci = chunk.GetCurrentChunk();

    if (ci.m_sChunkName == "Materials")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        plLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return PL_FAILURE;
      }

      // number of materials
      chunk >> count;
      m_Materials.SetCount(count);

      // each material
      for (plUInt32 i = 0; i < m_Materials.GetCount(); ++i)
      {
        plUInt32 idx;
        chunk >> idx;                      // Material Index
        chunk >> m_Materials[idx].m_sPath; // Material Path (data directory relative)
        /// \todo Material Path (relative to mesh file)
      }
    }

    if (chunk.GetCurrentChunk().m_sChunkName == "SubMeshes")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        plLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return PL_FAILURE;
      }

      // number of sub-meshes
      chunk >> count;
      m_SubMeshes.SetCount(count);

      for (plUInt32 i = 0; i < m_SubMeshes.GetCount(); ++i)
      {
        plUInt32 idx;
        chunk >> idx;                                // Sub-Mesh index
        chunk >> m_SubMeshes[idx].m_uiMaterialIndex; // The material to use
        chunk >> m_SubMeshes[idx].m_uiFirstPrimitive;
        chunk >> m_SubMeshes[idx].m_uiPrimitiveCount;

        /// \todo load from file
        m_SubMeshes[idx].m_Bounds = plBoundingBoxSphere::MakeInvalid();
      }
    }

    if (ci.m_sChunkName == "MeshInfo")
    {
      if (ci.m_uiChunkVersion > 4)
      {
        plLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return PL_FAILURE;
      }

      // Number of vertices
      plUInt32 uiVertexCount = 0;
      chunk >> uiVertexCount;

      // Number of primitives
      plUInt32 uiPrimitiveCount = 0;
      chunk >> uiPrimitiveCount;

      // Whether any index buffer is used
      chunk >> bHasIndexBuffer;

      // Whether the indices are 16 or 32 Bit, always false, if no index buffer is used
      chunk >> b32BitIndices;

      // Number of vertex streams
      plUInt32 uiStreamCount = 0;
      chunk >> uiStreamCount;

      plUInt8 uiTopology = plGALPrimitiveTopology::Triangles;
      if (ci.m_uiChunkVersion >= 3)
      {
        chunk >> uiTopology;
      }

      for (plUInt32 i = 0; i < uiStreamCount; ++i)
      {
        plUInt32 idx;
        chunk >> idx; // Vertex stream index
        PL_ASSERT_DEV(idx == i, "Invalid stream index ({0}) in file (should be {1})", idx, i);

        plInt32 iFormat, iSemantic;
        plUInt16 uiElementSize, uiOffset;

        chunk >> iFormat;
        chunk >> iSemantic;
        chunk >> uiElementSize; // not needed, but can be used to check that memory layout has not changed
        chunk >> uiOffset;      // not needed, but can be used to check that memory layout has not changed

        if (uiVersion < 7)
        {
          // plGALVertexAttributeSemantic got new elements inserted
          // need to adjust old file formats accordingly

          if (iSemantic >= plGALVertexAttributeSemantic::Color2) // should be plGALVertexAttributeSemantic::TexCoord0 instead
          {
            iSemantic += 6;
          }
        }

        m_MeshBufferDescriptor.AddStream((plGALVertexAttributeSemantic::Enum)iSemantic, (plGALResourceFormat::Enum)iFormat);
      }

      m_MeshBufferDescriptor.AllocateStreams(uiVertexCount, (plGALPrimitiveTopology::Enum)uiTopology, uiPrimitiveCount);

      // Version 2
      if (ci.m_uiChunkVersion >= 2)
      {
        bCalculateBounds = false;
        chunk >> m_Bounds.m_vCenter;
        chunk >> m_Bounds.m_vBoxHalfExtends;
        chunk >> m_Bounds.m_fSphereRadius;
      }
      if (ci.m_uiChunkVersion >= 4)
      {
        chunk >> m_fMaxBoneVertexOffset;
      }
    }

    if (ci.m_sChunkName == "VertexBuffer")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        plLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return PL_FAILURE;
      }

      // size in bytes
      chunk >> count;
      m_MeshBufferDescriptor.GetVertexBufferData().SetCountUninitialized(count);

      if (!m_MeshBufferDescriptor.GetVertexBufferData().IsEmpty())
        chunk.ReadBytes(m_MeshBufferDescriptor.GetVertexBufferData().GetData(), m_MeshBufferDescriptor.GetVertexBufferData().GetCount());
    }

    if (ci.m_sChunkName == "IndexBuffer")
    {
      if (ci.m_uiChunkVersion != 1)
      {
        plLog::Error("Version of chunk '{0}' is invalid ({1})", ci.m_sChunkName, ci.m_uiChunkVersion);
        return PL_FAILURE;
      }

      // size in bytes
      chunk >> count;
      m_MeshBufferDescriptor.GetIndexBufferData().SetCountUninitialized(count);

      if (!m_MeshBufferDescriptor.GetIndexBufferData().IsEmpty())
        chunk.ReadBytes(m_MeshBufferDescriptor.GetIndexBufferData().GetData(), m_MeshBufferDescriptor.GetIndexBufferData().GetCount());
    }

    if (ci.m_sChunkName == "BindPose")
    {
      PL_SUCCEED_OR_RETURN(chunk.ReadHashTable(m_Bones));
    }

    if (ci.m_sChunkName == "Skeleton")
    {
      chunk >> m_hDefaultSkeleton;
    }

    chunk.NextChunk();
  }

  chunk.EndStream();

  if (bCalculateBounds)
  {
    ComputeBounds();

    auto b = m_Bounds;
    plLog::Info("Calculated Bounds: {0} | {1} | {2} - {3} | {4} | {5}", plArgF(b.m_vCenter.x, 2), plArgF(b.m_vCenter.y, 2), plArgF(b.m_vCenter.z, 2), plArgF(b.m_vBoxHalfExtends.x, 2), plArgF(b.m_vBoxHalfExtends.y, 2), plArgF(b.m_vBoxHalfExtends.z, 2));
  }

  return PL_SUCCESS;
}

void plMeshResourceDescriptor::ComputeBounds()
{
  if (m_hMeshBuffer.IsValid())
  {
    plResourceLock<plMeshBufferResource> pMeshBuffer(m_hMeshBuffer, plResourceAcquireMode::AllowLoadingFallback);
    m_Bounds = pMeshBuffer->GetBounds();
  }
  else
  {
    m_Bounds = m_MeshBufferDescriptor.ComputeBounds();
  }
}

plResult plMeshResourceDescriptor::BoneData::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream << m_GlobalInverseRestPoseMatrix;
  inout_stream << m_uiBoneIndex;

  return PL_SUCCESS;
}

plResult plMeshResourceDescriptor::BoneData::Deserialize(plStreamReader& inout_stream)
{
  inout_stream >> m_GlobalInverseRestPoseMatrix;
  inout_stream >> m_uiBoneIndex;

  return PL_SUCCESS;
}


