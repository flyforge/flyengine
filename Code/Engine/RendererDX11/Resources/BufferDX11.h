
#pragma once

#include <RendererFoundation/Resources/Buffer.h>
#include <dxgi.h>

struct ID3D11Buffer;

class PLASMA_RENDERERDX11_DLL plGALBufferDX11 : public plGALBuffer
{
public:
  ID3D11Buffer* GetDXBuffer() const;

  DXGI_FORMAT GetIndexFormat() const;

protected:
  friend class plGALDeviceDX11;
  friend class plMemoryUtils;

  plGALBufferDX11(const plGALBufferCreationDescription& Description);

  virtual ~plGALBufferDX11();

  virtual plResult InitPlatform(plGALDevice* pDevice, plArrayPtr<const plUInt8> pInitialData) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  ID3D11Buffer* m_pDXBuffer;

  DXGI_FORMAT m_IndexFormat; // Only applicable for index buffers
};

#include <RendererDX11/Resources/Implementation/BufferDX11_inl.h>
