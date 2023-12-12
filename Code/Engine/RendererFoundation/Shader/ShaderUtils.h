#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>

class plShaderUtils
{
public:
  PLASMA_ALWAYS_INLINE static plColor KelvinToRGB(float temperature)
  {
    float r, g, b;

    // Temperature must fall between 1000 and 40000 degrees
    // The fitting require to divide kelvin by 1000 (allow more precision)
    float kelvin = plMath::Clamp(temperature, 1000.0f, 40000.0f) / 1000.0f;
    float kelvin2 = kelvin * kelvin;

    // Using 6570 as a pivot is an approximation, pivot point for red is around 6580 and for blue and green around 6560.
    // Calculate each color in turn (Note, clamp is not really necessary as all value belongs to [0..1] but can help for extremum).
    // Red
    r = kelvin < 6.570f ? 1.0f : plMath::Clamp((1.35651f + 0.216422f * kelvin + 0.000633715f * kelvin2) / (-3.24223f + 0.918711f * kelvin), 0.0f, 1.0f);
    // Green
    g = kelvin < 6.570f ?
      plMath::Clamp((-399.809f + 414.271f * kelvin + 111.543f * kelvin2) / (2779.24f + 164.143f * kelvin + 84.7356f * kelvin2), 0.0f, 1.0f) :
      plMath::Clamp((1370.38f + 734.616f * kelvin + 0.689955f * kelvin2) / (-4625.69f + 1699.87f * kelvin), 0.0f, 1.0f);
    // Blue
    b = kelvin > 6.570f ? 1.0f : plMath::Clamp((348.963f - 523.53f * kelvin + 183.62f * kelvin2) / (2848.82f - 214.52f * kelvin + 78.8614f * kelvin2), 0.0f, 1.0f);

    return plColor(r, g, b, 1.0f);
  }


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
