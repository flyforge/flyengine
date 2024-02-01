#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/ViewRenderMode.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/GlobalConstants.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plViewRenderMode, 1)
  PL_ENUM_CONSTANT(plViewRenderMode::None)->AddAttributes(new plGroupAttribute("Default")),
  PL_ENUM_CONSTANT(plViewRenderMode::WireframeColor)->AddAttributes(new plGroupAttribute("Wireframe")),
  PL_ENUM_CONSTANT(plViewRenderMode::WireframeMonochrome),
  PL_ENUM_CONSTANT(plViewRenderMode::DiffuseLitOnly)->AddAttributes(new plGroupAttribute("Lighting")),
  PL_ENUM_CONSTANT(plViewRenderMode::SpecularLitOnly),
  PL_ENUM_CONSTANT(plViewRenderMode::LightCount)->AddAttributes(new plGroupAttribute("Performance")),
  PL_ENUM_CONSTANT(plViewRenderMode::DecalCount),
  PL_ENUM_CONSTANT(plViewRenderMode::StaticVsDynamic),
  PL_ENUM_CONSTANT(plViewRenderMode::TexCoordsUV0)->AddAttributes(new plGroupAttribute("TexCoords")),
  PL_ENUM_CONSTANT(plViewRenderMode::TexCoordsUV1),
  PL_ENUM_CONSTANT(plViewRenderMode::VertexColors0)->AddAttributes(new plGroupAttribute("VertexColors")),
  PL_ENUM_CONSTANT(plViewRenderMode::VertexColors1),
  PL_ENUM_CONSTANT(plViewRenderMode::VertexNormals)->AddAttributes(new plGroupAttribute("Normals")),
  PL_ENUM_CONSTANT(plViewRenderMode::VertexTangents),
  PL_ENUM_CONSTANT(plViewRenderMode::PixelNormals),
  PL_ENUM_CONSTANT(plViewRenderMode::DiffuseColor)->AddAttributes(new plGroupAttribute("PixelColors")),
  PL_ENUM_CONSTANT(plViewRenderMode::DiffuseColorRange),
  PL_ENUM_CONSTANT(plViewRenderMode::SpecularColor),
  PL_ENUM_CONSTANT(plViewRenderMode::EmissiveColor),
  PL_ENUM_CONSTANT(plViewRenderMode::Roughness)->AddAttributes(new plGroupAttribute("Surface")),
  PL_ENUM_CONSTANT(plViewRenderMode::Occlusion),
  PL_ENUM_CONSTANT(plViewRenderMode::Depth),
  PL_ENUM_CONSTANT(plViewRenderMode::BoneWeights)->AddAttributes(new plGroupAttribute("Animation")),
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
plTempHashedString plViewRenderMode::GetPermutationValue(Enum renderMode)
{
  if (renderMode >= WireframeColor && renderMode <= WireframeMonochrome)
  {
    return "RENDER_PASS_WIREFRAME";
  }
  else if (renderMode >= DiffuseLitOnly && renderMode < ENUM_COUNT)
  {
    return "RENDER_PASS_EDITOR";
  }

  return "";
}

// static
int plViewRenderMode::GetRenderPassForShader(Enum renderMode)
{
  switch (renderMode)
  {
    case plViewRenderMode::None:
      return -1;

    case plViewRenderMode::WireframeColor:
      return WIREFRAME_RENDER_PASS_COLOR;

    case plViewRenderMode::WireframeMonochrome:
      return WIREFRAME_RENDER_PASS_MONOCHROME;

    case plViewRenderMode::DiffuseLitOnly:
      return EDITOR_RENDER_PASS_DIFFUSE_LIT_ONLY;

    case plViewRenderMode::SpecularLitOnly:
      return EDITOR_RENDER_PASS_SPECULAR_LIT_ONLY;

    case plViewRenderMode::LightCount:
      return EDITOR_RENDER_PASS_LIGHT_COUNT;

    case plViewRenderMode::DecalCount:
      return EDITOR_RENDER_PASS_DECAL_COUNT;

    case plViewRenderMode::TexCoordsUV0:
      return EDITOR_RENDER_PASS_TEXCOORDS_UV0;

    case plViewRenderMode::TexCoordsUV1:
      return EDITOR_RENDER_PASS_TEXCOORDS_UV1;

    case plViewRenderMode::VertexColors0:
      return EDITOR_RENDER_PASS_VERTEX_COLORS0;

    case plViewRenderMode::VertexColors1:
      return EDITOR_RENDER_PASS_VERTEX_COLORS1;

    case plViewRenderMode::VertexNormals:
      return EDITOR_RENDER_PASS_VERTEX_NORMALS;

    case plViewRenderMode::VertexTangents:
      return EDITOR_RENDER_PASS_VERTEX_TANGENTS;

    case plViewRenderMode::PixelNormals:
      return EDITOR_RENDER_PASS_PIXEL_NORMALS;

    case plViewRenderMode::DiffuseColor:
      return EDITOR_RENDER_PASS_DIFFUSE_COLOR;

    case plViewRenderMode::DiffuseColorRange:
      return EDITOR_RENDER_PASS_DIFFUSE_COLOR_RANGE;

    case plViewRenderMode::SpecularColor:
      return EDITOR_RENDER_PASS_SPECULAR_COLOR;

    case plViewRenderMode::EmissiveColor:
      return EDITOR_RENDER_PASS_EMISSIVE_COLOR;

    case plViewRenderMode::Roughness:
      return EDITOR_RENDER_PASS_ROUGHNESS;

    case plViewRenderMode::Occlusion:
      return EDITOR_RENDER_PASS_OCCLUSION;

    case plViewRenderMode::Depth:
      return EDITOR_RENDER_PASS_DEPTH;

    case plViewRenderMode::StaticVsDynamic:
      return EDITOR_RENDER_PASS_STATIC_VS_DYNAMIC;

    case plViewRenderMode::BoneWeights:
      return EDITOR_RENDER_PASS_BONE_WEIGHTS;

    default:
      PL_ASSERT_NOT_IMPLEMENTED;
      return -1;
  }
}

// static
void plViewRenderMode::GetDebugText(Enum renderMode, plStringBuilder& out_sDebugText)
{
  if (renderMode == DiffuseColorRange)
  {
    out_sDebugText = "Pure magenta means the diffuse color is too dark, pure green means it is too bright.";
  }
  else if (renderMode == StaticVsDynamic)
  {
    out_sDebugText = "Static objects are shown in green, dynamic objects are shown in red.";
  }
}



PL_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_ViewRenderMode);
