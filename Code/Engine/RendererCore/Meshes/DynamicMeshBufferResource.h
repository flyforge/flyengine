#pragma once

#include <Core/ResourceManager/Resource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using plDynamicMeshBufferResourceHandle = plTypedResourceHandle<class plDynamicMeshBufferResource>;

struct plDynamicMeshBufferResourceDescriptor
{
  plGALPrimitiveTopology::Enum m_Topology = plGALPrimitiveTopology::Triangles;
  plGALIndexType::Enum m_IndexType = plGALIndexType::UInt;
  plUInt32 m_uiMaxPrimitives = 0;
  plUInt32 m_uiMaxVertices = 0;
  bool m_bColorStream = false;
};

struct PLASMA_RENDERERCORE_DLL plDynamicMeshVertex
{
  PLASMA_DECLARE_POD_TYPE();

  plVec3 m_vPosition;
  plVec2 m_vTexCoord;
  plVec3 m_vEncodedNormal;
  plVec4 m_vEncodedTangent;
  //plColorLinearUB m_Color;

  PLASMA_ALWAYS_INLINE void EncodeNormal(const plVec3& vNormal)
  {
    // store in [0; 1] range
    m_vEncodedNormal = vNormal * 0.5f + plVec3(0.5f);

    // this is the same
    //plMeshBufferUtils::EncodeNormal(normal, plByteArrayPtr(reinterpret_cast<plUInt8*>(&m_vEncodedNormal), sizeof(plVec3)), plMeshNormalPrecision::_32Bit).IgnoreResult();
  }

  PLASMA_ALWAYS_INLINE void EncodeTangent(const plVec3& vTangent, float fBitangentSign)
  {
    // store in [0; 1] range
    m_vEncodedTangent.x = vTangent.x * 0.5f + 0.5f;
    m_vEncodedTangent.y = vTangent.y * 0.5f + 0.5f;
    m_vEncodedTangent.z = vTangent.z * 0.5f + 0.5f;
    m_vEncodedTangent.w = fBitangentSign < 0.0f ? 0.0f : 1.0f;

    // this is the same
    //plMeshBufferUtils::EncodeTangent(tangent, bitangentSign, plByteArrayPtr(reinterpret_cast<plUInt8*>(&m_vEncodedTangent), sizeof(plVec4)), plMeshNormalPrecision::_32Bit).IgnoreResult();
  }
};

class PLASMA_RENDERERCORE_DLL plDynamicMeshBufferResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDynamicMeshBufferResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plDynamicMeshBufferResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plDynamicMeshBufferResource, plDynamicMeshBufferResourceDescriptor);

public:
  plDynamicMeshBufferResource();
  ~plDynamicMeshBufferResource();

  PLASMA_ALWAYS_INLINE const plDynamicMeshBufferResourceDescriptor& GetDescriptor() const { return m_Descriptor; }
  PLASMA_ALWAYS_INLINE plGALBufferHandle GetVertexBuffer() const { return m_hVertexBuffer; }
  PLASMA_ALWAYS_INLINE plGALBufferHandle GetIndexBuffer() const { return m_hIndexBuffer; }
  PLASMA_ALWAYS_INLINE plGALBufferHandle GetColorBuffer() const { return m_hColorBuffer; }

  /// \brief Grants write access to the vertex data, and flags the data as 'dirty'.
  plArrayPtr<plDynamicMeshVertex> AccessVertexData()
  {
    m_bAccessedVB = true;
    return m_VertexData;
  }

  /// \brief Grants write access to the 16 bit index data, and flags the data as 'dirty'.
  ///
  /// Accessing this data is only valid, if the buffer was created with 16 bit indices.
  plArrayPtr<plUInt16> AccessIndex16Data()
  {
    m_bAccessedIB = true;
    return m_Index16Data;
  }

  /// \brief Grants write access to the 32 bit index data, and flags the data as 'dirty'.
  ///
  /// Accessing this data is only valid, if the buffer was created with 32 bit indices.
  plArrayPtr<plUInt32> AccessIndex32Data()
  {
    m_bAccessedIB = true;
    return m_Index32Data;
  }

  /// \brief Grants write access to the color data, and flags the data as 'dirty'.
  ///
  /// Accessing this data is only valid, if creation of the color buffer was enabled.
  plArrayPtr<plColorLinearUB> AccessColorData()
  {
    m_bAccessedCB = true;
    return m_ColorData;
  }

  const plVertexDeclarationInfo& GetVertexDeclaration() const { return m_VertexDeclaration; }

  /// \brief Uploads the current vertex and index data to the GPU.
  ///
  /// If all values are set to default, the entire data is uploaded.
  /// If \a uiNumVertices or \a uiNumIndices is set to the max value, all vertices or indices (after their start offset) are uploaded.
  ///
  /// In all other cases, the number of elements to upload must be within valid bounds.
  ///
  /// This function can be used to only upload a subset of the modified data.
  ///
  /// Note that this function doesn't do anything, if the vertex or index data wasn't recently accessed through AccessVertexData(), AccessIndex16Data() or AccessIndex32Data(). So if you want to upload multiple pieces of the data to the GPU, you have to call these functions in between to flag the uploaded data as out-of-date.
  void UpdateGpuBuffer(plGALCommandEncoder* pGALCommandEncoder, plUInt32 uiFirstVertex = 0, plUInt32 uiNumVertices = plMath::MaxValue<plUInt32>(), plUInt32 uiFirstIndex = 0, plUInt32 uiNumIndices = plMath::MaxValue<plUInt32>(), plGALUpdateMode::Enum mode = plGALUpdateMode::Discard);

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  bool m_bAccessedVB = false;
  bool m_bAccessedIB = false;
  bool m_bAccessedCB = false;

  plGALBufferHandle m_hVertexBuffer;
  plGALBufferHandle m_hIndexBuffer;
  plGALBufferHandle m_hColorBuffer;
  plDynamicMeshBufferResourceDescriptor m_Descriptor;

  plVertexDeclarationInfo m_VertexDeclaration;
  plDynamicArray<plDynamicMeshVertex, plAlignedAllocatorWrapper> m_VertexData;
  plDynamicArray<plUInt16, plAlignedAllocatorWrapper> m_Index16Data;
  plDynamicArray<plUInt32, plAlignedAllocatorWrapper> m_Index32Data;
  plDynamicArray<plColorLinearUB, plAlignedAllocatorWrapper> m_ColorData;
};
