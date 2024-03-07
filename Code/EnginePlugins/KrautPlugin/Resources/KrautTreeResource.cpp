#include <KrautPlugin/KrautPluginPCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <KrautPlugin/Resources/KrautTreeResource.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>
#include <RendererCore/Textures/Texture2DResource.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  include <Foundation/IO/CompressedStreamZstd.h>
#endif

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plKrautTreeResource, 1, plRTTIDefaultAllocator<plKrautTreeResource>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_RESOURCE_IMPLEMENT_COMMON_CODE(plKrautTreeResource);
// clang-format on

plKrautTreeResource::plKrautTreeResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
  m_Details.m_Bounds = plBoundingBoxSphere::MakeInvalid();
}

plResourceLoadDesc plKrautTreeResource::UnloadData(Unload WhatToUnload)
{
  plResourceLoadDesc res;
  res.m_State = GetLoadingState();
  res.m_uiQualityLevelsDiscardable = GetNumQualityLevelsDiscardable();
  res.m_uiQualityLevelsLoadable = GetNumQualityLevelsLoadable();

  // we currently can only unload the entire KrautTree
  // if (WhatToUnload == Unload::AllQualityLevels)
  {
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;
    res.m_State = plResourceState::Unloaded;
  }

  return res;
}

plResourceLoadDesc plKrautTreeResource::UpdateContent(plStreamReader* Stream)
{
  plKrautTreeResourceDescriptor desc;
  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

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

  if (desc.Load(*Stream).Failed())
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(std::move(desc));
}

void plKrautTreeResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // TODO
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(*this);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

PL_RESOURCE_IMPLEMENT_CREATEABLE(plKrautTreeResource, plKrautTreeResourceDescriptor)
{
  m_TreeLODs.Clear();
  m_Details = descriptor.m_Details;

  plStringBuilder sResName, sResDesc;

  for (plUInt32 lodIdx = 0; lodIdx < descriptor.m_Lods.GetCount(); ++lodIdx)
  {
    const auto& lodSrc = descriptor.m_Lods[lodIdx];

    if (lodSrc.m_LodType != plKrautLodType::Mesh)
    {
      // ignore impostor LODs
      break;
    }

    auto& lodDst = m_TreeLODs.ExpandAndGetRef();

    lodDst.m_LodType = lodSrc.m_LodType;
    lodDst.m_fMinLodDistance = lodSrc.m_fMinLodDistance;
    lodDst.m_fMaxLodDistance = lodSrc.m_fMaxLodDistance;

    plMeshResourceDescriptor md;
    auto& buffer = md.MeshBufferDesc();

    const plUInt32 uiNumVertices = lodSrc.m_Vertices.GetCount();
    const plUInt32 uiNumTriangles = lodSrc.m_Triangles.GetCount();
    const plUInt32 uiSubMeshes = lodSrc.m_SubMeshes.GetCount();

    if (uiNumVertices == 0 || uiNumTriangles == 0 || uiSubMeshes == 0)
       continue;

    buffer.AddStream(plGALVertexAttributeSemantic::Position, plGALResourceFormat::XYZFloat);                                                // 0
    buffer.AddStream(plGALVertexAttributeSemantic::TexCoord0, plGALResourceFormat::XYFloat);                                                // 1
    buffer.AddStream(plGALVertexAttributeSemantic::TexCoord1, plGALResourceFormat::XYFloat);                                                // 2
    buffer.AddStream(plGALVertexAttributeSemantic::Normal, plMeshNormalPrecision::ToResourceFormatNormal(plMeshNormalPrecision::_10Bit));   // 3
    buffer.AddStream(plGALVertexAttributeSemantic::Tangent, plMeshNormalPrecision::ToResourceFormatTangent(plMeshNormalPrecision::_10Bit)); // 4
    buffer.AddStream(plGALVertexAttributeSemantic::Color0, plGALResourceFormat::XYZWFloat);                                                 // 5 TODO: better packing
    buffer.AddStream(plGALVertexAttributeSemantic::Color1, plGALResourceFormat::XYZWFloat);                                                 // 6 TODO: better packing
    buffer.AllocateStreams(uiNumVertices, plGALPrimitiveTopology::Triangles, uiNumTriangles);

    for (plUInt32 v = 0; v < uiNumVertices; ++v)
    {
      const auto& vtx = lodSrc.m_Vertices[v];

      buffer.SetVertexData<plVec3>(0, v, vtx.m_vPosition);
      buffer.SetVertexData<plVec2>(1, v, plVec2(vtx.m_vTexCoord.x, vtx.m_vTexCoord.y));
      buffer.SetVertexData<plVec2>(2, v, plVec2(vtx.m_vTexCoord.z, vtx.m_fAmbientOcclusion));
      plMeshBufferUtils::EncodeNormal(vtx.m_vNormal, buffer.GetVertexData(3, v), plMeshNormalPrecision::_10Bit).IgnoreResult();
      plMeshBufferUtils::EncodeTangent(vtx.m_vTangent, 1.0f, buffer.GetVertexData(4, v), plMeshNormalPrecision::_10Bit).IgnoreResult();

      plColor color;
      color.r = vtx.m_fBendAndFlutterStrength;
      color.g = (float)vtx.m_uiBranchLevel;
      color.b = plMath::ColorByteToFloat(vtx.m_uiFlutterPhase);
      color.a = plMath::ColorByteToFloat(vtx.m_uiColorVariation);

      buffer.SetVertexData<plColor>(5, v, color);

      buffer.SetVertexData<plVec4>(6, v, vtx.m_vBendAnchor.GetAsVec4(vtx.m_fAnchorBendStrength));
    }

    for (plUInt32 t = 0; t < uiNumTriangles; ++t)
    {
      const auto& tri = lodSrc.m_Triangles[t];

      buffer.SetTriangleIndices(t, tri.m_uiVertexIndex[0], tri.m_uiVertexIndex[1], tri.m_uiVertexIndex[2]);
    }

    for (plUInt32 sm = 0; sm < uiSubMeshes; ++sm)
    {
      const auto& subMesh = lodSrc.m_SubMeshes[sm];

      md.AddSubMesh(subMesh.m_uiNumTriangles, subMesh.m_uiFirstTriangle, subMesh.m_uiMaterialIndex);
    }

    md.ComputeBounds();

    for (plUInt32 mat = 0; mat < descriptor.m_Materials.GetCount(); ++mat)
    {
      md.SetMaterial(mat, descriptor.m_Materials[mat].m_sMaterial);
    }

    sResName.SetFormat("{0}_{1}_LOD{2}", GetResourceID(), GetCurrentResourceChangeCounter(), lodIdx);

    if (GetResourceDescription().IsEmpty())
    {
      sResDesc = sResName;
    }
    else
    {
      sResDesc.SetFormat("{0}_{1}_LOD{2}", GetResourceDescription(), GetCurrentResourceChangeCounter(), lodIdx);
    }

    lodDst.m_hMesh = plResourceManager::GetExistingResource<plMeshResource>(sResName);

    if (!lodDst.m_hMesh.IsValid())
    {
      lodDst.m_hMesh = plResourceManager::GetOrCreateResource<plMeshResource>(sResName, std::move(md), sResDesc);
    }
  }

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Loaded;

  return res;
}

