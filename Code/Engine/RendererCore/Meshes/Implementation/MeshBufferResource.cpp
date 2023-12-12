#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMeshBufferResource, 1, plRTTIDefaultAllocator<plMeshBufferResource>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plMeshBufferResource);
// clang-format on

plMeshBufferResourceDescriptor::plMeshBufferResourceDescriptor()
{
  m_Topology = plGALPrimitiveTopology::Triangles;
  m_uiVertexSize = 0;
  m_uiVertexCount = 0;
}

plMeshBufferResourceDescriptor::~plMeshBufferResourceDescriptor() = default;

void plMeshBufferResourceDescriptor::Clear()
{
  m_Topology = plGALPrimitiveTopology::Triangles;
  m_uiVertexSize = 0;
  m_uiVertexCount = 0;
  m_VertexDeclaration.m_uiHash = 0;
  m_VertexDeclaration.m_VertexStreams.Clear();
  m_VertexStreamData.Clear();
  m_IndexBufferData.Clear();
}

plArrayPtr<const plUInt8> plMeshBufferResourceDescriptor::GetVertexBufferData() const
{
  return m_VertexStreamData.GetArrayPtr();
}

plArrayPtr<const plUInt8> plMeshBufferResourceDescriptor::GetIndexBufferData() const
{
  return m_IndexBufferData.GetArrayPtr();
}

plDynamicArray<plUInt8, plAlignedAllocatorWrapper>& plMeshBufferResourceDescriptor::GetVertexBufferData()
{
  PLASMA_ASSERT_DEV(!m_VertexStreamData.IsEmpty(), "The vertex data must be allocated first");
  return m_VertexStreamData;
}

plDynamicArray<plUInt8, plAlignedAllocatorWrapper>& plMeshBufferResourceDescriptor::GetIndexBufferData()
{
  PLASMA_ASSERT_DEV(!m_IndexBufferData.IsEmpty(), "The index data must be allocated first");
  return m_IndexBufferData;
}

plUInt32 plMeshBufferResourceDescriptor::AddStream(plGALVertexAttributeSemantic::Enum semantic, plGALResourceFormat::Enum format)
{
  PLASMA_ASSERT_DEV(m_VertexStreamData.IsEmpty(), "This function can only be called before 'AllocateStreams' is called");

  for (plUInt32 i = 0; i < m_VertexDeclaration.m_VertexStreams.GetCount(); ++i)
  {
    PLASMA_ASSERT_DEV(m_VertexDeclaration.m_VertexStreams[i].m_Semantic != semantic, "The given semantic {0} is already used by a previous stream", semantic);
  }

  plVertexStreamInfo si;

  si.m_Semantic = semantic;
  si.m_Format = format;
  si.m_uiOffset = 0;
  si.m_uiElementSize = static_cast<plUInt16>(plGALResourceFormat::GetBitsPerElement(format) / 8);
  m_uiVertexSize += si.m_uiElementSize;

  PLASMA_ASSERT_DEV(si.m_uiElementSize > 0, "Invalid Element Size. Format not supported?");

  if (!m_VertexDeclaration.m_VertexStreams.IsEmpty())
    si.m_uiOffset = m_VertexDeclaration.m_VertexStreams.PeekBack().m_uiOffset + m_VertexDeclaration.m_VertexStreams.PeekBack().m_uiElementSize;

  m_VertexDeclaration.m_VertexStreams.PushBack(si);

  return m_VertexDeclaration.m_VertexStreams.GetCount() - 1;
}

void plMeshBufferResourceDescriptor::AddCommonStreams()
{
  AddStream(plGALVertexAttributeSemantic::Position, plGALResourceFormat::XYZFloat);
  AddStream(plGALVertexAttributeSemantic::TexCoord0, plMeshTexCoordPrecision::ToResourceFormat(plMeshTexCoordPrecision::Default));
  AddStream(plGALVertexAttributeSemantic::Normal, plMeshNormalPrecision::ToResourceFormatNormal(plMeshNormalPrecision::Default));
  AddStream(plGALVertexAttributeSemantic::Tangent, plMeshNormalPrecision::ToResourceFormatTangent(plMeshNormalPrecision::Default));
}

