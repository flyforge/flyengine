#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Types/Bitflags.h>

/// \brief Defines which operations can be performed on an plGALResourceFormat
/// \sa plGALDeviceCapabilities::m_FormatSupport
struct plGALResourceFormatSupport
{
  using StorageType = plUInt8;

  enum Enum
  {
    None = 0,
    Sample = PLASMA_BIT(0),          ///< The format can be sampled as a texture in a shader or in case of integer textures load can be called.
    Render = PLASMA_BIT(1),          ///< The format can be used as a render target texture.
    VertexAttribute = PLASMA_BIT(2), ///< The format can be used as a vertex attribute.
    Default = 0
  };

  struct Bits
  {
    StorageType Sample : 1;
    StorageType Render : 1;
    StorageType VertexAttribute : 1;
  };
};
PLASMA_DECLARE_FLAGS_OPERATORS(plGALResourceFormatSupport);

/// \brief This struct holds information about the rendering device capabilities (e.g. what shader stages are supported and more)
/// To get the device capabilities you need to call the GetCapabilities() function on an plGALDevice object.
struct PLASMA_RENDERERFOUNDATION_DLL plGALDeviceCapabilities
{
  // Device description
  plString m_sAdapterName = "Unknown";
  plUInt64 m_uiDedicatedVRAM = 0;
  plUInt64 m_uiDedicatedSystemRAM = 0;
  plUInt64 m_uiSharedSystemRAM = 0;
  bool m_bHardwareAccelerated = false;

  // General capabilities
  bool m_bMultithreadedResourceCreation = false; ///< whether creating resources is allowed on other threads than the main thread
  bool m_bNoOverwriteBufferUpdate = false;

  // Draw related capabilities
  bool m_bShaderStageSupported[plGALShaderStage::ENUM_COUNT] = {};
  bool m_bInstancing = false;
  bool m_b32BitIndices = false;
  bool m_bIndirectDraw = false;
  bool m_bConservativeRasterization = false;
  bool m_bVertexShaderRenderTargetArrayIndex = false;
  plUInt16 m_uiMaxConstantBuffers = 0;


  // Texture related capabilities
  bool m_bTextureArrays = false;
  bool m_bCubemapArrays = false;
  bool m_bSharedTextures = false;
  plUInt16 m_uiMaxTextureDimension = 0;
  plUInt16 m_uiMaxCubemapDimension = 0;
  plUInt16 m_uiMax3DTextureDimension = 0;
  plUInt16 m_uiMaxAnisotropy = 0;
  plDynamicArray<plBitflags<plGALResourceFormatSupport>> m_FormatSupport;

  // Output related capabilities
  plUInt16 m_uiMaxRendertargets = 0;
  plUInt16 m_uiUAVCount = 0;
  bool m_bAlphaToCoverage = false;
};
