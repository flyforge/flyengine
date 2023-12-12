
// static
PLASMA_ALWAYS_INLINE plGALResourceFormat::Enum plMeshNormalPrecision::ToResourceFormatNormal(Enum value)
{
  return value == _10Bit ? plGALResourceFormat::RGB10A2UIntNormalized
                         : (value == _16Bit ? plGALResourceFormat::RGBAUShortNormalized : plGALResourceFormat::XYZFloat);
}

// static
PLASMA_ALWAYS_INLINE plGALResourceFormat::Enum plMeshNormalPrecision::ToResourceFormatTangent(Enum value)
{
  return value == _10Bit ? plGALResourceFormat::RGB10A2UIntNormalized
                         : (value == _16Bit ? plGALResourceFormat::RGBAUShortNormalized : plGALResourceFormat::XYZWFloat);
}

//////////////////////////////////////////////////////////////////////////

// static
PLASMA_ALWAYS_INLINE plGALResourceFormat::Enum plMeshTexCoordPrecision::ToResourceFormat(Enum value)
{
  return value == _16Bit ? plGALResourceFormat::UVHalf : plGALResourceFormat::UVFloat;
}

//////////////////////////////////////////////////////////////////////////

// static
PLASMA_ALWAYS_INLINE plGALResourceFormat::Enum plMeshBoneWeigthPrecision::ToResourceFormat(Enum value)
{
  switch (value)
  {
    case _8Bit:
      return plGALResourceFormat::RGBAUByteNormalized;
    case _10Bit:
      return plGALResourceFormat::RGB10A2UIntNormalized;
    case _16Bit:
      return plGALResourceFormat::RGBAUShortNormalized;
    case _32Bit:
      return plGALResourceFormat::RGBAFloat;
  }
  return plGALResourceFormat::RGBAFloat;
}

//////////////////////////////////////////////////////////////////////////

// static
PLASMA_ALWAYS_INLINE plResult plMeshBufferUtils::EncodeNormal(const plVec3& vNormal, plArrayPtr<plUInt8> dest, plMeshNormalPrecision::Enum normalPrecision)
{
  return EncodeNormal(vNormal, dest, plMeshNormalPrecision::ToResourceFormatNormal(normalPrecision));
}

// static
PLASMA_ALWAYS_INLINE plResult plMeshBufferUtils::EncodeTangent(const plVec3& vTangent, float fTangentSign, plArrayPtr<plUInt8> dest, plMeshNormalPrecision::Enum tangentPrecision)
{
  return EncodeTangent(vTangent, fTangentSign, dest, plMeshNormalPrecision::ToResourceFormatTangent(tangentPrecision));
}

// static
PLASMA_ALWAYS_INLINE plResult plMeshBufferUtils::EncodeTexCoord(const plVec2& vTexCoord, plArrayPtr<plUInt8> dest, plMeshTexCoordPrecision::Enum texCoordPrecision)
{
  return EncodeTexCoord(vTexCoord, dest, plMeshTexCoordPrecision::ToResourceFormat(texCoordPrecision));
}

// static
PLASMA_ALWAYS_INLINE plResult plMeshBufferUtils::EncodeBoneWeights(const plVec4& vWeights, plArrayPtr<plUInt8> dest, plMeshBoneWeigthPrecision::Enum precision)
{
  return EncodeBoneWeights(vWeights, dest, plMeshBoneWeigthPrecision::ToResourceFormat(precision));
}

// static
PLASMA_ALWAYS_INLINE plResult plMeshBufferUtils::EncodeNormal(const plVec3& vNormal, plArrayPtr<plUInt8> dest, plGALResourceFormat::Enum destFormat)
{
  // we store normals in unsigned formats thus we need to map from -1..1 to 0..1 here
  return EncodeFromVec3(vNormal * 0.5f + plVec3(0.5f), dest, destFormat);
}