void plMeshBufferResourceDescriptor::AllocateStreams(plUInt32 uiNumVertices, plGALPrimitiveTopology::Enum topology, plUInt32 uiNumPrimitives, bool bZeroFill /*= false*/)
{
  PLASMA_ASSERT_DEV(!m_VertexDeclaration.m_VertexStreams.IsEmpty(), "You have to add streams via 'AddStream' before calling this function");

  m_Topology = topology;
  m_uiVertexCount = uiNumVertices;
  const plUInt32 uiVertexStreamSize = m_uiVertexSize * uiNumVertices;

  if (bZeroFill)
  {
    m_VertexStreamData.SetCount(uiVertexStreamSize);
  }
  else
  {
    m_VertexStreamData.SetCountUninitialized(uiVertexStreamSize);
  }

  if (uiNumPrimitives > 0)
  {
    // use an index buffer at all
    plUInt32 uiIndexBufferSize = uiNumPrimitives * plGALPrimitiveTopology::VerticesPerPrimitive(topology);

    if (Uses32BitIndices())
    {
      uiIndexBufferSize *= sizeof(plUInt32);
    }
    else
    {
      uiIndexBufferSize *= sizeof(plUInt16);
    }

    m_IndexBufferData.SetCountUninitialized(uiIndexBufferSize);
  }
}

