#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>



struct plGALShaderDescriptorType
{
  using StorageType = plUInt8;
  enum Enum : plUInt8
  {
    Unknown = 0,

    // ## Sampler -> pImageInfo (sampler)
    // HLSL: SamplerState, SamplerComparisonState
    Sampler, //  eSampler = VK_DESCRIPTOR_TYPE_SAMPLER, // Image sampler

    // ## CBV Buffer -> pBufferInfo
    // HLSL: cbuffer, ConstantBuffer
    ConstantBuffer, // eUniformBuffer = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,                // Read-only struct (constant buffer)

    // READ-ONLY = SRV
    // ## SRV Image -> pImageInfo
    // HLSL: Texture*
    Texture, // eSampledImage = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,// Read-only image view. Requires VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT. Any layout except for render target
    // ## SRV Texel Buffer -> pTexelBufferView
    // HLSL: Buffer
    TexelBuffer, // eUniformTexelBuffer = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, // Read-only linear texel buffer with view. Requires VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT
    // ## SRV Buffer -> pBufferInfo
    // HLSL: StructuredBuffer, ByteAddressBuffer
    StructuredBuffer, // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // Read / write struct (UAV).

    // READ-WRITE = UAV
    // ## UAV Image -> pImageInfo
    // HLSL: RWTexture*
    TextureRW, // eStorageImage = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, // Read / write image view. Requires VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT, General Layout
    // ## UAV Texel Buffer -> pTexelBufferView
    // HLSL: RWBuffer
    TexelBufferRW, // eStorageTexelBuffer = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, // Read / write linear texel buffer with view. Requires VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT
    // ## UAV Buffer -> pBufferInfo
    // HLSL: RWStructuredBuffer, RWByteAddressBuffer, AppendStructuredBuffer, ConsumeStructuredBuffer
    StructuredBufferRW, // eStorageBuffer = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // Read / write struct (UAV).

    // spirv UAV uav_counter_binding ??
    // Not supported: tbuffer, TextureBuffer, these map to CBV on DX11 and to eStorageBuffer on Vulkan, requiring to use a contantBufferView or a UAV so it bleeds impl details
    // eCombinedImageSampler = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    // eInputAttachment = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, //frame-buffer local read-only image view. Required for render passes
    // eInlineUniformBlock = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK, // Vulkan 1.3 addition, surpasses push-constants
    // eAccelerationStructureKHR = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, // Vulkan extension, raytracing
    // PushConstants

    Default = Unknown
  };
};

/// \brief
struct plGALShaderResourceType
{
  using StorageType = plUInt8;
  enum Enum : plUInt8
  {
    Sampler,
    ConstantBuffer,
    SRV,
    UAV,
    ENUM_COUNT,
    Default = Sampler
  };

  inline static plGALShaderResourceType::Enum MakeFromShaderDescriptorType(plGALShaderDescriptorType::Enum type)
  {
    switch (type)
    {
      case plGALShaderDescriptorType::Sampler:
        return plGALShaderResourceType::Sampler;
      case plGALShaderDescriptorType::ConstantBuffer:
        return plGALShaderResourceType::ConstantBuffer;
      case plGALShaderDescriptorType::Texture:
      case plGALShaderDescriptorType::TexelBuffer:
      case plGALShaderDescriptorType::StructuredBuffer:
        return plGALShaderResourceType::SRV;
      case plGALShaderDescriptorType::TextureRW:
      case plGALShaderDescriptorType::TexelBufferRW:
      case plGALShaderDescriptorType::StructuredBufferRW:
        return plGALShaderResourceType::UAV;
      default:
        PLASMA_REPORT_FAILURE("Missing enum");
        return plGALShaderResourceType::ConstantBuffer;
    }
  }
};

struct plGALShaderTextureType
{
  using StorageType = plUInt8;
  enum Enum : plUInt8
  {
    Unknown = 0,
    Texture1D = 1,
    Texture1DArray = 2,
    Texture2D = 3,
    Texture2DArray = 4,
    Texture2DMS = 5,
    Texture2DMSArray = 6,
    Texture3D = 7,
    TextureCube = 8,
    TextureCubeArray = 9,

    Default = Unknown
  };

  static inline bool IsArray(plGALShaderTextureType::Enum format)
  {
    switch (format)
    {
      case plGALShaderTextureType::Texture1DArray:
      case plGALShaderTextureType::Texture2DArray:
      case plGALShaderTextureType::Texture2DMSArray:
      case plGALShaderTextureType::TextureCubeArray:
        return true;
      default:
        return false;
    }
  }
};


/// \brief Defines a swap chain's present mode.
/// \sa plGALWindowSwapChainCreationDescription
struct plGALPresentMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    Immediate,
    VSync,
    ENUM_COUNT,
    Default = VSync
  };
};


struct plGALVertexAttributeSemantic
{
  using StorageType = plUInt8;

  enum Enum : plUInt8
  {
    Position,
    Normal,
    Tangent,
    Color0,
    Color1,
    Color2,
    Color3,
    Color4,
    Color5,
    Color6,
    Color7,
    TexCoord0,
    TexCoord1,
    TexCoord2,
    TexCoord3,
    TexCoord4,
    TexCoord5,
    TexCoord6,
    TexCoord7,
    TexCoord8,
    TexCoord9,

    BiTangent,
    BoneIndices0,
    BoneIndices1,
    BoneWeights0,
    BoneWeights1,

    ENUM_COUNT,
    Default = Position
  };
};

struct plGALBufferType
{
  using StorageType = plUInt8;

  enum Enum
  {
    Generic = 0,
    VertexBuffer,
    IndexBuffer,
    ConstantBuffer,

    ENUM_COUNT,

    Default = Generic
  };
};

struct plGALQueryType
{
  using StorageType = plUInt8;

  enum Enum
  {
    /// Number of samples that passed the depth and stencil test between begin and end (on a context).
    NumSamplesPassed,
    /// Boolean version of NumSamplesPassed.
    AnySamplesPassed,

    Default = NumSamplesPassed

    // Note:
    // GALFence provides an implementation of "event queries".
  };
};


// Type of the shared texture
struct plGALSharedTextureType
{
  using StorageType = plUInt8;

  enum Enum : plUInt8
  {
    None,     ///< Not shared
    Exported, ///< Allocation owned by this process
    Imported, ///< Allocation owned by a different process
    Default = None
  };
};