//////////////////////////////////////////////////////////////////////////

void plKrautTreeResourceDescriptor::Save(plStreamWriter& inout_stream0) const
{
  plUInt8 uiVersion = 15;

  inout_stream0 << uiVersion;

  plUInt8 uiCompressionMode = 0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  uiCompressionMode = 1;
  plCompressedStreamWriterZstd stream(&inout_stream0, 0, plCompressedStreamWriterZstd::Compression::Average);
#else
  plStreamWriter& stream = stream0;
#endif

  inout_stream0 << uiCompressionMode;

  const plUInt8 uiNumLods = static_cast<plUInt8>(m_Lods.GetCount());
  stream << uiNumLods;

  for (plUInt8 lodIdx = 0; lodIdx < uiNumLods; ++lodIdx)
  {
    const auto& lod = m_Lods[lodIdx];

    stream << static_cast<plUInt8>(lod.m_LodType);
    stream << lod.m_fMinLodDistance;
    stream << lod.m_fMaxLodDistance;
    stream << lod.m_Vertices.GetCount();
    stream << lod.m_Triangles.GetCount();
    stream << lod.m_SubMeshes.GetCount();

    for (const auto& vtx : lod.m_Vertices)
    {
      stream << vtx.m_vPosition;
      stream << vtx.m_vTexCoord;
      stream << vtx.m_vNormal;
      stream << vtx.m_vTangent;
      stream << vtx.m_fAmbientOcclusion;
      stream << vtx.m_uiColorVariation;
      stream << vtx.m_uiBranchLevel;
      stream << vtx.m_vBendAnchor;
      stream << vtx.m_fAnchorBendStrength;
      stream << vtx.m_fBendAndFlutterStrength;
      stream << vtx.m_uiFlutterPhase;
    }

    for (const auto& tri : lod.m_Triangles)
    {
      stream << tri.m_uiVertexIndex[0];
      stream << tri.m_uiVertexIndex[1];
      stream << tri.m_uiVertexIndex[2];
    }

    for (const auto& sm : lod.m_SubMeshes)
    {
      stream << sm.m_uiFirstTriangle;
      stream << sm.m_uiNumTriangles;
      stream << sm.m_uiMaterialIndex;
    }
  }

  const plUInt8 uiNumMats = static_cast<plUInt8>(m_Materials.GetCount());
  stream << uiNumMats;

  for (const auto& mat : m_Materials)
  {
    stream << static_cast<plUInt8>(mat.m_MaterialType);
    stream << mat.m_sMaterial;
    stream << mat.m_VariationColor;
  }

  stream << m_Details.m_Bounds;
  stream << m_Details.m_vLeafCenter;
  stream << m_Details.m_fStaticColliderRadius;
  stream << m_Details.m_sSurfaceResource;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  stream.FinishCompressedStream().IgnoreResult();

  plLog::Dev("Compressed Kraut tree data from {0} KB to {1} KB ({2}%%)", plArgF((float)stream.GetUncompressedSize() / 1024.0f, 1), plArgF((float)stream.GetCompressedSize() / 1024.0f, 1), plArgF(100.0f * stream.GetCompressedSize() / stream.GetUncompressedSize(), 1));
#endif
}

