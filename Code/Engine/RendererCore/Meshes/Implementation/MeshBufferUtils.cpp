#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Math/Float16.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

namespace
{
  template <plUInt32 Bits>
  PL_ALWAYS_INLINE plUInt32 ColorFloatToUNorm(float value)
  {
    // Implemented according to
    // https://docs.microsoft.com/en-us/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (plMath::IsNaN(value))
    {
      return 0;
    }
    else
    {
      float fMaxValue = ((1 << Bits) - 1);
      return static_cast<plUInt32>(plMath::Saturate(value) * fMaxValue + 0.5f);
    }
  }

  template <plUInt32 Bits>
  constexpr inline float ColorUNormToFloat(plUInt32 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/en-us/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    plUInt32 uiMaxValue = ((1 << Bits) - 1);
    float fMaxValue = ((1 << Bits) - 1);
    return (value & uiMaxValue) * (1.0f / fMaxValue);
  }
} // namespace

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plMeshNormalPrecision, 1)
  PL_ENUM_CONSTANT(plMeshNormalPrecision::_10Bit),
  PL_ENUM_CONSTANT(plMeshNormalPrecision::_16Bit),
  PL_ENUM_CONSTANT(plMeshNormalPrecision::_32Bit),
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_ENUM(plMeshTexCoordPrecision, 1)
  PL_ENUM_CONSTANT(plMeshTexCoordPrecision::_16Bit),
  PL_ENUM_CONSTANT(plMeshTexCoordPrecision::_32Bit),
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_ENUM(plMeshBoneWeigthPrecision, 1)
  PL_ENUM_CONSTANT(plMeshBoneWeigthPrecision::_8Bit),
  PL_ENUM_CONSTANT(plMeshBoneWeigthPrecision::_10Bit),
  PL_ENUM_CONSTANT(plMeshBoneWeigthPrecision::_16Bit),
  PL_ENUM_CONSTANT(plMeshBoneWeigthPrecision::_32Bit),
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
plResult plMeshBufferUtils::EncodeFromFloat(const float fSource, plArrayPtr<plUInt8> dest, plGALResourceFormat::Enum destFormat)
{
  PL_ASSERT_DEBUG(dest.GetCount() >= plGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case plGALResourceFormat::RFloat:
      *reinterpret_cast<float*>(dest.GetPtr()) = fSource;
      return PL_SUCCESS;
    case plGALResourceFormat::RHalf:
      *reinterpret_cast<plFloat16*>(dest.GetPtr()) = fSource;
      return PL_SUCCESS;
    default:
      return PL_FAILURE;
  }
}

// static
plResult plMeshBufferUtils::EncodeFromVec2(const plVec2& vSource, plArrayPtr<plUInt8> dest, plGALResourceFormat::Enum destFormat)
{
  PL_ASSERT_DEBUG(dest.GetCount() >= plGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case plGALResourceFormat::RGFloat:
      *reinterpret_cast<plVec2*>(dest.GetPtr()) = vSource;
      return PL_SUCCESS;

    case plGALResourceFormat::RGHalf:
      *reinterpret_cast<plFloat16Vec2*>(dest.GetPtr()) = vSource;
      return PL_SUCCESS;

    default:
      return PL_FAILURE;
  }
}

