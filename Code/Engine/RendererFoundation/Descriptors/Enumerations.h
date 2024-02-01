#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

/// \brief The type of a shader resource (plShaderResourceBinding).
/// Shader resources need to be bound to a shader for it to function. This includes samplers, constant buffers, textures etc. which are all handled by PL via GAL resource types / views. However, vertex buffers, index buffers and vertex layouts are not considered shader resources and are handled separately.
/// \sa plGALShaderTextureType, plShaderResourceBinding
struct plGALShaderResourceType
{
  using StorageType = plUInt8;
  enum Enum : plUInt8
  {
    Unknown = 0,
    /// Texture sampler (plGALSamplerStateHandle). HLSL: SamplerState, SamplerComparisonState
    Sampler,

    /// Read-only struct (plGALBufferHandle). HLSL: cbuffer, ConstantBuffer
    ConstantBuffer,
    // Read-only struct. Set directly via plGALCommandEncoder::SetPushConstants. HLSL: Use macro BEGIN_PUSH_CONSTANTS, END_PUSH_CONSTANTS, GET_PUSH_CONSTANT
    PushConstants,

    /// \name Shader Resource Views (SRVs). These are set via plGALResourceViewHandle.
    ///@{

    /// Read-only texture view. When set, plGALShaderTextureType is also set. HLSL: Texture*
    Texture,
    /// Read-only texture view with attached sampler. When set, plGALShaderTextureType is also set. HLSL: Name sampler the same as texture with _AutoSampler appended.
    TextureAndSampler,
    /// Read-only texel buffer. It's like a 1D texture. HLSL: Buffer
    TexelBuffer,
    /// Read-only array of structs. HLSL: StructuredBuffer<T>, ByteAddressBuffer.
    StructuredBuffer,

    ///@}
    /// \name Unordered Access Views (UAVs). These are set via plGALUnorderedAccessViewHandle.
    ///@{

    /// Read-write texture view. When set, plGALShaderTextureType is also set. HLSL: RWTexture*
    TextureRW,
    /// Read-write texel buffer. It's like a 1D texture. HLSL: RWBuffer
    TexelBufferRW,
    /// Read-write array of structs. HLSL: RWStructuredBuffer<T>, RWByteAddressBuffer, AppendStructuredBuffer, ConsumeStructuredBuffer
    StructuredBufferRW,

    ///@}

    // #TODO_SHADER: Future work:
    // Not supported: PL does not support AppendStructuredBuffer, ConsumeStructuredBuffer yet so while the shader can be compiled, nothing can be bound to these resources. On Vulkan, will probably need yet another type to distinguish the data from the count resource (uav_counter_binding).
    // Not supported: tbuffer, TextureBuffer, these map to CBV on DX11 and to eStorageBuffer on Vulkan, requiring to use a constantBufferView or a UAV. Thus, it bleeds platform implementation details.
    // Not supported: (Vulkan) VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, frame-buffer local read-only image view. Required for render passes on mobile.
    // Not supported: (Vulkan) VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK,Vulkan 1.3 addition, surpasses push-constants but not widely supported yet. May be able to abstract this via PushConstants and custom shader compiler / GAL implementations.
    // Not supported: (Vulkan) VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, Vulkan extension for raytracing.

    Default = Unknown
  };
};

/// \brief General category of the shader resource (plShaderResourceBinding).
/// Note that these are flags because some resources can be multiple resource types, e.g. plGALShaderResourceType::TextureAndSampler.
struct plGALShaderResourceCategory
{
  using StorageType = plUInt8;
  static constexpr int ENUM_COUNT = 4;
  enum Enum : plUInt8
  {
    Sampler = PL_BIT(0), //< Sampler (plGALSamplerStateHandle).
    ConstantBuffer = PL_BIT(1), //< Constant Buffer (plGALBufferHandle)
    SRV = PL_BIT(2), //< Shader Resource Views (plGALResourceViewHandle).
    UAV = PL_BIT(3), //< Unordered Access Views (plGALUnorderedAccessViewHandle).
    Default = 0
  };

  struct Bits
  {
    StorageType Sampler : 1;
    StorageType ConstantBuffer : 1;
    StorageType SRV : 1;
    StorageType UAV : 1;
  };

  static plBitflags<plGALShaderResourceCategory> MakeFromShaderDescriptorType(plGALShaderResourceType::Enum type);
};

PL_DECLARE_FLAGS_OPERATORS(plGALShaderResourceCategory);

/// \brief The texture type of the shader resource (plShaderResourceBinding).
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

  static bool IsArray(plGALShaderTextureType::Enum format);
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

/// \brief Defines the usage semantic of a vertex attribute.
/// \sa plGALVertexAttribute
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

/// \brief General type of buffer.
/// \sa plGALBufferCreationDescription
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

/// \brief Type of GPU->CPU query.
/// \sa plGALQueryCreationDescription
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

/// \brief Type of the shared texture (INTERNAL)
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

#include <RendererFoundation/Descriptors/Implementation/Enumerations_inl.h>
