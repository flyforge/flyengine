#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Memory/AllocatorWrapper.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

using plMeshBufferResourceHandle = plTypedResourceHandle<class plMeshBufferResource>;
class plGeometry;

struct PLASMA_RENDERERCORE_DLL plVertexStreamInfo : public plHashableStruct<plVertexStreamInfo>
{
  PLASMA_DECLARE_POD_TYPE();

  plGALVertexAttributeSemantic::Enum m_Semantic;
  plUInt8 m_uiVertexBufferSlot = 0;
  plGALResourceFormat::Enum m_Format;
  plUInt16 m_uiOffset;      ///< at which byte offset the first element starts
  plUInt16 m_uiElementSize; ///< the number of bytes for this element type (depends on the format); this is not the stride between elements!
};

struct PLASMA_RENDERERCORE_DLL plVertexDeclarationInfo
{
  void ComputeHash();

  plHybridArray<plVertexStreamInfo, 8> m_VertexStreams;
  plUInt32 m_uiHash;
};


struct PLASMA_RENDERERCORE_DLL plMeshBufferResourceDescriptor
{
public:
  plMeshBufferResourceDescriptor();
  ~plMeshBufferResourceDescriptor();

  void Clear();

  /// \brief Use this function to add vertex streams to the mesh buffer. The return value is the index of the just added stream.
  plUInt32 AddStream(plGALVertexAttributeSemantic::Enum semantic, plGALResourceFormat::Enum format);

  /// \brief Adds common vertex streams to the mesh buffer.
  ///
  /// The streams are added in this order (with the corresponding stream indices):
  /// * Position (index 0)
  /// * TexCoord0 (index 1)
  /// * Normal (index 2)
  /// * Tangent (index 3)
  void AddCommonStreams();

  /// \brief After all streams are added, call this to allocate the data for the streams. If uiNumPrimitives is 0, the mesh buffer will not
  /// use indexed rendering.
  void AllocateStreams(plUInt32 uiNumVertices, plGALPrimitiveTopology::Enum topology = plGALPrimitiveTopology::Triangles, plUInt32 uiNumPrimitives = 0, bool bZeroFill = false);

  /// \brief Creates streams and fills them with data from the plGeometry. Only the geometry matching the given topology is used.
  ///  Streams that do not match any of the data inside the plGeometry directly are skipped.
  void AllocateStreamsFromGeometry(const plGeometry& geom, plGALPrimitiveTopology::Enum topology = plGALPrimitiveTopology::Triangles);

  /// \brief Gives read access to the allocated vertex data
  plArrayPtr<const plUInt8> GetVertexBufferData() const;

  /// \brief Gives read access to the allocated index data
  plArrayPtr<const plUInt8> GetIndexBufferData() const;

  /// \brief Allows write access to the allocated vertex data. This can be used for copying data fast into the array.
  plDynamicArray<plUInt8, plAlignedAllocatorWrapper>& GetVertexBufferData();

  /// \brief Allows write access to the allocated index data. This can be used for copying data fast into the array.
  plDynamicArray<plUInt8, plAlignedAllocatorWrapper>& GetIndexBufferData();

  /// \brief Slow, but convenient method to write one piece of vertex data at a time into the stream buffer.
  ///
  /// uiStream is the index of the data stream to write to.
  /// uiVertexIndex is the index of the vertex for which to write the data.
  /// data is the piece of data to write to the stream.
  template <typename TYPE>
  void SetVertexData(plUInt32 uiStream, plUInt32 uiVertexIndex, const TYPE& data)
  {
    reinterpret_cast<TYPE&>(m_VertexStreamData[m_uiVertexSize * uiVertexIndex + m_VertexDeclaration.m_VertexStreams[uiStream].m_uiOffset]) = data;
  }

