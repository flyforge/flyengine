
#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>

struct ID3D11InputLayout;

class plGALVertexDeclarationDX11 : public plGALVertexDeclaration
{
public:
  PL_ALWAYS_INLINE ID3D11InputLayout* GetDXInputLayout() const;

protected:
  friend class plGALDeviceDX11;
  friend class plMemoryUtils;

  virtual plResult InitPlatform(plGALDevice* pDevice) override;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  plGALVertexDeclarationDX11(const plGALVertexDeclarationCreationDescription& Description);

  virtual ~plGALVertexDeclarationDX11();

  ID3D11InputLayout* m_pDXInputLayout = nullptr;
};

#include <RendererDX11/Shader/Implementation/VertexDeclarationDX11_inl.h>