// static
plResult plMeshBufferUtils::EncodeFromVec3(const plVec3& vSource, plArrayPtr<plUInt8> dest, plGALResourceFormat::Enum destFormat)
{
  PL_ASSERT_DEBUG(dest.GetCount() >= plGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case plGALResourceFormat::RGBFloat:
      *reinterpret_cast<plVec3*>(dest.GetPtr()) = vSource;
      return PL_SUCCESS;

    case plGALResourceFormat::RGBAUShortNormalized:
      reinterpret_cast<plUInt16*>(dest.GetPtr())[0] = plMath::ColorFloatToShort(vSource.x);
      reinterpret_cast<plUInt16*>(dest.GetPtr())[1] = plMath::ColorFloatToShort(vSource.y);
      reinterpret_cast<plUInt16*>(dest.GetPtr())[2] = plMath::ColorFloatToShort(vSource.z);
      reinterpret_cast<plUInt16*>(dest.GetPtr())[3] = 0;
      return PL_SUCCESS;

    case plGALResourceFormat::RGBAShortNormalized:
      reinterpret_cast<plInt16*>(dest.GetPtr())[0] = plMath::ColorFloatToSignedShort(vSource.x);
      reinterpret_cast<plInt16*>(dest.GetPtr())[1] = plMath::ColorFloatToSignedShort(vSource.y);
      reinterpret_cast<plInt16*>(dest.GetPtr())[2] = plMath::ColorFloatToSignedShort(vSource.z);
      reinterpret_cast<plInt16*>(dest.GetPtr())[3] = 0;
      return PL_SUCCESS;

    case plGALResourceFormat::RGB10A2UIntNormalized:
      *reinterpret_cast<plUInt32*>(dest.GetPtr()) = ColorFloatToUNorm<10>(vSource.x);
      *reinterpret_cast<plUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(vSource.y) << 10;
      *reinterpret_cast<plUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(vSource.z) << 20;
      return PL_SUCCESS;

    case plGALResourceFormat::RGBAUByteNormalized:
      dest.GetPtr()[0] = plMath::ColorFloatToByte(vSource.x);
      dest.GetPtr()[1] = plMath::ColorFloatToByte(vSource.y);
      dest.GetPtr()[2] = plMath::ColorFloatToByte(vSource.z);
      dest.GetPtr()[3] = 0;
      return PL_SUCCESS;

    case plGALResourceFormat::RGBAByteNormalized:
      dest.GetPtr()[0] = plMath::ColorFloatToSignedByte(vSource.x);
      dest.GetPtr()[1] = plMath::ColorFloatToSignedByte(vSource.y);
      dest.GetPtr()[2] = plMath::ColorFloatToSignedByte(vSource.z);
      dest.GetPtr()[3] = 0;
      return PL_SUCCESS;
    default:
      return PL_FAILURE;
  }
}

// static
plResult plMeshBufferUtils::EncodeFromVec4(const plVec4& vSource, plArrayPtr<plUInt8> dest, plGALResourceFormat::Enum destFormat)
{
  PL_ASSERT_DEBUG(dest.GetCount() >= plGALResourceFormat::GetBitsPerElement(destFormat) / 8, "Destination buffer is too small");

  switch (destFormat)
  {
    case plGALResourceFormat::RGBAFloat:
      *reinterpret_cast<plVec4*>(dest.GetPtr()) = vSource;
      return PL_SUCCESS;

    case plGALResourceFormat::RGBAHalf:
      *reinterpret_cast<plFloat16Vec4*>(dest.GetPtr()) = vSource;
      return PL_SUCCESS;

    case plGALResourceFormat::RGBAUShortNormalized:
      reinterpret_cast<plUInt16*>(dest.GetPtr())[0] = plMath::ColorFloatToShort(vSource.x);
      reinterpret_cast<plUInt16*>(dest.GetPtr())[1] = plMath::ColorFloatToShort(vSource.y);
      reinterpret_cast<plUInt16*>(dest.GetPtr())[2] = plMath::ColorFloatToShort(vSource.z);
      reinterpret_cast<plUInt16*>(dest.GetPtr())[3] = plMath::ColorFloatToShort(vSource.w);
      return PL_SUCCESS;

    case plGALResourceFormat::RGBAShortNormalized:
      reinterpret_cast<plInt16*>(dest.GetPtr())[0] = plMath::ColorFloatToSignedShort(vSource.x);
      reinterpret_cast<plInt16*>(dest.GetPtr())[1] = plMath::ColorFloatToSignedShort(vSource.y);
      reinterpret_cast<plInt16*>(dest.GetPtr())[2] = plMath::ColorFloatToSignedShort(vSource.z);
      reinterpret_cast<plInt16*>(dest.GetPtr())[3] = plMath::ColorFloatToSignedShort(vSource.w);
      return PL_SUCCESS;

    case plGALResourceFormat::RGB10A2UIntNormalized:
      *reinterpret_cast<plUInt32*>(dest.GetPtr()) = ColorFloatToUNorm<10>(vSource.x);
      *reinterpret_cast<plUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(vSource.y) << 10;
      *reinterpret_cast<plUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<10>(vSource.z) << 20;
      *reinterpret_cast<plUInt32*>(dest.GetPtr()) |= ColorFloatToUNorm<2>(vSource.w) << 30;
      return PL_SUCCESS;

    case plGALResourceFormat::RGBAUByteNormalized:
      dest.GetPtr()[0] = plMath::ColorFloatToByte(vSource.x);
      dest.GetPtr()[1] = plMath::ColorFloatToByte(vSource.y);
      dest.GetPtr()[2] = plMath::ColorFloatToByte(vSource.z);
      dest.GetPtr()[3] = plMath::ColorFloatToByte(vSource.w);
      return PL_SUCCESS;

    case plGALResourceFormat::RGBAByteNormalized:
      dest.GetPtr()[0] = plMath::ColorFloatToSignedByte(vSource.x);
      dest.GetPtr()[1] = plMath::ColorFloatToSignedByte(vSource.y);
      dest.GetPtr()[2] = plMath::ColorFloatToSignedByte(vSource.z);
      dest.GetPtr()[3] = plMath::ColorFloatToSignedByte(vSource.w);
      return PL_SUCCESS;

    default:
      return PL_FAILURE;
  }
}

