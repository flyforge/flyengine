
#pragma once

#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>

struct plMeshBufferResourceDescriptor;

struct plMeshNormalPrecision
{
  using StorageType = plUInt8;

  enum Enum
  {
    _10Bit,
    _16Bit,
    _32Bit,

    Default = _10Bit
  };

  /// \brief Convert mesh normal precision to actual resource format used for normals
  static plGALResourceFormat::Enum ToResourceFormatNormal(Enum value);

  /// \brief Convert mesh normal precision to actual resource format used for tangents
  static plGALResourceFormat::Enum ToResourceFormatTangent(Enum value);
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plMeshNormalPrecision);

struct plMeshTexCoordPrecision
{
  using StorageType = plUInt8;

  enum Enum
  {
    _16Bit,
    _32Bit,

    Default = _16Bit
  };

  /// \brief Convert mesh texcoord precision to actual resource format
  static plGALResourceFormat::Enum ToResourceFormat(Enum value);
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plMeshTexCoordPrecision);

struct plMeshBoneWeigthPrecision
{
  using StorageType = plUInt8;

  enum Enum
  {
    _8Bit,
    _10Bit,
    _16Bit,
    _32Bit,

    Default = _8Bit
  };

  /// \brief Convert mesh texcoord precision to actual resource format
  static plGALResourceFormat::Enum ToResourceFormat(Enum value);
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plMeshBoneWeigthPrecision);

struct PLASMA_RENDERERCORE_DLL plMeshBufferUtils
{
  static plResult EncodeNormal(const plVec3& vNormal, plArrayPtr<plUInt8> dest, plMeshNormalPrecision::Enum normalPrecision);
  static plResult EncodeTangent(const plVec3& vTangent, float fTangentSign, plArrayPtr<plUInt8> dest, plMeshNormalPrecision::Enum tangentPrecision);
  static plResult EncodeTexCoord(const plVec2& vTexCoord, plArrayPtr<plUInt8> dest, plMeshTexCoordPrecision::Enum texCoordPrecision);
  static plResult EncodeBoneWeights(const plVec4& vWeights, plArrayPtr<plUInt8> dest, plMeshBoneWeigthPrecision::Enum precision);

  static plResult EncodeNormal(const plVec3& vNormal, plArrayPtr<plUInt8> dest, plGALResourceFormat::Enum destFormat);
  static plResult EncodeTangent(const plVec3& vTangent, float fTangentSign, plArrayPtr<plUInt8> dest, plGALResourceFormat::Enum destFormat);
  static plResult EncodeTexCoord(const plVec2& vTexCoord, plArrayPtr<plUInt8> dest, plGALResourceFormat::Enum destFormat);
  static plResult EncodeBoneWeights(const plVec4& vWeights, plArrayPtr<plUInt8> dest, plGALResourceFormat::Enum destFormat);

  static plResult DecodeNormal(plArrayPtr<const plUInt8> source, plVec3& ref_vDestNormal, plMeshNormalPrecision::Enum normalPrecision);
  static plResult DecodeTangent(
    plArrayPtr<const plUInt8> source, plVec3& ref_vDestTangent, float& ref_fDestBiTangentSign, plMeshNormalPrecision::Enum tangentPrecision);
  static plResult DecodeTexCoord(plArrayPtr<const plUInt8> source, plVec2& ref_vDestTexCoord, plMeshTexCoordPrecision::Enum texCoordPrecision);

  static plResult DecodeNormal(plArrayPtr<const plUInt8> source, plGALResourceFormat::Enum sourceFormat, plVec3& ref_vDestNormal);
  static plResult DecodeTangent(
    plArrayPtr<const plUInt8> source, plGALResourceFormat::Enum sourceFormat, plVec3& ref_vDestTangent, float& ref_fDestBiTangentSign);
  static plResult DecodeTexCoord(plArrayPtr<const plUInt8> source, plGALResourceFormat::Enum sourceFormat, plVec2& ref_vDestTexCoord);

  // low level conversion functions
  static plResult EncodeFromFloat(const float fSource, plArrayPtr<plUInt8> dest, plGALResourceFormat::Enum destFormat);
  static plResult EncodeFromVec2(const plVec2& vSource, plArrayPtr<plUInt8> dest, plGALResourceFormat::Enum destFormat);
  static plResult EncodeFromVec3(const plVec3& vSource, plArrayPtr<plUInt8> dest, plGALResourceFormat::Enum destFormat);
  static plResult EncodeFromVec4(const plVec4& vSource, plArrayPtr<plUInt8> dest, plGALResourceFormat::Enum destFormat);

  static plResult DecodeToFloat(plArrayPtr<const plUInt8> source, plGALResourceFormat::Enum sourceFormat, float& ref_fDest);
  static plResult DecodeToVec2(plArrayPtr<const plUInt8> source, plGALResourceFormat::Enum sourceFormat, plVec2& ref_vDest);
  static plResult DecodeToVec3(plArrayPtr<const plUInt8> source, plGALResourceFormat::Enum sourceFormat, plVec3& ref_vDest);
  static plResult DecodeToVec4(plArrayPtr<const plUInt8> source, plGALResourceFormat::Enum sourceFormat, plVec4& ref_vDest);

  /// \brief Helper function to get the position stream from the given mesh buffer descriptor
  static plResult GetPositionStream(const plMeshBufferResourceDescriptor& meshBufferDesc, const plVec3*& out_pPositions, plUInt32& out_uiElementStride);

  /// \brief Helper function to get the position and normal stream from the given mesh buffer descriptor
  static plResult GetPositionAndNormalStream(const plMeshBufferResourceDescriptor& meshBufferDesc, const plVec3*& out_pPositions, const plUInt8*& out_pNormals, plGALResourceFormat::Enum& out_normalFormat, plUInt32& out_uiElementStride);
};

#include <RendererCore/Meshes/Implementation/MeshBufferUtils_inl.h>
