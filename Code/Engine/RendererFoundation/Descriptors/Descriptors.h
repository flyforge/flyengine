
#pragma once

#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Types/SharedPtr.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Resources/ResourceFormats.h>
#include <RendererFoundation/Shader/ShaderByteCode.h>
#include <RendererFoundation/Descriptors/Enumerations.h>
#include <Texture/Image/ImageEnums.h>

class plWindowBase;



struct plGALWindowSwapChainCreationDescription : public plHashableStruct<plGALWindowSwapChainCreationDescription>
{
  plWindowBase* m_pWindow = nullptr;

  // Describes the format that should be used for the backbuffer.
  // Note however, that different platforms may enforce restrictions on this.
  plGALMSAASampleCount::Enum m_SampleCount = plGALMSAASampleCount::None;
  plGALResourceFormat::Enum m_BackBufferFormat = plGALResourceFormat::RGBAUByteNormalizedsRGB;
  plEnum<plGALPresentMode> m_InitialPresentMode = plGALPresentMode::VSync;

  bool m_bDoubleBuffered = true;
  bool m_bAllowScreenshots = false;
};

struct plGALSwapChainCreationDescription : public plHashableStruct<plGALSwapChainCreationDescription>
{
  const plRTTI* m_pSwapChainType = nullptr;
};

struct plGALDeviceCreationDescription
{
  bool m_bDebugDevice = false;
};

struct plGALShaderCreationDescription : public plHashableStruct<plGALShaderCreationDescription>
{
  plGALShaderCreationDescription();
  ~plGALShaderCreationDescription();

  bool HasByteCodeForStage(plGALShaderStage::Enum stage) const;

  plSharedPtr<plGALShaderByteCode> m_ByteCodes[plGALShaderStage::ENUM_COUNT];
};

struct plGALRenderTargetBlendDescription : public plHashableStruct<plGALRenderTargetBlendDescription>
{
  plEnum<plGALBlend> m_SourceBlend = plGALBlend::One;
  plEnum<plGALBlend> m_DestBlend = plGALBlend::One;
  plEnum<plGALBlendOp> m_BlendOp = plGALBlendOp::Add;

  plEnum<plGALBlend> m_SourceBlendAlpha = plGALBlend::One;
  plEnum<plGALBlend> m_DestBlendAlpha = plGALBlend::One;
  plEnum<plGALBlendOp> m_BlendOpAlpha = plGALBlendOp::Add;

  plUInt8 m_uiWriteMask = 0xFF;    ///< Enables writes to color channels. Bit1 = Red Channel, Bit2 = Green Channel, Bit3 = Blue Channel, Bit4 = Alpha
                                   ///< Channel, Bit 5-8 are unused
  bool m_bBlendingEnabled = false; ///< If enabled, the color will be blended into the render target. Otherwise it will overwrite the render target.
                                   ///< Set m_uiWriteMask to 0 to disable all writes to the render target.
};

struct plGALBlendStateCreationDescription : public plHashableStruct<plGALBlendStateCreationDescription>
{
  plGALRenderTargetBlendDescription m_RenderTargetBlendDescriptions[PL_GAL_MAX_RENDERTARGET_COUNT];

  bool m_bAlphaToCoverage = false;  ///< Alpha-to-coverage can only be used with MSAA render targets. Default is false.
  bool m_bIndependentBlend = false; ///< If disabled, the blend state of the first render target is used for all render targets. Otherwise each
                                    ///< render target uses a different blend state.
};

struct plGALStencilOpDescription : public plHashableStruct<plGALStencilOpDescription>
{
  plEnum<plGALStencilOp> m_FailOp = plGALStencilOp::Keep;
  plEnum<plGALStencilOp> m_DepthFailOp = plGALStencilOp::Keep;
  plEnum<plGALStencilOp> m_PassOp = plGALStencilOp::Keep;

  plEnum<plGALCompareFunc> m_StencilFunc = plGALCompareFunc::Always;
};

struct plGALDepthStencilStateCreationDescription : public plHashableStruct<plGALDepthStencilStateCreationDescription>
{
  plGALStencilOpDescription m_FrontFaceStencilOp;
  plGALStencilOpDescription m_BackFaceStencilOp;

  plEnum<plGALCompareFunc> m_DepthTestFunc = plGALCompareFunc::Less;

  bool m_bSeparateFrontAndBack = false; ///< If false, DX11 will use front face values for both front & back face values, GL will not call
                                        ///< gl*Separate() funcs
  bool m_bDepthTest = true;
  bool m_bDepthWrite = true;
  bool m_bStencilTest = false;
  plUInt8 m_uiStencilReadMask = 0xFF;
  plUInt8 m_uiStencilWriteMask = 0xFF;
};