// static
plResult plMeshBufferUtils::DecodeToFloat(plArrayPtr<const plUInt8> source, plGALResourceFormat::Enum sourceFormat, float& ref_fDest)
{
  PL_ASSERT_DEBUG(source.GetCount() >= plGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case plGALResourceFormat::RFloat:
      ref_fDest = *reinterpret_cast<const float*>(source.GetPtr());
      return PL_SUCCESS;
    case plGALResourceFormat::RHalf:
      ref_fDest = *reinterpret_cast<const plFloat16*>(source.GetPtr());
      return PL_SUCCESS;
    default:
      return PL_FAILURE;
  }
}

// static
plResult plMeshBufferUtils::DecodeToVec2(plArrayPtr<const plUInt8> source, plGALResourceFormat::Enum sourceFormat, plVec2& ref_vDest)
{
  PL_ASSERT_DEBUG(source.GetCount() >= plGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case plGALResourceFormat::RGFloat:
      ref_vDest = *reinterpret_cast<const plVec2*>(source.GetPtr());
      return PL_SUCCESS;
    case plGALResourceFormat::RGHalf:
      ref_vDest = *reinterpret_cast<const plFloat16Vec2*>(source.GetPtr());
      return PL_SUCCESS;
    default:
      return PL_FAILURE;
  }
}

// static
plResult plMeshBufferUtils::DecodeToVec3(plArrayPtr<const plUInt8> source, plGALResourceFormat::Enum sourceFormat, plVec3& ref_vDest)
{
  PL_ASSERT_DEBUG(source.GetCount() >= plGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case plGALResourceFormat::RGBFloat:
      ref_vDest = *reinterpret_cast<const plVec3*>(source.GetPtr());
      return PL_SUCCESS;

    case plGALResourceFormat::RGBAUShortNormalized:
      ref_vDest.x = plMath::ColorShortToFloat(reinterpret_cast<const plUInt16*>(source.GetPtr())[0]);
      ref_vDest.y = plMath::ColorShortToFloat(reinterpret_cast<const plUInt16*>(source.GetPtr())[1]);
      ref_vDest.z = plMath::ColorShortToFloat(reinterpret_cast<const plUInt16*>(source.GetPtr())[2]);
      return PL_SUCCESS;

    case plGALResourceFormat::RGBAShortNormalized:
      ref_vDest.x = plMath::ColorSignedShortToFloat(reinterpret_cast<const plInt16*>(source.GetPtr())[0]);
      ref_vDest.y = plMath::ColorSignedShortToFloat(reinterpret_cast<const plInt16*>(source.GetPtr())[1]);
      ref_vDest.z = plMath::ColorSignedShortToFloat(reinterpret_cast<const plInt16*>(source.GetPtr())[2]);
      return PL_SUCCESS;

    case plGALResourceFormat::RGB10A2UIntNormalized:
      ref_vDest.x = ColorUNormToFloat<10>(*reinterpret_cast<const plUInt32*>(source.GetPtr()));
      ref_vDest.y = ColorUNormToFloat<10>(*reinterpret_cast<const plUInt32*>(source.GetPtr()) >> 10);
      ref_vDest.z = ColorUNormToFloat<10>(*reinterpret_cast<const plUInt32*>(source.GetPtr()) >> 20);
      return PL_SUCCESS;

    case plGALResourceFormat::RGBAUByteNormalized:
      ref_vDest.x = plMath::ColorByteToFloat(source.GetPtr()[0]);
      ref_vDest.y = plMath::ColorByteToFloat(source.GetPtr()[1]);
      ref_vDest.z = plMath::ColorByteToFloat(source.GetPtr()[2]);
      return PL_SUCCESS;

    case plGALResourceFormat::RGBAByteNormalized:
      ref_vDest.x = plMath::ColorSignedByteToFloat(source.GetPtr()[0]);
      ref_vDest.y = plMath::ColorSignedByteToFloat(source.GetPtr()[1]);
      ref_vDest.z = plMath::ColorSignedByteToFloat(source.GetPtr()[2]);
      return PL_SUCCESS;
    default:
      return PL_FAILURE;
  }
}

