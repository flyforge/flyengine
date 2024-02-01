#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <RendererCore/Declarations.h>

//////////////////////////////////////////////////////////////////////////
// plShaderBindFlags
//////////////////////////////////////////////////////////////////////////

struct PL_RENDERERCORE_DLL plShaderBindFlags
{
  using StorageType = plUInt32;

  enum Enum
  {
    None = 0,                ///< No flags causes the default shader binding behavior (all render states are applied)
    ForceRebind = PL_BIT(0), ///< Executes shader binding (and state setting), even if the shader hasn't changed. Use this, when the same shader was
                             ///< previously used with custom bound states
    NoRasterizerState =
      PL_BIT(1), ///< The rasterizer state that is associated with the shader will not be bound. Use this when you intend to bind a custom rasterizer
    NoDepthStencilState = PL_BIT(
      2), ///< The depth-stencil state that is associated with the shader will not be bound. Use this when you intend to bind a custom depth-stencil
    NoBlendState =
      PL_BIT(3), ///< The blend state that is associated with the shader will not be bound. Use this when you intend to bind a custom blend
    NoStateBinding = NoRasterizerState | NoDepthStencilState | NoBlendState,

    Default = None
  };

  struct Bits
  {
    StorageType ForceRebind : 1;
    StorageType NoRasterizerState : 1;
    StorageType NoDepthStencilState : 1;
    StorageType NoBlendState : 1;
  };
};

PL_DECLARE_FLAGS_OPERATORS(plShaderBindFlags);

//////////////////////////////////////////////////////////////////////////
// plRenderContextFlags
//////////////////////////////////////////////////////////////////////////

struct PL_RENDERERCORE_DLL plRenderContextFlags
{
  using StorageType = plUInt32;

  enum Enum
  {
    None = 0,
    ShaderStateChanged = PL_BIT(0),
    TextureBindingChanged = PL_BIT(1),
    UAVBindingChanged = PL_BIT(2),
    SamplerBindingChanged = PL_BIT(3),
    BufferBindingChanged = PL_BIT(4),
    ConstantBufferBindingChanged = PL_BIT(5),
    MeshBufferBindingChanged = PL_BIT(6),
    MaterialBindingChanged = PL_BIT(7),

    AllStatesInvalid = ShaderStateChanged | TextureBindingChanged | UAVBindingChanged | SamplerBindingChanged | BufferBindingChanged |
                       ConstantBufferBindingChanged | MeshBufferBindingChanged,
    Default = None
  };

  struct Bits
  {
    StorageType ShaderStateChanged : 1;
    StorageType TextureBindingChanged : 1;
    StorageType UAVBindingChanged : 1;
    StorageType SamplerBindingChanged : 1;
    StorageType BufferBindingChanged : 1;
    StorageType ConstantBufferBindingChanged : 1;
    StorageType MeshBufferBindingChanged : 1;
    StorageType MaterialBindingChanged : 1;
  };
};

PL_DECLARE_FLAGS_OPERATORS(plRenderContextFlags);

//////////////////////////////////////////////////////////////////////////
// plDefaultSamplerFlags
//////////////////////////////////////////////////////////////////////////

struct PL_RENDERERCORE_DLL plDefaultSamplerFlags
{
  using StorageType = plUInt32;

  enum Enum
  {
    PointFiltering = 0,
    LinearFiltering = PL_BIT(0),

    Wrap = 0,
    Clamp = PL_BIT(1)
  };

  struct Bits
  {
    StorageType LinearFiltering : 1;
    StorageType Clamp : 1;
  };
};

PL_DECLARE_FLAGS_OPERATORS(plDefaultSamplerFlags);
