#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/RefCounted.h>

// Configure the DLL Import/Export Define
#if PLASMA_ENABLED(PLASMA_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_RENDERERFOUNDATION_LIB
#    define PLASMA_RENDERERFOUNDATION_DLL PLASMA_DECL_EXPORT
#  else
#    define PLASMA_RENDERERFOUNDATION_DLL PLASMA_DECL_IMPORT
#  endif
#else
#  define PLASMA_RENDERERFOUNDATION_DLL
#endif

// Necessary array sizes
#define PLASMA_GAL_MAX_CONSTANT_BUFFER_COUNT 16
#define PLASMA_GAL_MAX_SAMPLER_COUNT 16
#define PLASMA_GAL_MAX_VERTEX_BUFFER_COUNT 16
#define PLASMA_GAL_MAX_RENDERTARGET_COUNT 8

// Forward declarations

struct plGALDeviceCreationDescription;
struct plGALSwapChainCreationDescription;
struct plGALWindowSwapChainCreationDescription;
struct plGALShaderCreationDescription;
struct plGALTextureCreationDescription;
struct plGALBufferCreationDescription;
struct plGALDepthStencilStateCreationDescription;
struct plGALBlendStateCreationDescription;
struct plGALRasterizerStateCreationDescription;
struct plGALVertexDeclarationCreationDescription;
struct plGALQueryCreationDescription;
struct plGALSamplerStateCreationDescription;
struct plGALResourceViewCreationDescription;
struct plGALRenderTargetViewCreationDescription;
struct plGALUnorderedAccessViewCreationDescription;

class plGALSwapChain;
class plGALShader;
class plGALResourceBase;
class plGALTexture;
class plGALSharedTexture;
class plGALBuffer;
class plGALDepthStencilState;
class plGALBlendState;
class plGALRasterizerState;
class plGALRenderTargetSetup;
class plGALVertexDeclaration;
class plGALQuery;
class plGALSamplerState;
class plGALResourceView;
class plGALRenderTargetView;
class plGALUnorderedAccessView;
class plGALDevice;
class plGALPass;
class plGALCommandEncoder;
class plGALRenderCommandEncoder;
class plGALComputeCommandEncoder;

// Basic enums
struct plGALPrimitiveTopology
{
  using StorageType = plUInt8;
  enum Enum
  {
    // keep this order, it is used to allocate the desired number of indices in plMeshBufferResourceDescriptor::AllocateStreams
    Points,    // 1 index per primitive
    Lines,     // 2 indices per primitive
    Triangles, // 3 indices per primitive
    ENUM_COUNT,
    Default = Triangles
  };

  static plUInt32 VerticesPerPrimitive(plGALPrimitiveTopology::Enum e) { return (plUInt32)e + 1; }
};

struct PLASMA_RENDERERFOUNDATION_DLL plGALIndexType
{
  enum Enum
  {
    None,   // indices are not used, vertices are just used in order to form primitives
    UShort, // 16 bit indices are used to select which vertices shall form a primitive, thus meshes can only use up to 65535 vertices
    UInt,   // 32 bit indices are used to select which vertices shall form a primitive

    ENUM_COUNT
  };


  /// \brief The size in bytes of a single element of the given index format.
  static plUInt8 GetSize(plGALIndexType::Enum format) { return s_Size[format]; }

private:
  static const plUInt8 s_Size[plGALIndexType::ENUM_COUNT];
};


struct PLASMA_RENDERERFOUNDATION_DLL plGALShaderStage
{
  using StorageType = plUInt8;

  enum Enum : plUInt8
  {
    VertexShader,
    HullShader,
    DomainShader,
    GeometryShader,
    PixelShader,
    ComputeShader,
    /*
    TaskShader,
    MeshShader,
    RayGenShader,
    RayAnyHitShader,
    RayClosestHitShader,
    RayMissShader,
    RayIntersectionShader,
    */
    ENUM_COUNT,
    Default = VertexShader
  };

  static const char* Names[ENUM_COUNT];
};

struct PLASMA_RENDERERFOUNDATION_DLL plGALShaderStageFlags
{
  using StorageType = plUInt16;