plResult plKrautTreeResourceDescriptor::Load(plStreamReader& inout_stream0)
{
  plUInt8 uiVersion = 0;

  inout_stream0 >> uiVersion;

  if (uiVersion < 15)
    return PL_FAILURE;

  plUInt8 uiCompressionMode = 0;
  inout_stream0 >> uiCompressionMode;

  plStreamReader* pCompressor = &inout_stream0;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  plCompressedStreamReaderZstd decompressorZstd;
#endif

  switch (uiCompressionMode)
  {
    case 0:
      break;

    case 1:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
      decompressorZstd.SetInputStream(&inout_stream0);
      pCompressor = &decompressorZstd;
      break;
#else
      plLog::Error("Kraut tree is compressed with zstandard, but support for this compressor is not compiled in.");
      return PL_FAILURE;
#endif

    default:
      plLog::Error("Kraut tree is compressed with an unknown algorithm.");
      return PL_FAILURE;
  }

  plStreamReader& stream = *pCompressor;

  plUInt8 uiNumLods = 0;
  stream >> uiNumLods;

  for (plUInt8 lodIdx = 0; lodIdx < uiNumLods; ++lodIdx)
  {
    auto& lod = m_Lods.ExpandAndGetRef();

    plUInt8 lodType;
    stream >> lodType;
    lod.m_LodType = static_cast<plKrautLodType>(lodType);
    stream >> lod.m_fMinLodDistance;
    stream >> lod.m_fMaxLodDistance;

    plUInt32 numVertices, numTriangles, numSubMeshes;

    stream >> numVertices;
    stream >> numTriangles;
    stream >> numSubMeshes;

    lod.m_Vertices.SetCountUninitialized(numVertices);
    lod.m_Triangles.SetCountUninitialized(numTriangles);
    lod.m_SubMeshes.SetCount(numSubMeshes); // initialize this one because of the material handle

    for (auto& vtx : lod.m_Vertices)
    {
      stream >> vtx.m_vPosition;
      stream >> vtx.m_vTexCoord;
      stream >> vtx.m_vNormal;
      stream >> vtx.m_vTangent;
      stream >> vtx.m_fAmbientOcclusion;
      stream >> vtx.m_uiColorVariation;
      stream >> vtx.m_uiBranchLevel;
      stream >> vtx.m_vBendAnchor;
      stream >> vtx.m_fAnchorBendStrength;
      stream >> vtx.m_fBendAndFlutterStrength;
      stream >> vtx.m_uiFlutterPhase;
    }

    for (auto& tri : lod.m_Triangles)
    {
      stream >> tri.m_uiVertexIndex[0];
      stream >> tri.m_uiVertexIndex[1];
      stream >> tri.m_uiVertexIndex[2];
    }

    for (auto& sm : lod.m_SubMeshes)
    {
      stream >> sm.m_uiFirstTriangle;
      stream >> sm.m_uiNumTriangles;
      stream >> sm.m_uiMaterialIndex;
    }
  }

  plUInt8 uiNumMats = 0;

  stream >> uiNumMats;
  m_Materials.SetCount(uiNumMats);

  for (auto& mat : m_Materials)
  {
    plUInt8 matType = 0;
    stream >> matType;
    mat.m_MaterialType = static_cast<plKrautMaterialType>(matType);

    if (uiVersion >= 14)
    {
      stream >> mat.m_sMaterial;
    }
    else
    {
      plStringBuilder tmp;
      stream >> tmp;
      stream >> tmp;
    }

    stream >> mat.m_VariationColor;
  }

  stream >> m_Details.m_Bounds;
  stream >> m_Details.m_vLeafCenter;
  stream >> m_Details.m_fStaticColliderRadius;
  stream >> m_Details.m_sSurfaceResource;

  if (uiVersion == 13)
  {
    stream >> uiNumMats;

    for (plUInt32 i = 0; i < uiNumMats; ++i)
    {
      stream >> m_Materials[i].m_sMaterial;
    }
  }

  return PL_SUCCESS;
}