void plMeshBufferResourceDescriptor::AllocateStreamsFromGeometry(const plGeometry& geom, plGALPrimitiveTopology::Enum topology)
{
  plLogBlock _("Allocate Streams From Geometry");

  // Index Buffer Generation
  plDynamicArray<plUInt32> Indices;

  if (topology == plGALPrimitiveTopology::Points)
  {
    // Leaving indices empty disables indexed rendering.
  }
  else if (topology == plGALPrimitiveTopology::Lines)
  {
    Indices.Reserve(geom.GetLines().GetCount() * 2);

    for (plUInt32 p = 0; p < geom.GetLines().GetCount(); ++p)
    {
      Indices.PushBack(geom.GetLines()[p].m_uiStartVertex);
      Indices.PushBack(geom.GetLines()[p].m_uiEndVertex);
    }
  }
  else if (topology == plGALPrimitiveTopology::Triangles)
  {
    Indices.Reserve(geom.GetPolygons().GetCount() * 6);

    for (plUInt32 p = 0; p < geom.GetPolygons().GetCount(); ++p)
    {
      for (plUInt32 v = 0; v < geom.GetPolygons()[p].m_Vertices.GetCount() - 2; ++v)
      {
        Indices.PushBack(geom.GetPolygons()[p].m_Vertices[0]);
        Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 1]);
        Indices.PushBack(geom.GetPolygons()[p].m_Vertices[v + 2]);
      }
    }
  }
  AllocateStreams(geom.GetVertices().GetCount(), topology, Indices.GetCount() / (topology + 1));

  // Fill vertex buffer.
  for (plUInt32 s = 0; s < m_VertexDeclaration.m_VertexStreams.GetCount(); ++s)
  {
    const plVertexStreamInfo& si = m_VertexDeclaration.m_VertexStreams[s];
    switch (si.m_Semantic)
    {
      case plGALVertexAttributeSemantic::Position:
      {
        if (si.m_Format == plGALResourceFormat::XYZFloat)
        {
          for (plUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<plVec3>(s, v, geom.GetVertices()[v].m_vPosition);
          }
        }
        else
        {
          plLog::Error("Position stream with format '{0}' is not supported.", (int)si.m_Format);
        }
      }
      break;

      case plGALVertexAttributeSemantic::Normal:
      {
        for (plUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
        {
          if (plMeshBufferUtils::EncodeNormal(geom.GetVertices()[v].m_vNormal, GetVertexData(s, v), si.m_Format).Failed())
          {
            plLog::Error("Normal stream with format '{0}' is not supported.", (int)si.m_Format);
            break;
          }
        }
      }
      break;

      case plGALVertexAttributeSemantic::Tangent:
      {
        for (plUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
        {
          if (plMeshBufferUtils::EncodeTangent(geom.GetVertices()[v].m_vTangent, geom.GetVertices()[v].m_fBiTangentSign, GetVertexData(s, v), si.m_Format).Failed())
          {
            plLog::Error("Tangent stream with format '{0}' is not supported.", (int)si.m_Format);
            break;
          }
        }
      }
      break;

      case plGALVertexAttributeSemantic::Color0:
      case plGALVertexAttributeSemantic::Color1:
      {
        if (si.m_Format == plGALResourceFormat::RGBAUByteNormalized)
        {
          for (plUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<plColorLinearUB>(s, v, geom.GetVertices()[v].m_Color);
          }
        }
        else
        {
          plLog::Error("Color stream with format '{0}' is not supported.", (int)si.m_Format);
        }
      }
      break;

      case plGALVertexAttributeSemantic::TexCoord0:
      case plGALVertexAttributeSemantic::TexCoord1:
      {
        for (plUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
        {
          if (plMeshBufferUtils::EncodeTexCoord(geom.GetVertices()[v].m_vTexCoord, GetVertexData(s, v), si.m_Format).Failed())
          {
            plLog::Error("UV stream with format '{0}' is not supported.", (int)si.m_Format);
            break;
          }
        }
      }
      break;

      case plGALVertexAttributeSemantic::BoneIndices0:
      {
        // if a bone index array is available, move the custom index into it

        if (si.m_Format == plGALResourceFormat::RGBAUByte)
        {
          for (plUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            plVec4U16 boneIndices = geom.GetVertices()[v].m_BoneIndices;
            plVec4U8 storage(static_cast<plUInt8>(boneIndices.x), static_cast<plUInt8>(boneIndices.y), static_cast<plUInt8>(boneIndices.z), static_cast<plUInt8>(boneIndices.w));
            SetVertexData<plVec4U8>(s, v, storage);
          }
        }
        else if (si.m_Format == plGALResourceFormat::RGBAUShort)
        {
          for (plUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<plVec4U16>(s, v, geom.GetVertices()[v].m_BoneIndices);
          }
        }
      }
      break;

      case plGALVertexAttributeSemantic::BoneWeights0:
      {
        // if a bone weight array is available, set it to fully use the first bone

        if (si.m_Format == plGALResourceFormat::RGBAUByteNormalized)
        {
          for (plUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<plColorLinearUB>(s, v, geom.GetVertices()[v].m_BoneWeights);
          }
        }

        if (si.m_Format == plGALResourceFormat::XYZWFloat)
        {
          for (plUInt32 v = 0; v < geom.GetVertices().GetCount(); ++v)
          {
            SetVertexData<plVec4>(s, v, plColor(geom.GetVertices()[v].m_BoneWeights).GetAsVec4());
          }
        }
      }
      break;

      case plGALVertexAttributeSemantic::BoneIndices1:
      case plGALVertexAttributeSemantic::BoneWeights1:
        // Don't error out for these semantics as they may be used by the user (e.g. breakable mesh construction)
        break;

      default:
      {
        plLog::Error("Streams semantic '{0}' is not supported.", (int)si.m_Semantic);
      }
      break;
    }
  }

  // Fill index buffer.
  if (topology == plGALPrimitiveTopology::Points)
  {
    for (plUInt32 t = 0; t < Indices.GetCount(); t += 1)
    {
      SetPointIndices(t, Indices[t]);
    }
  }
  else if (topology == plGALPrimitiveTopology::Triangles)
  {
    for (plUInt32 t = 0; t < Indices.GetCount(); t += 3)
    {
      SetTriangleIndices(t / 3, Indices[t], Indices[t + 1], Indices[t + 2]);
    }
  }
  else if (topology == plGALPrimitiveTopology::Lines)
  {
    for (plUInt32 t = 0; t < Indices.GetCount(); t += 2)
    {
      SetLineIndices(t / 2, Indices[t], Indices[t + 1]);
    }
  }
}

void plMeshBufferResourceDescriptor::SetPointIndices(plUInt32 uiPoint, plUInt32 uiVertex0)
{
  PLASMA_ASSERT_DEBUG(m_Topology == plGALPrimitiveTopology::Points, "Wrong topology");

  if (Uses32BitIndices())
  {
    plUInt32* pIndices = reinterpret_cast<plUInt32*>(&m_IndexBufferData[uiPoint * sizeof(plUInt32) * 1]);
    pIndices[0] = uiVertex0;
  }
  else
  {
    plUInt16* pIndices = reinterpret_cast<plUInt16*>(&m_IndexBufferData[uiPoint * sizeof(plUInt16) * 1]);
    pIndices[0] = static_cast<plUInt16>(uiVertex0);
  }
}

void plMeshBufferResourceDescriptor::SetLineIndices(plUInt32 uiLine, plUInt32 uiVertex0, plUInt32 uiVertex1)
{
  PLASMA_ASSERT_DEBUG(m_Topology == plGALPrimitiveTopology::Lines, "Wrong topology");

  if (Uses32BitIndices())
  {
    plUInt32* pIndices = reinterpret_cast<plUInt32*>(&m_IndexBufferData[uiLine * sizeof(plUInt32) * 2]);
    pIndices[0] = uiVertex0;
    pIndices[1] = uiVertex1;
  }
  else
  {
    plUInt16* pIndices = reinterpret_cast<plUInt16*>(&m_IndexBufferData[uiLine * sizeof(plUInt16) * 2]);
    pIndices[0] = static_cast<plUInt16>(uiVertex0);
    pIndices[1] = static_cast<plUInt16>(uiVertex1);
  }
}

void plMeshBufferResourceDescriptor::SetTriangleIndices(plUInt32 uiTriangle, plUInt32 uiVertex0, plUInt32 uiVertex1, plUInt32 uiVertex2)
{
  PLASMA_ASSERT_DEBUG(m_Topology == plGALPrimitiveTopology::Triangles, "Wrong topology");

  if (Uses32BitIndices())
  {
    plUInt32* pIndices = reinterpret_cast<plUInt32*>(&m_IndexBufferData[uiTriangle * sizeof(plUInt32) * 3]);
    pIndices[0] = uiVertex0;
    pIndices[1] = uiVertex1;
    pIndices[2] = uiVertex2;
  }
  else
  {
    plUInt16* pIndices = reinterpret_cast<plUInt16*>(&m_IndexBufferData[uiTriangle * sizeof(plUInt16) * 3]);
    pIndices[0] = static_cast<plUInt16>(uiVertex0);
    pIndices[1] = static_cast<plUInt16>(uiVertex1);
    pIndices[2] = static_cast<plUInt16>(uiVertex2);
  }
}

plUInt32 plMeshBufferResourceDescriptor::GetPrimitiveCount() const
{
  const plUInt32 divider = m_Topology + 1;

  if (!m_IndexBufferData.IsEmpty())
  {
    if (Uses32BitIndices())
      return (m_IndexBufferData.GetCount() / sizeof(plUInt32)) / divider;
    else
      return (m_IndexBufferData.GetCount() / sizeof(plUInt16)) / divider;
  }
  else
  {
    return m_uiVertexCount / divider;
  }
}

plBoundingBoxSphere plMeshBufferResourceDescriptor::ComputeBounds() const
{
  plBoundingBoxSphere bounds;
  bounds.SetInvalid();

  for (plUInt32 i = 0; i < m_VertexDeclaration.m_VertexStreams.GetCount(); ++i)
  {
    if (m_VertexDeclaration.m_VertexStreams[i].m_Semantic == plGALVertexAttributeSemantic::Position)
    {
      PLASMA_ASSERT_DEBUG(m_VertexDeclaration.m_VertexStreams[i].m_Format == plGALResourceFormat::XYZFloat, "Position format is not usable");

      const plUInt32 offset = m_VertexDeclaration.m_VertexStreams[i].m_uiOffset;

      if (!m_VertexStreamData.IsEmpty() && m_uiVertexCount > 0)
      {
        bounds.SetFromPoints(reinterpret_cast<const plVec3*>(&m_VertexStreamData[offset]), m_uiVertexCount, m_uiVertexSize);
      }

      return bounds;
    }
  }

  return bounds;
}

plResult plMeshBufferResourceDescriptor::RecomputeNormals()
{
  if (m_Topology != plGALPrimitiveTopology::Triangles)
    return PLASMA_FAILURE; // normals not needed

  const plUInt32 uiVertexSize = m_uiVertexSize;
  const plUInt8* pPositions = nullptr;
  plUInt8* pNormals = nullptr;
  plGALResourceFormat::Enum normalsFormat = plGALResourceFormat::XYZFloat;

  for (plUInt32 i = 0; i < m_VertexDeclaration.m_VertexStreams.GetCount(); ++i)
  {
    if (m_VertexDeclaration.m_VertexStreams[i].m_Semantic == plGALVertexAttributeSemantic::Position && m_VertexDeclaration.m_VertexStreams[i].m_Format == plGALResourceFormat::XYZFloat)
    {
      pPositions = GetVertexData(i, 0).GetPtr();
    }

    if (m_VertexDeclaration.m_VertexStreams[i].m_Semantic == plGALVertexAttributeSemantic::Normal)
    {
      normalsFormat = m_VertexDeclaration.m_VertexStreams[i].m_Format;
      pNormals = GetVertexData(i, 0).GetPtr();
    }
  }

  if (pPositions == nullptr || pNormals == nullptr)
    return PLASMA_FAILURE; // there are no normals that could be recomputed

  plDynamicArray<plVec3> newNormals;
  newNormals.SetCountUninitialized(m_uiVertexCount);

  for (auto& n : newNormals)
  {
    n.SetZero();
  }

  plResult res = PLASMA_SUCCESS;

  const plUInt16* pIndices16 = reinterpret_cast<const plUInt16*>(m_IndexBufferData.GetData());
  const plUInt32* pIndices32 = reinterpret_cast<const plUInt32*>(m_IndexBufferData.GetData());
  const bool bUseIndices32 = Uses32BitIndices();

  // Compute unnormalized triangle normals and add them to all vertices.
  // This way large triangles have an higher influence on the vertex normal.
  for (plUInt32 triIdx = 0; triIdx < GetPrimitiveCount(); ++triIdx)
  {
    const plUInt32 v0 = bUseIndices32 ? pIndices32[triIdx * 3 + 0] : pIndices16[triIdx * 3 + 0];
    const plUInt32 v1 = bUseIndices32 ? pIndices32[triIdx * 3 + 1] : pIndices16[triIdx * 3 + 1];
    const plUInt32 v2 = bUseIndices32 ? pIndices32[triIdx * 3 + 2] : pIndices16[triIdx * 3 + 2];

    const plVec3 p0 = *reinterpret_cast<const plVec3*>(pPositions + plMath::SafeMultiply64(uiVertexSize, v0));
    const plVec3 p1 = *reinterpret_cast<const plVec3*>(pPositions + plMath::SafeMultiply64(uiVertexSize, v1));
    const plVec3 p2 = *reinterpret_cast<const plVec3*>(pPositions + plMath::SafeMultiply64(uiVertexSize, v2));

    const plVec3 d01 = p1 - p0;
    const plVec3 d02 = p2 - p0;

    const plVec3 triNormal = d01.CrossRH(d02);

    if (triNormal.IsValid())
    {
      newNormals[v0] += triNormal;
      newNormals[v1] += triNormal;
      newNormals[v2] += triNormal;
    }
  }

  for (plUInt32 i = 0; i < newNormals.GetCount(); ++i)
  {
    // normalize the new normal
    if (newNormals[i].NormalizeIfNotZero(plVec3::UnitXAxis()).Failed())
      res = PLASMA_FAILURE;

    // then encode it in the target format precision and write it back to the buffer
    PLASMA_SUCCEED_OR_RETURN(plMeshBufferUtils::EncodeNormal(newNormals[i], plByteArrayPtr(pNormals + plMath::SafeMultiply64(uiVertexSize, i), sizeof(plVec3)), normalsFormat));
  }

  return res;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plMeshBufferResource::plMeshBufferResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

plMeshBufferResource::~plMeshBufferResource()
{
  PLASMA_ASSERT_DEBUG(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  PLASMA_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");
}

plResourceLoadDesc plMeshBufferResource::UnloadData(Unload WhatToUnload)
{
  if (!m_hVertexBuffer.IsInvalidated())
  {
    plGALDevice::GetDefaultDevice()->DestroyBuffer(m_hVertexBuffer);
    m_hVertexBuffer.Invalidate();
  }

  if (!m_hIndexBuffer.IsInvalidated())
  {
    plGALDevice::GetDefaultDevice()->DestroyBuffer(m_hIndexBuffer);
    m_hIndexBuffer.Invalidate();
  }

  m_uiPrimitiveCount = 0;

  // we cannot compute this in UpdateMemoryUsage(), so we only read the data there, therefore we need to update this information here
  ModifyMemoryUsage().m_uiMemoryGPU = 0;

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  return res;
}

plResourceLoadDesc plMeshBufferResource::UpdateContent(plStreamReader* Stream)
{
  PLASMA_REPORT_FAILURE("This resource type does not support loading data from file.");

  return plResourceLoadDesc();
}

void plMeshBufferResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // we cannot compute this data here, so we update it wherever we know the memory usage

  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plMeshBufferResource);
  out_NewMemoryUsage.m_uiMemoryGPU = ModifyMemoryUsage().m_uiMemoryGPU;
}

PLASMA_RESOURCE_IMPLEMENT_CREATEABLE(plMeshBufferResource, plMeshBufferResourceDescriptor)
{
  PLASMA_ASSERT_DEBUG(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  PLASMA_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");

  m_VertexDeclaration = descriptor.GetVertexDeclaration();
  m_VertexDeclaration.ComputeHash();

  m_uiPrimitiveCount = descriptor.GetPrimitiveCount();
  m_Topology = descriptor.GetTopology();

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  m_hVertexBuffer = pDevice->CreateVertexBuffer(descriptor.GetVertexDataSize(), descriptor.GetVertexCount(), descriptor.GetVertexBufferData().GetArrayPtr());

  plStringBuilder sName;
  sName.Format("{0} Vertex Buffer", GetResourceDescription());
  pDevice->GetBuffer(m_hVertexBuffer)->SetDebugName(sName);

  if (descriptor.HasIndexBuffer())
  {
    m_hIndexBuffer = pDevice->CreateIndexBuffer(descriptor.Uses32BitIndices() ? plGALIndexType::UInt : plGALIndexType::UShort, m_uiPrimitiveCount * plGALPrimitiveTopology::VerticesPerPrimitive(m_Topology), descriptor.GetIndexBufferData());

    sName.Format("{0} Index Buffer", GetResourceDescription());
    pDevice->GetBuffer(m_hIndexBuffer)->SetDebugName(sName);

    // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
    ModifyMemoryUsage().m_uiMemoryGPU = descriptor.GetVertexBufferData().GetCount() + descriptor.GetIndexBufferData().GetCount();
  }
  else
  {
    // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
    ModifyMemoryUsage().m_uiMemoryGPU = descriptor.GetVertexBufferData().GetCount();
  }


  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  m_Bounds = descriptor.ComputeBounds();

  return res;
}

void plVertexDeclarationInfo::ComputeHash()
{
  m_uiHash = 0;

  for (const auto& vs : m_VertexStreams)
  {
    m_uiHash += vs.CalculateHash();

    PLASMA_ASSERT_DEBUG(m_uiHash != 0, "Invalid Hash Value");
  }
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshBufferResource);
