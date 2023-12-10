#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDynamicMeshBufferResource, 1, plRTTIDefaultAllocator<plDynamicMeshBufferResource>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plDynamicMeshBufferResource);
// clang-format on

plDynamicMeshBufferResource::plDynamicMeshBufferResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

plDynamicMeshBufferResource::~plDynamicMeshBufferResource()
{
  PLASMA_ASSERT_DEBUG(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  PLASMA_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");
  PLASMA_ASSERT_DEBUG(m_hColorBuffer.IsInvalidated(), "Implementation error");
}

plResourceLoadDesc plDynamicMeshBufferResource::UnloadData(Unload WhatToUnload)
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

  if (!m_hColorBuffer.IsInvalidated())
  {
    plGALDevice::GetDefaultDevice()->DestroyBuffer(m_hColorBuffer);
    m_hColorBuffer.Invalidate();
  }

  // we cannot compute this in UpdateMemoryUsage(), so we only read the data there, therefore we need to update this information here
  ModifyMemoryUsage().m_uiMemoryGPU = 0;

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  return res;
}

plResourceLoadDesc plDynamicMeshBufferResource::UpdateContent(plStreamReader* Stream)
{
  PLASMA_REPORT_FAILURE("This resource type does not support loading data from file.");

  return plResourceLoadDesc();
}

void plDynamicMeshBufferResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // we cannot compute this data here, so we update it wherever we know the memory usage

  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(plDynamicMeshBufferResource) + m_VertexData.GetHeapMemoryUsage() + m_Index16Data.GetHeapMemoryUsage() + m_Index32Data.GetHeapMemoryUsage() + m_ColorData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = ModifyMemoryUsage().m_uiMemoryGPU;
}

