#pragma once

#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Pipeline/Declarations.h>

struct PLASMA_RENDERERCORE_DLL plViewRenderMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    None,
    WireframeColor,
    WireframeMonochrome,
    DiffuseLitOnly,
    SpecularLitOnly,
    LightCount,
    DecalCount,
    TexCoordsUV0,
    TexCoordsUV1,
    VertexColors0,
    VertexColors1,
    VertexNormals,
    VertexTangents,
    PixelNormals,
    DiffuseColor,
    DiffuseColorRange,
    SpecularColor,
    EmissiveColor,
    Roughness,
    Occlusion,
    Depth,
    StaticVsDynamic,
    BoneWeights,

    ENUM_COUNT,

    Default = None
  };

  static plTempHashedString GetPermutationValue(Enum renderMode);
  static int GetRenderPassForShader(Enum renderMode);
  static void GetDebugText(Enum renderMode, plStringBuilder& out_sDebugText);
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RENDERERCORE_DLL, plViewRenderMode);