  /// \brief Slow, but convenient method to access one piece of vertex data at a time into the stream buffer.
  ///
  /// uiStream is the index of the data stream to write to.
  /// uiVertexIndex is the index of the vertex for which to write the data.
  plArrayPtr<plUInt8> GetVertexData(plUInt32 uiStream, plUInt32 uiVertexIndex) { return m_VertexStreamData.GetArrayPtr().GetSubArray(m_uiVertexSize * uiVertexIndex + m_VertexDeclaration.m_VertexStreams[uiStream].m_uiOffset); }

  /// \brief Writes the vertex index for the given point into the index buffer.
  void SetPointIndices(plUInt32 uiPoint, plUInt32 uiVertex0);

  /// \brief Writes the two vertex indices for the given line into the index buffer.
  void SetLineIndices(plUInt32 uiLine, plUInt32 uiVertex0, plUInt32 uiVertex1);

  /// \brief Writes the three vertex indices for the given triangle into the index buffer.
  void SetTriangleIndices(plUInt32 uiTriangle, plUInt32 uiVertex0, plUInt32 uiVertex1, plUInt32 uiVertex2);

  /// \brief Allows to read the stream info of the descriptor, which is filled out by AddStream()
  const plVertexDeclarationInfo& GetVertexDeclaration() const { return m_VertexDeclaration; }

  /// \brief Returns the byte size of all the data for one vertex.
  plUInt32 GetVertexDataSize() const { return m_uiVertexSize; }

  /// \brief Return the number of vertices, with which AllocateStreams() was called.
  plUInt32 GetVertexCount() const { return m_uiVertexCount; }

  /// \brief Returns the number of primitives that the array holds.
  plUInt32 GetPrimitiveCount() const;

  /// \brief Returns whether 16 or 32 Bit indices are to be used.
  bool Uses32BitIndices() const { return m_uiVertexCount > 0xFFFF; }

  /// \brief Returns whether an index buffer is available.
  bool HasIndexBuffer() const { return !m_IndexBufferData.IsEmpty(); }

  /// \brief Calculates the bounds using the data from the position stream
  plBoundingBoxSphere ComputeBounds() const;

  /// \brief Returns the primitive topology
  plGALPrimitiveTopology::Enum GetTopology() const { return m_Topology; }

  plResult RecomputeNormals();

private:
  plGALPrimitiveTopology::Enum m_Topology;
  plUInt32 m_uiVertexSize;
  plUInt32 m_uiVertexCount;
  plVertexDeclarationInfo m_VertexDeclaration;
  plDynamicArray<plUInt8, plAlignedAllocatorWrapper> m_VertexStreamData;
  plDynamicArray<plUInt8, plAlignedAllocatorWrapper> m_IndexBufferData;
};

class PLASMA_RENDERERCORE_DLL plMeshBufferResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMeshBufferResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plMeshBufferResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plMeshBufferResource, plMeshBufferResourceDescriptor);

public:
  plMeshBufferResource();
  ~plMeshBufferResource();

  PLASMA_ALWAYS_INLINE plUInt32 GetPrimitiveCount() const { return m_uiPrimitiveCount; }

  PLASMA_ALWAYS_INLINE plGALBufferHandle GetVertexBuffer() const { return m_hVertexBuffer; }

  PLASMA_ALWAYS_INLINE plGALBufferHandle GetIndexBuffer() const { return m_hIndexBuffer; }

  PLASMA_ALWAYS_INLINE plGALPrimitiveTopology::Enum GetTopology() const { return m_Topology; }

  /// \brief Returns the vertex declaration used by this mesh buffer.
  const plVertexDeclarationInfo& GetVertexDeclaration() const { return m_VertexDeclaration; }

  /// \brief Returns the bounds of the mesh
  const plBoundingBoxSphere& GetBounds() const { return m_Bounds; }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  plBoundingBoxSphere m_Bounds;
  plVertexDeclarationInfo m_VertexDeclaration;
  plUInt32 m_uiPrimitiveCount;
  plGALBufferHandle m_hVertexBuffer;
  plGALBufferHandle m_hIndexBuffer;
  plGALPrimitiveTopology::Enum m_Topology;
};
