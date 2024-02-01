#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Types/Enum.h>
#include <Texture/Image/ImageEnums.h>
#include <Texture/TexConv/TexConvEnums.h>

class plStreamWriter;
class plStreamReader;

struct PL_TEXTURE_DLL plTexFormat
{
  bool m_bSRGB = false;
  plEnum<plImageAddressMode> m_AddressModeU;
  plEnum<plImageAddressMode> m_AddressModeV;
  plEnum<plImageAddressMode> m_AddressModeW;

  // version 2
  plEnum<plTextureFilterSetting> m_TextureFilter;

  // version 3
  plInt16 m_iRenderTargetResolutionX = 0;
  plInt16 m_iRenderTargetResolutionY = 0;

  // version 4
  float m_fResolutionScale = 1.0f;

  // version 5
  int m_GalRenderTargetFormat = 0;

  void WriteTextureHeader(plStreamWriter& inout_stream) const;
  void WriteRenderTargetHeader(plStreamWriter& inout_stream) const;
  void ReadHeader(plStreamReader& inout_stream);
};