/// \brief Describes the settings for a new rasterizer state. See plGALDevice::CreateRasterizerState
struct plGALRasterizerStateCreationDescription : public plHashableStruct<plGALRasterizerStateCreationDescription>
{
  plEnum<plGALCullMode> m_CullMode = plGALCullMode::Back; ///< Which sides of a triangle to cull. Default is plGALCullMode::Back
  plInt32 m_iDepthBias = 0;                               ///< The pixel depth bias. Default is 0
  float m_fDepthBiasClamp = 0.0f;                         ///< The pixel depth bias clamp. Default is 0
  float m_fSlopeScaledDepthBias = 0.0f;                   ///< The pixel slope scaled depth bias clamp. Default is 0
  bool m_bWireFrame = false;                              ///< Whether triangles are rendered filled or as wireframe. Default is false
  bool m_bFrontCounterClockwise = false;                  ///< Sets which triangle winding order defines the 'front' of a triangle. If true, the front of a triangle
                                                          ///< is the one where the vertices appear in counter clockwise order. Default is false
  bool m_bScissorTest = false;
  bool m_bConservativeRasterization = false; ///< Whether conservative rasterization is enabled
};

struct plGALSamplerStateCreationDescription : public plHashableStruct<plGALSamplerStateCreationDescription>
{
  plEnum<plGALTextureFilterMode> m_MinFilter;
  plEnum<plGALTextureFilterMode> m_MagFilter;
  plEnum<plGALTextureFilterMode> m_MipFilter;

  plEnum<plImageAddressMode> m_AddressU;
  plEnum<plImageAddressMode> m_AddressV;
  plEnum<plImageAddressMode> m_AddressW;

  plEnum<plGALCompareFunc> m_SampleCompareFunc;

  plColor m_BorderColor = plColor::Black;

  float m_fMipLodBias = 0.0f;
  float m_fMinMip = -1.0f;
  float m_fMaxMip = 42000.0f;

  plUInt32 m_uiMaxAnisotropy = 4;
};

struct PL_RENDERERFOUNDATION_DLL plGALVertexAttribute
{
  plGALVertexAttribute() = default;

  plGALVertexAttribute(plGALVertexAttributeSemantic::Enum semantic, plGALResourceFormat::Enum format, plUInt16 uiOffset, plUInt8 uiVertexBufferSlot,
    bool bInstanceData);

  plGALVertexAttributeSemantic::Enum m_eSemantic = plGALVertexAttributeSemantic::Position;
  plGALResourceFormat::Enum m_eFormat = plGALResourceFormat::XYZFloat;
  plUInt16 m_uiOffset = 0;
  plUInt8 m_uiVertexBufferSlot = 0;
  bool m_bInstanceData = false;
};

struct PL_RENDERERFOUNDATION_DLL plGALVertexDeclarationCreationDescription : public plHashableStruct<plGALVertexDeclarationCreationDescription>
{
  plGALShaderHandle m_hShader;
  plStaticArray<plGALVertexAttribute, 16> m_VertexAttributes;
};

// Need to add: immutable (GPU only), default(GPU only, but allows CopyToTempStorage updates), transient (allows plGALUpdateMode::Discard), staging: read(back), staging: write (constantly mapped), unified memory (mobile, onboard GPU, allows all ops)
// Or use VmaMemoryUsage  + read write flags?
struct plGALResourceAccess
{
  PL_ALWAYS_INLINE bool IsImmutable() const { return m_bImmutable; }

  bool m_bReadBack = false;
  bool m_bImmutable = true;
};

struct plGALBufferCreationDescription : public plHashableStruct<plGALBufferCreationDescription>
{
  plUInt32 m_uiStructSize = 0;
  plUInt32 m_uiTotalSize = 0;

  plEnum<plGALBufferType> m_BufferType = plGALBufferType::Generic;

  bool m_bUseForIndirectArguments = false;
  bool m_bUseAsStructuredBuffer = false;
  bool m_bAllowRawViews = false;
  bool m_bAllowShaderResourceView = false;
  bool m_bAllowUAV = false;

  plGALResourceAccess m_ResourceAccess;
};

struct plGALTextureCreationDescription : public plHashableStruct<plGALTextureCreationDescription>
{
  void SetAsRenderTarget(plUInt32 uiWidth, plUInt32 uiHeight, plGALResourceFormat::Enum format, plGALMSAASampleCount::Enum sampleCount = plGALMSAASampleCount::None);

