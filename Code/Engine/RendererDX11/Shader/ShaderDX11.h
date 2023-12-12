
#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/Shader.h>

struct ID3D11VertexShader;
struct ID3D11HullShader;
struct ID3D11DomainShader;
struct ID3D11GeometryShader;
struct ID3D11PixelShader;
struct ID3D11ComputeShader;

class PLASMA_RENDERERDX11_DLL plGALShaderDX11 : public plGALShader
{
public:
  void SetDebugName(const char* szName) const override;

  PLASMA_ALWAYS_INLINE ID3D11VertexShader* GetDXVertexShader() const;

  PLASMA_ALWAYS_INLINE ID3D11HullShader* GetDXHullShader() const;

  PLASMA_ALWAYS_INLINE ID3D11DomainShader* GetDXDomainShader() const;

  PLASMA_ALWAYS_INLINE ID3D11GeometryShader* GetDXGeometryShader() const;

  PLASMA_ALWAYS_INLINE ID3D11PixelShader* GetDXPixelShader() const;

  PLASMA_ALWAYS_INLINE ID3D11ComputeShader* GetDXComputeShader() const;


protected:
  friend class plGALDeviceDX11;
  friend class plMemoryUtils;

  plGALShaderDX11(const plGALShaderCreationDescription& description);

  virtual ~plGALShaderDX11();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  ID3D11VertexShader* m_pVertexShader;
  ID3D11HullShader* m_pHullShader;
  ID3D11DomainShader* m_pDomainShader;
  ID3D11GeometryShader* m_pGeometryShader;
  ID3D11PixelShader* m_pPixelShader;
  ID3D11ComputeShader* m_pComputeShader;
};

#include <RendererDX11/Shader/Implementation/ShaderDX11_inl.h>