  enum Enum : plUInt16
  {
    VertexShader = PLASMA_BIT(0),
    HullShader = PLASMA_BIT(1),
    DomainShader = PLASMA_BIT(2),
    GeometryShader = PLASMA_BIT(3),
    PixelShader = PLASMA_BIT(4),
    ComputeShader = PLASMA_BIT(5),
    /*
    TaskShader = PLASMA_BIT(6),
    MeshShader = PLASMA_BIT(7),
    RayGenShader = PLASMA_BIT(8),
    RayAnyHitShader = PLASMA_BIT(9),
    RayClosestHitShader = PLASMA_BIT(10),
    RayMissShader = PLASMA_BIT(11),
    RayIntersectionShader = PLASMA_BIT(12),
    */
    Default = 0
  };

  struct Bits
  {
    StorageType VertexShader : 1;
    StorageType HullShader : 1;
    StorageType DomainShader : 1;
    StorageType GeometryShader : 1;
    StorageType PixelShader : 1;
    StorageType ComputeShader : 1;
  };

  inline static plGALShaderStageFlags::Enum MakeFromShaderStage(plGALShaderStage::Enum stage)
  {
    return static_cast<plGALShaderStageFlags::Enum>(PLASMA_BIT(stage));
  }
};
PLASMA_DECLARE_FLAGS_OPERATORS(plGALShaderStageFlags);


struct PLASMA_RENDERERFOUNDATION_DLL plGALMSAASampleCount
{
  using StorageType = plUInt8;

  enum Enum
  {
    None = 1,
    TwoSamples = 2,
    FourSamples = 4,
    EightSamples = 8,

    ENUM_COUNT = 4,

    Default = None
  };
};

struct plGALTextureType
{
  using StorageType = plUInt8;

  enum Enum
  {
    Invalid = -1,
    Texture2D = 0,
    TextureCube,
    Texture3D,
    Texture2DProxy,
    Texture2DShared,

    ENUM_COUNT,

    Default = Texture2D
  };
};

struct plGALBlend
{
  using StorageType = plUInt8;

  enum Enum
  {
    Zero = 0,
    One,
    SrcColor,
    InvSrcColor,
    SrcAlpha,
    InvSrcAlpha,
    DestAlpha,
    InvDestAlpha,
    DestColor,
    InvDestColor,
    SrcAlphaSaturated,
    BlendFactor,
    InvBlendFactor,

    ENUM_COUNT,

    Default = One
  };
};

struct plGALBlendOp
{
  using StorageType = plUInt8;

  enum Enum
  {
    Add = 0,
    Subtract,
    RevSubtract,
    Min,
    Max,

    ENUM_COUNT,
    Default = Add
  };
};

struct plGALStencilOp
{
  using StorageType = plUInt8;

  enum Enum
  {
    Keep = 0,
    Zero,
    Replace,
    IncrementSaturated,
    DecrementSaturated,
    Invert,
    Increment,
    Decrement,

    ENUM_COUNT,

    Default = Keep
  };
};

struct plGALCompareFunc
{
  using StorageType = plUInt8;

  enum Enum
  {
    Never = 0,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,

    ENUM_COUNT,

    Default = Never
  };
};

/// \brief Defines which sides of a polygon gets culled by the graphics card
struct plGALCullMode
{
  using StorageType = plUInt8;

  /// \brief Defines which sides of a polygon gets culled by the graphics card
  enum Enum
  {
    None = 0,  ///< Triangles do not get culled
    Front = 1, ///< When the 'front' of a triangle is visible, it gets culled. The rasterizer state defines which side is the 'front'. See
               ///< plGALRasterizerStateCreationDescription for details.
    Back = 2,  ///< When the 'back'  of a triangle is visible, it gets culled. The rasterizer state defines which side is the 'front'. See
               ///< plGALRasterizerStateCreationDescription for details.

    ENUM_COUNT,

    Default = Back
  };
};

struct plGALTextureFilterMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    Point = 0,
    Linear,
    Anisotropic,

    Default = Linear
  };
};

struct plGALUpdateMode
{
  enum Enum
  {
    Discard,          ///< Buffer must be completely overwritten. No old data will be read. Data will not persist across frames.
    NoOverwrite,      ///< User is responsible for synchronizing access between GPU and CPU.
    CopyToTempStorage ///< Upload to temp buffer, then buffer to buffer transfer at the current time in the command buffer.
  };
};