  plUInt32 m_uiWidth = 0;
  plUInt32 m_uiHeight = 0;
  plUInt32 m_uiDepth = 1;
  plUInt32 m_uiMipLevelCount = 1;
  plUInt32 m_uiArraySize = 1;

  plEnum<plGALResourceFormat> m_Format = plGALResourceFormat::Invalid;
  plEnum<plGALMSAASampleCount> m_SampleCount = plGALMSAASampleCount::None;
  plEnum<plGALTextureType> m_Type = plGALTextureType::Texture2D;

  bool m_bAllowShaderResourceView = true;
  bool m_bAllowUAV = false;
  bool m_bCreateRenderTarget = false;
  bool m_bAllowDynamicMipGeneration = false;

  plGALResourceAccess m_ResourceAccess;

  void* m_pExisitingNativeObject = nullptr; ///< Can be used to encapsulate existing native textures in objects usable by the GAL
};

struct plGALResourceViewCreationDescription : public plHashableStruct<plGALResourceViewCreationDescription>
{
  plGALTextureHandle m_hTexture;

  plGALBufferHandle m_hBuffer;

  plEnum<plGALResourceFormat> m_OverrideViewFormat = plGALResourceFormat::Invalid;

  // Texture only
  plUInt32 m_uiMostDetailedMipLevel = 0;
  plUInt32 m_uiMipLevelsToUse = 0xFFFFFFFFu;

  plUInt32 m_uiFirstArraySlice = 0; // For cubemap array: index of first 2d slice to start with
  plUInt32 m_uiArraySize = 1;       // For cubemap array: number of cubemaps

  // Buffer only
  plUInt32 m_uiFirstElement = 0;
  plUInt32 m_uiNumElements = 0;
  bool m_bRawView = false;
};

struct plGALRenderTargetViewCreationDescription : public plHashableStruct<plGALRenderTargetViewCreationDescription>
{
  plGALTextureHandle m_hTexture;

  plEnum<plGALResourceFormat> m_OverrideViewFormat = plGALResourceFormat::Invalid;

  plUInt32 m_uiMipLevel = 0;

  plUInt32 m_uiFirstSlice = 0;
  plUInt32 m_uiSliceCount = 1;

  bool m_bReadOnly = false; ///< Can be used for depth stencil views to create read only views (e.g. for soft particles using the native depth buffer)
};

struct plGALUnorderedAccessViewCreationDescription : public plHashableStruct<plGALUnorderedAccessViewCreationDescription>
{
  plGALTextureHandle m_hTexture;

  plGALBufferHandle m_hBuffer;

  plEnum<plGALResourceFormat> m_OverrideViewFormat = plGALResourceFormat::Invalid;

  // Texture only
  plUInt32 m_uiMipLevelToUse = 0;   ///< Which MipLevel is accessed with this UAV
  plUInt32 m_uiFirstArraySlice = 0; ///< First depth slice for 3D Textures.
  plUInt32 m_uiArraySize = 1;       ///< Number of depth slices for 3D textures.

  // Buffer only
  plUInt32 m_uiFirstElement = 0;
  plUInt32 m_uiNumElements = 0;
  bool m_bRawView = false;
  bool m_bAppend = false; // Allows appending data to the end of the buffer.
};

struct plGALQueryCreationDescription : public plHashableStruct<plGALQueryCreationDescription>
{
  plEnum<plGALQueryType> m_type = plGALQueryType::NumSamplesPassed;

  /// In case this query is used for occlusion culling (type AnySamplesPassed), this determines whether drawing should be done if the query
  /// status is still unknown.
  bool m_bDrawIfUnknown = true;
};

/// \brief Type for important GAL events.
struct plGALDeviceEvent
{
  enum Type
  {
    AfterInit,
    BeforeShutdown,
    BeforeBeginFrame,
    AfterBeginFrame,
    BeforeEndFrame,
    AfterEndFrame,
    // could add resource creation/destruction events, if this would be useful
  };

  Type m_Type;
  class plGALDevice* m_pDevice;
};

// Opaque platform specific handle
// Typically holds a platform specific handle for the texture and it's synchronization primitive
struct plGALPlatformSharedHandle : public plHashableStruct<plGALPlatformSharedHandle>
{
  plUInt64 m_hSharedTexture = 0;
  plUInt64 m_hSemaphore = 0;
  plUInt32 m_uiProcessId = 0;
  plUInt32 m_uiMemoryTypeIndex = 0;
  plUInt64 m_uiSize = 0;
};

#include <RendererFoundation/Descriptors/Implementation/Descriptors_inl.h>