PLASMA_RESOURCE_IMPLEMENT_CREATEABLE(plDynamicMeshBufferResource, plDynamicMeshBufferResourceDescriptor)
{
  PLASMA_ASSERT_DEBUG(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  PLASMA_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");
  PLASMA_ASSERT_DEBUG(m_hColorBuffer.IsInvalidated(), "Implementation error");

  m_Descriptor = descriptor;

  m_VertexData.SetCountUninitialized(m_Descriptor.m_uiMaxVertices);

  {
    plVertexStreamInfo si;
    si.m_uiOffset = 0;
    si.m_Format = plGALResourceFormat::XYZFloat;
    si.m_Semantic = plGALVertexAttributeSemantic::Position;
    si.m_uiElementSize = sizeof(plVec3);
    m_VertexDeclaration.m_VertexStreams.PushBack(si);

    si.m_uiOffset += si.m_uiElementSize;
    si.m_Format = plGALResourceFormat::XYFloat;
    si.m_Semantic = plGALVertexAttributeSemantic::TexCoord0;
    si.m_uiElementSize = sizeof(plVec2);
    m_VertexDeclaration.m_VertexStreams.PushBack(si);

    si.m_uiOffset += si.m_uiElementSize;
    si.m_Format = plGALResourceFormat::XYZFloat;
    si.m_Semantic = plGALVertexAttributeSemantic::Normal;
    si.m_uiElementSize = sizeof(plVec3);
    m_VertexDeclaration.m_VertexStreams.PushBack(si);

    si.m_uiOffset += si.m_uiElementSize;
    si.m_Format = plGALResourceFormat::XYZWFloat;
    si.m_Semantic = plGALVertexAttributeSemantic::Tangent;
    si.m_uiElementSize = sizeof(plVec4);
    m_VertexDeclaration.m_VertexStreams.PushBack(si);

    if (m_Descriptor.m_bColorStream)
    {
      si.m_uiVertexBufferSlot = 1; // separate buffer
      si.m_uiOffset = 0;
      si.m_Format = plGALResourceFormat::RGBAUByteNormalized;
      si.m_Semantic = plGALVertexAttributeSemantic::Color0;
      si.m_uiElementSize = sizeof(plColorLinearUB);
      m_VertexDeclaration.m_VertexStreams.PushBack(si);
    }

    m_VertexDeclaration.ComputeHash();
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  m_hVertexBuffer = pDevice->CreateVertexBuffer(sizeof(plDynamicMeshVertex), m_Descriptor.m_uiMaxVertices /* no initial data -> mutable */);

  plStringBuilder sName;
  sName.Format("{0} - Dynamic Vertex Buffer", GetResourceDescription());
  pDevice->GetBuffer(m_hVertexBuffer)->SetDebugName(sName);

  const plUInt32 uiMaxIndices = plGALPrimitiveTopology::VerticesPerPrimitive(m_Descriptor.m_Topology) * m_Descriptor.m_uiMaxPrimitives;

  if (m_Descriptor.m_bColorStream)
  {
    m_ColorData.SetCountUninitialized(uiMaxIndices);
    m_hColorBuffer = pDevice->CreateVertexBuffer(sizeof(plColorLinearUB), m_Descriptor.m_uiMaxVertices /* no initial data -> mutable */);

    sName.Format("{0} - Dynamic Color Buffer", GetResourceDescription());
    pDevice->GetBuffer(m_hColorBuffer)->SetDebugName(sName);
  }

  if (m_Descriptor.m_IndexType == plGALIndexType::UInt)
  {
    m_Index32Data.SetCountUninitialized(uiMaxIndices);

    m_hIndexBuffer = pDevice->CreateIndexBuffer(plGALIndexType::UInt, uiMaxIndices /* no initial data -> mutable */);

    sName.Format("{0} - Dynamic Index32 Buffer", GetResourceDescription());
    pDevice->GetBuffer(m_hIndexBuffer)->SetDebugName(sName);
  }
  else if (m_Descriptor.m_IndexType == plGALIndexType::UShort)
  {
    m_Index16Data.SetCountUninitialized(uiMaxIndices);

    m_hIndexBuffer = pDevice->CreateIndexBuffer(plGALIndexType::UShort, uiMaxIndices /* no initial data -> mutable */);

    sName.Format("{0} - Dynamic Index16 Buffer", GetResourceDescription());
    pDevice->GetBuffer(m_hIndexBuffer)->SetDebugName(sName);
  }

  // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
  ModifyMemoryUsage().m_uiMemoryGPU = m_VertexData.GetHeapMemoryUsage() + m_Index32Data.GetHeapMemoryUsage() + m_Index16Data.GetHeapMemoryUsage() + m_ColorData.GetHeapMemoryUsage();

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  return res;
}

void plDynamicMeshBufferResource::UpdateGpuBuffer(plGALCommandEncoder* pGALCommandEncoder, plUInt32 uiFirstVertex, plUInt32 uiNumVertices, plUInt32 uiFirstIndex, plUInt32 uiNumIndices, plGALUpdateMode::Enum mode /*= plGALUpdateMode::Discard*/)
{
  if (m_bAccessedVB && uiNumVertices > 0)
  {
    if (uiNumVertices == plMath::MaxValue<plUInt32>())
      uiNumVertices = m_VertexData.GetCount() - uiFirstVertex;

    PLASMA_ASSERT_DEV(uiNumVertices <= m_VertexData.GetCount(), "Can't upload {} vertices, the buffer was allocated to hold a maximum of {} vertices.", uiNumVertices, m_VertexData.GetCount());

    m_bAccessedVB = false;

    pGALCommandEncoder->UpdateBuffer(m_hVertexBuffer, sizeof(plDynamicMeshVertex) * uiFirstVertex, m_VertexData.GetArrayPtr().GetSubArray(uiFirstVertex, uiNumVertices).ToByteArray(), mode);
  }

  if (m_bAccessedCB && uiNumVertices > 0)
  {
    if (uiNumVertices == plMath::MaxValue<plUInt32>())
      uiNumVertices = m_ColorData.GetCount() - uiFirstVertex;

    PLASMA_ASSERT_DEV(uiNumVertices <= m_ColorData.GetCount(), "Can't upload {} vertices, the buffer was allocated to hold a maximum of {} vertices.", uiNumVertices, m_ColorData.GetCount());

    m_bAccessedCB = false;

    pGALCommandEncoder->UpdateBuffer(m_hColorBuffer, sizeof(plColorLinearUB) * uiFirstVertex, m_ColorData.GetArrayPtr().GetSubArray(uiFirstVertex, uiNumVertices).ToByteArray(), mode);
  }

  if (m_bAccessedIB && uiNumIndices > 0 && !m_hIndexBuffer.IsInvalidated())
  {
    m_bAccessedIB = false;

    if (!m_Index16Data.IsEmpty())
    {
      PLASMA_ASSERT_DEV(uiFirstIndex < m_Index16Data.GetCount(), "Invalid first index value {}", uiFirstIndex);

      if (uiNumIndices == plMath::MaxValue<plUInt32>())
        uiNumIndices = m_Index16Data.GetCount() - uiFirstIndex;

      PLASMA_ASSERT_DEV(uiNumIndices <= m_Index16Data.GetCount(), "Can't upload {} indices, the buffer was allocated to hold a maximum of {} indices.", uiNumIndices, m_Index16Data.GetCount());

      pGALCommandEncoder->UpdateBuffer(m_hIndexBuffer, sizeof(plUInt16) * uiFirstIndex, m_Index16Data.GetArrayPtr().GetSubArray(uiFirstIndex, uiNumIndices).ToByteArray(), mode);
    }
    else if (!m_Index32Data.IsEmpty())
    {
      PLASMA_ASSERT_DEV(uiFirstIndex < m_Index32Data.GetCount(), "Invalid first index value {}", uiFirstIndex);

      if (uiNumIndices == plMath::MaxValue<plUInt32>())
        uiNumIndices = m_Index32Data.GetCount() - uiFirstIndex;

      PLASMA_ASSERT_DEV(uiNumIndices <= m_Index32Data.GetCount(), "Can't upload {} indices, the buffer was allocated to hold a maximum of {} indices.", uiNumIndices, m_Index32Data.GetCount());

      pGALCommandEncoder->UpdateBuffer(m_hIndexBuffer, sizeof(plUInt32) * uiFirstIndex, m_Index32Data.GetArrayPtr().GetSubArray(uiFirstIndex, uiNumIndices).ToByteArray(), mode);
    }
  }
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_DynamicMeshBufferResource);