// static
plResult plMeshBufferUtils::DecodeToVec4(plArrayPtr<const plUInt8> source, plGALResourceFormat::Enum sourceFormat, plVec4& ref_vDest)
{
  PL_ASSERT_DEBUG(source.GetCount() >= plGALResourceFormat::GetBitsPerElement(sourceFormat) / 8, "Source buffer is too small");

  switch (sourceFormat)
  {
    case plGALResourceFormat::RGBAFloat:
      ref_vDest = *reinterpret_cast<const plVec4*>(source.GetPtr());
      return PL_SUCCESS;

    case plGALResourceFormat::RGBAHalf:
      ref_vDest = *reinterpret_cast<const plFloat16Vec4*>(source.GetPtr());
      return PL_SUCCESS;

    case plGALResourceFormat::RGBAUShortNormalized:
      ref_vDest.x = plMath::ColorShortToFloat(reinterpret_cast<const plUInt16*>(source.GetPtr())[0]);
      ref_vDest.y = plMath::ColorShortToFloat(reinterpret_cast<const plUInt16*>(source.GetPtr())[1]);
      ref_vDest.z = plMath::ColorShortToFloat(reinterpret_cast<const plUInt16*>(source.GetPtr())[2]);
      ref_vDest.w = plMath::ColorShortToFloat(reinterpret_cast<const plUInt16*>(source.GetPtr())[3]);
      return PL_SUCCESS;

    case plGALResourceFormat::RGBAShortNormalized:
      ref_vDest.x = plMath::ColorSignedShortToFloat(reinterpret_cast<const plInt16*>(source.GetPtr())[0]);
      ref_vDest.y = plMath::ColorSignedShortToFloat(reinterpret_cast<const plInt16*>(source.GetPtr())[1]);
      ref_vDest.z = plMath::ColorSignedShortToFloat(reinterpret_cast<const plInt16*>(source.GetPtr())[2]);
      ref_vDest.w = plMath::ColorSignedShortToFloat(reinterpret_cast<const plInt16*>(source.GetPtr())[3]);
      return PL_SUCCESS;

    case plGALResourceFormat::RGB10A2UIntNormalized:
      ref_vDest.x = ColorUNormToFloat<10>(*reinterpret_cast<const plUInt32*>(source.GetPtr()));
      ref_vDest.y = ColorUNormToFloat<10>(*reinterpret_cast<const plUInt32*>(source.GetPtr()) >> 10);
      ref_vDest.z = ColorUNormToFloat<10>(*reinterpret_cast<const plUInt32*>(source.GetPtr()) >> 20);
      ref_vDest.w = ColorUNormToFloat<2>(*reinterpret_cast<const plUInt32*>(source.GetPtr()) >> 30);
      return PL_SUCCESS;

    case plGALResourceFormat::RGBAUByteNormalized:
      ref_vDest.x = plMath::ColorByteToFloat(source.GetPtr()[0]);
      ref_vDest.y = plMath::ColorByteToFloat(source.GetPtr()[1]);
      ref_vDest.z = plMath::ColorByteToFloat(source.GetPtr()[2]);
      ref_vDest.w = plMath::ColorByteToFloat(source.GetPtr()[3]);
      return PL_SUCCESS;

    case plGALResourceFormat::RGBAByteNormalized:
      ref_vDest.x = plMath::ColorSignedByteToFloat(source.GetPtr()[0]);
      ref_vDest.y = plMath::ColorSignedByteToFloat(source.GetPtr()[1]);
      ref_vDest.z = plMath::ColorSignedByteToFloat(source.GetPtr()[2]);
      ref_vDest.w = plMath::ColorSignedByteToFloat(source.GetPtr()[3]);
      return PL_SUCCESS;

    default:
      return PL_FAILURE;
  }
}

