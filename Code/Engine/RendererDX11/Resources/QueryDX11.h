#pragma once

#include <RendererFoundation/Resources/Query.h>

struct ID3D11Query;

class plGALQueryDX11 : public plGALQuery
{
public:
  PL_ALWAYS_INLINE ID3D11Query* GetDXQuery() const;

protected:
  friend class plGALDeviceDX11;
  friend class plMemoryUtils;

  plGALQueryDX11(const plGALQueryCreationDescription& Description);
  ~plGALQueryDX11();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  ID3D11Query* m_pDXQuery = nullptr;
};

#include <RendererDX11/Resources/Implementation/QueryDX11_inl.h>
