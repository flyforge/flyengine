#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>

class plShaderUtils
{
public:
  PLASMA_ALWAYS_INLINE static plUInt32 Float3ToRGB10(plVec3 value)
  {
    const plVec3 unsignedValue = value * 0.5f + plVec3(0.5f);

    const plUInt32 r = plMath::Clamp(static_cast<plUInt32>(unsignedValue.x * 1023.0f + 0.5f), 0u, 1023u);
    const plUInt32 g = plMath::Clamp(static_cast<plUInt32>(unsignedValue.y * 1023.0f + 0.5f), 0u, 1023u);
    const plUInt32 b = plMath::Clamp(static_cast<plUInt32>(unsignedValue.z * 1023.0f + 0.5f), 0u, 1023u);

    return r | (g << 10) | (b << 20);
  }

  PLASMA_ALWAYS_INLINE static plUInt32 PackFloat16intoUint(plFloat16 x, plFloat16 y)
  {
    const plUInt32 r = x.GetRawData();
    const plUInt32 g = y.GetRawData();

    return r | (g << 16);
  }

  PLASMA_ALWAYS_INLINE static plUInt32 Float2ToRG16F(plVec2 value)
  {
    const plUInt32 r = plFloat16(value.x).GetRawData();
    const plUInt32 g = plFloat16(value.y).GetRawData();

    return r | (g << 16);
  }

  PLASMA_ALWAYS_INLINE static void Float4ToRGBA16F(plVec4 value, plUInt32& out_uiRG, plUInt32& out_uiBA)
  {
    out_uiRG = Float2ToRG16F(plVec2(value.x, value.y));
    out_uiBA = Float2ToRG16F(plVec2(value.z, value.w));
  }

  enum class plBuiltinShaderType
  {
    CopyImage,
    CopyImageArray,
    DownscaleImage,
    DownscaleImageArray,
  };

  struct plBuiltinShader
  {
    plGALShaderHandle m_hActiveGALShader;
    plGALBlendStateHandle m_hBlendState;
    plGALDepthStencilStateHandle m_hDepthStencilState;
    plGALRasterizerStateHandle m_hRasterizerState;
  };

  PLASMA_RENDERERFOUNDATION_DLL static plDelegate<void(plBuiltinShaderType type, plBuiltinShader& out_shader)> g_RequestBuiltinShaderCallback;

  PLASMA_ALWAYS_INLINE static void RequestBuiltinShader(plBuiltinShaderType type, plBuiltinShader& out_shader)
  {
    g_RequestBuiltinShaderCallback(type, out_shader);
  }
};
PLASMA_DEFINE_AS_POD_TYPE(plShaderUtils::plBuiltinShaderType);