// static
plResult plMeshBufferUtils::GetPositionStream(const plMeshBufferResourceDescriptor& meshBufferDesc, const plVec3*& out_pPositions, plUInt32& out_uiElementStride)
{
  const plVertexDeclarationInfo& vdi = meshBufferDesc.GetVertexDeclaration();
  const plUInt8* pRawVertexData = meshBufferDesc.GetVertexBufferData().GetPtr();

  const plVec3* pPositions = nullptr;

  for (plUInt32 vs = 0; vs < vdi.m_VertexStreams.GetCount(); ++vs)
  {
    if (vdi.m_VertexStreams[vs].m_Semantic == plGALVertexAttributeSemantic::Position)
    {
      if (vdi.m_VertexStreams[vs].m_Format != plGALResourceFormat::RGBFloat)
      {
        plLog::Error("Unsupported vertex position format {0}", (int)vdi.m_VertexStreams[vs].m_Format);
        return PL_FAILURE; // other position formats are not supported
      }

      pPositions = reinterpret_cast<const plVec3*>(pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset);
    }
  }

  if (pPositions == nullptr)
  {
    plLog::Error("No position stream found");
    return PL_FAILURE;
  }

  out_pPositions = pPositions;
  out_uiElementStride = meshBufferDesc.GetVertexDataSize();
  return PL_SUCCESS;
}

// static
plResult plMeshBufferUtils::GetPositionAndNormalStream(const plMeshBufferResourceDescriptor& meshBufferDesc, const plVec3*& out_pPositions, const plUInt8*& out_pNormals, plGALResourceFormat::Enum& out_normalFormat, plUInt32& out_uiElementStride)
{
  const plVertexDeclarationInfo& vdi = meshBufferDesc.GetVertexDeclaration();
  const plUInt8* pRawVertexData = meshBufferDesc.GetVertexBufferData().GetPtr();

  const plVec3* pPositions = nullptr;
  const plUInt8* pNormals = nullptr;
  plGALResourceFormat::Enum normalFormat = plGALResourceFormat::Invalid;

  for (plUInt32 vs = 0; vs < vdi.m_VertexStreams.GetCount(); ++vs)
  {
    if (vdi.m_VertexStreams[vs].m_Semantic == plGALVertexAttributeSemantic::Position)
    {
      if (vdi.m_VertexStreams[vs].m_Format != plGALResourceFormat::RGBFloat)
      {
        plLog::Error("Unsupported vertex position format {0}", (int)vdi.m_VertexStreams[vs].m_Format);
        return PL_FAILURE; // other position formats are not supported
      }

      pPositions = reinterpret_cast<const plVec3*>(pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset);
    }
    else if (vdi.m_VertexStreams[vs].m_Semantic == plGALVertexAttributeSemantic::Normal)
    {
      pNormals = pRawVertexData + vdi.m_VertexStreams[vs].m_uiOffset;
      normalFormat = vdi.m_VertexStreams[vs].m_Format;
    }
  }

  if (pPositions == nullptr || pNormals == nullptr)
  {
    plLog::Error("No position and normal stream found");
    return PL_FAILURE;
  }

  plUInt8 dummySource[16] = {};
  plVec3 vNormal;
  if (DecodeNormal(plMakeArrayPtr(dummySource), normalFormat, vNormal).Failed())
  {
    plLog::Error("Unsupported vertex normal format {0}", normalFormat);
    return PL_FAILURE;
  }

  out_pPositions = pPositions;
  out_pNormals = pNormals;
  out_normalFormat = normalFormat;
  out_uiElementStride = meshBufferDesc.GetVertexDataSize();
  return PL_SUCCESS;
}


PL_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshBufferUtils);
