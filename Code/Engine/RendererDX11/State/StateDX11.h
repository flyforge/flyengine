
#pragma once

#include <RendererFoundation/State/State.h>


struct ID3D11BlendState;
struct ID3D11DepthStencilState;
struct ID3D11RasterizerState;
struct ID3D11RasterizerState2;
struct ID3D11SamplerState;

class PLASMA_RENDERERDX11_DLL plGALBlendStateDX11 : public plGALBlendState
{
public:
  PLASMA_ALWAYS_INLINE ID3D11BlendState* GetDXBlendState() const;

protected:
  friend class plGALDeviceDX11;
  friend class plMemoryUtils;

  plGALBlendStateDX11(const plGALBlendStateCreationDescription& Description);

  ~plGALBlendStateDX11();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  ID3D11BlendState* m_pDXBlendState = nullptr;
};

class PLASMA_RENDERERDX11_DLL plGALDepthStencilStateDX11 : public plGALDepthStencilState
{
public:
  PLASMA_ALWAYS_INLINE ID3D11DepthStencilState* GetDXDepthStencilState() const;

protected:
  friend class plGALDeviceDX11;
  friend class plMemoryUtils;

  plGALDepthStencilStateDX11(const plGALDepthStencilStateCreationDescription& Description);

  ~plGALDepthStencilStateDX11();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  ID3D11DepthStencilState* m_pDXDepthStencilState = nullptr;
};

class PLASMA_RENDERERDX11_DLL plGALRasterizerStateDX11 : public plGALRasterizerState
{
public:
  PLASMA_ALWAYS_INLINE ID3D11RasterizerState* GetDXRasterizerState() const;

protected:
  friend class plGALDeviceDX11;
  friend class plMemoryUtils;

  plGALRasterizerStateDX11(const plGALRasterizerStateCreationDescription& Description);

  ~plGALRasterizerStateDX11();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  ID3D11RasterizerState* m_pDXRasterizerState = nullptr;
};

class PLASMA_RENDERERDX11_DLL plGALSamplerStateDX11 : public plGALSamplerState
{
public:
  PLASMA_ALWAYS_INLINE ID3D11SamplerState* GetDXSamplerState() const;

protected:
  friend class plGALDeviceDX11;
  friend class plMemoryUtils;

  plGALSamplerStateDX11(const plGALSamplerStateCreationDescription& Description);

  ~plGALSamplerStateDX11();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  ID3D11SamplerState* m_pDXSamplerState = nullptr;
};


#include <RendererDX11/State/Implementation/StateDX11_inl.h>