// Basic structs
struct plGALTextureSubresource
{
  plUInt32 m_uiMipLevel = 0;
  plUInt32 m_uiArraySlice = 0;
};

struct plGALSystemMemoryDescription
{
  void* m_pData = nullptr;
  plUInt32 m_uiRowPitch = 0;
  plUInt32 m_uiSlicePitch = 0;
};

/// \brief Base class for GAL objects, stores a creation description of the object and also allows for reference counting.
template <typename CreationDescription>
class plGALObject : public plRefCounted
{
public:
  plGALObject(const CreationDescription& description)
    : m_Description(description)
  {
  }

  PLASMA_ALWAYS_INLINE const CreationDescription& GetDescription() const { return m_Description; }

protected:
  const CreationDescription m_Description;
};

// Handles
namespace plGAL
{
  using pl16_16Id = plGenericId<16, 16>;
  using pl18_14Id = plGenericId<18, 14>;
  using pl20_12Id = plGenericId<20, 12>;
} // namespace plGAL

class plGALSwapChainHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plGALSwapChainHandle, plGAL::pl16_16Id);

  friend class plGALDevice;
};

class plGALShaderHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plGALShaderHandle, plGAL::pl18_14Id);

  friend class plGALDevice;
};

class plGALTextureHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plGALTextureHandle, plGAL::pl18_14Id);

  friend class plGALDevice;
};

class plGALBufferHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plGALBufferHandle, plGAL::pl18_14Id);

  friend class plGALDevice;
};

class plGALResourceViewHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plGALResourceViewHandle, plGAL::pl18_14Id);

  friend class plGALDevice;
};

class plGALUnorderedAccessViewHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plGALUnorderedAccessViewHandle, plGAL::pl18_14Id);

  friend class plGALDevice;
};

class plGALRenderTargetViewHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plGALRenderTargetViewHandle, plGAL::pl18_14Id);

  friend class plGALDevice;
};

class plGALDepthStencilStateHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plGALDepthStencilStateHandle, plGAL::pl16_16Id);

  friend class plGALDevice;
};

class plGALBlendStateHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plGALBlendStateHandle, plGAL::pl16_16Id);

  friend class plGALDevice;
};

class plGALRasterizerStateHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plGALRasterizerStateHandle, plGAL::pl16_16Id);

  friend class plGALDevice;
};

class plGALSamplerStateHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plGALSamplerStateHandle, plGAL::pl16_16Id);

  friend class plGALDevice;
};

class plGALVertexDeclarationHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plGALVertexDeclarationHandle, plGAL::pl18_14Id);

  friend class plGALDevice;
};

class plGALQueryHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plGALQueryHandle, plGAL::pl20_12Id);

  friend class plGALDevice;
};

struct plGALTimestampHandle
{
  PLASMA_DECLARE_POD_TYPE();

  plUInt64 m_uiIndex;
  plUInt64 m_uiFrameCounter;
};

namespace plGAL
{
  struct ModifiedRange
  {
    PLASMA_ALWAYS_INLINE void Reset()
    {
      m_uiMin = plInvalidIndex;
      m_uiMax = 0;
    }

    PLASMA_FORCE_INLINE void SetToIncludeValue(plUInt32 value)
    {
      m_uiMin = plMath::Min(m_uiMin, value);
      m_uiMax = plMath::Max(m_uiMax, value);
    }

    PLASMA_FORCE_INLINE void SetToIncludeRange(plUInt32 uiMin, plUInt32 uiMax)
    {
      m_uiMin = plMath::Min(m_uiMin, uiMin);
      m_uiMax = plMath::Max(m_uiMax, uiMax);
    }

    PLASMA_ALWAYS_INLINE bool IsValid() const { return m_uiMin <= m_uiMax; }

    PLASMA_ALWAYS_INLINE plUInt32 GetCount() const { return m_uiMax - m_uiMin + 1; }

    plUInt32 m_uiMin = plInvalidIndex;
    plUInt32 m_uiMax = 0;
  };
} // namespace plGAL
