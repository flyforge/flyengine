

#pragma once
#include <RendererFoundation/RendererFoundationDLL.h>

/// \brief This struct holds information about the rendering device capabilities (e.g. what shader stages are supported and more)
/// To get the device capabilities you need to call the GetCapabilities() function on an plGALDevice object.
struct PLASMA_RENDERERFOUNDATION_DLL plGALDeviceCapabilities
{
  plGALDeviceCapabilities();

  // Device description
  plString m_sAdapterName = "Unknown";
  plUInt64 m_uiDedicatedVRAM = 0;
  plUInt64 m_uiDedicatedSystemRAM = 0;
  plUInt64 m_uiSharedSystemRAM = 0;
  bool m_bHardwareAccelerated = false;

  // General capabilities
  bool m_bMultithreadedResourceCreation; ///< whether creating resources is allowed on other threads than the main thread
  bool m_bNoOverwriteBufferUpdate;

  // Draw related capabilities
  bool m_bShaderStageSupported[plGALShaderStage::ENUM_COUNT];
  bool m_bInstancing;
  bool m_b32BitIndices;
  bool m_bIndirectDraw;
  bool m_bStreamOut;
  bool m_bConservativeRasterization;
  bool m_bVertexShaderRenderTargetArrayIndex = false;
  plUInt16 m_uiMaxConstantBuffers;


  // Texture related capabilities
  bool m_bTextureArrays;
  bool m_bCubemapArrays;
  bool m_bB5G6R5Textures;
  plUInt16 m_uiMaxTextureDimension;
  plUInt16 m_uiMaxCubemapDimension;
  plUInt16 m_uiMax3DTextureDimension;
  plUInt16 m_uiMaxAnisotropy;


  // Output related capabilities
  plUInt16 m_uiMaxRendertargets;
  plUInt16 m_uiUAVCount;
  bool m_bAlphaToCoverage;
};