// static
PLASMA_ALWAYS_INLINE plResult plMeshBufferUtils::EncodeTangent(const plVec3& vTangent, float fTangentSign, plArrayPtr<plUInt8> dest, plGALResourceFormat::Enum destFormat)
{
  // make sure biTangentSign is either -1 or 1
  fTangentSign = (fTangentSign < 0.0f) ? -1.0f : 1.0f;

  // we store tangents in unsigned formats thus we need to map from -1..1 to 0..1 here
  return EncodeFromVec4(vTangent.GetAsVec4(fTangentSign) * 0.5f + plVec4(0.5f), dest, destFormat);
}

// static
PLASMA_ALWAYS_INLINE plResult plMeshBufferUtils::EncodeTexCoord(const plVec2& vTexCoord, plArrayPtr<plUInt8> dest, plGALResourceFormat::Enum destFormat)
{
  return EncodeFromVec2(vTexCoord, dest, destFormat);
}

// static
PLASMA_ALWAYS_INLINE plResult plMeshBufferUtils::EncodeBoneWeights(const plVec4& vWeights, plArrayPtr<plUInt8> dest, plGALResourceFormat::Enum destFormat)
{
  return EncodeFromVec4(vWeights, dest, destFormat);
}

// static
PLASMA_ALWAYS_INLINE plResult plMeshBufferUtils::DecodeNormal(plArrayPtr<const plUInt8> source, plVec3& ref_vDestNormal, plMeshNormalPrecision::Enum normalPrecision)
{
  return DecodeNormal(source, plMeshNormalPrecision::ToResourceFormatNormal(normalPrecision), ref_vDestNormal);
}

// static
PLASMA_ALWAYS_INLINE plResult plMeshBufferUtils::DecodeTangent(plArrayPtr<const plUInt8> source, plVec3& ref_vDestTangent, float& ref_fDestBiTangentSign, plMeshNormalPrecision::Enum tangentPrecision)
{
  return DecodeTangent(source, plMeshNormalPrecision::ToResourceFormatTangent(tangentPrecision), ref_vDestTangent, ref_fDestBiTangentSign);
}

// static
PLASMA_ALWAYS_INLINE plResult plMeshBufferUtils::DecodeTexCoord(plArrayPtr<const plUInt8> source, plVec2& ref_vDestTexCoord, plMeshTexCoordPrecision::Enum texCoordPrecision)
{
  return DecodeTexCoord(source, plMeshTexCoordPrecision::ToResourceFormat(texCoordPrecision), ref_vDestTexCoord);
}

// static
PLASMA_ALWAYS_INLINE plResult plMeshBufferUtils::DecodeNormal(plArrayPtr<const plUInt8> source, plGALResourceFormat::Enum sourceFormat, plVec3& ref_vDestNormal)
{
  plVec3 tempNormal;
  PLASMA_SUCCEED_OR_RETURN(DecodeToVec3(source, sourceFormat, tempNormal));
  ref_vDestNormal = tempNormal * 2.0f - plVec3(1.0f);
  return PLASMA_SUCCESS;
}

// static
PLASMA_ALWAYS_INLINE plResult plMeshBufferUtils::DecodeTangent(plArrayPtr<const plUInt8> source, plGALResourceFormat::Enum sourceFormat, plVec3& ref_vDestTangent, float& ref_fDestBiTangentSign)
{
  plVec4 tempTangent;
  PLASMA_SUCCEED_OR_RETURN(DecodeToVec4(source, sourceFormat, tempTangent));
  ref_vDestTangent = tempTangent.GetAsVec3() * 2.0f - plVec3(1.0f);
  ref_fDestBiTangentSign = tempTangent.w * 2.0f - 1.0f;
  return PLASMA_SUCCESS;
}

// static
PLASMA_ALWAYS_INLINE plResult plMeshBufferUtils::DecodeTexCoord(plArrayPtr<const plUInt8> source, plGALResourceFormat::Enum sourceFormat, plVec2& ref_vDestTexCoord)
{
  return DecodeToVec2(source, sourceFormat, ref_vDestTexCoord);
}
