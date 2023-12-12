#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class PLASMA_RENDERERFOUNDATION_DLL plGALShader : public plGALObject<plGALShaderCreationDescription>
{
public:
  virtual void SetDebugName(const char* szName) const = 0;

protected:
  friend class plGALDevice;

  virtual plResult InitPlatform(plGALDevice* pDevice) = 0;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) = 0;

  plGALShader(const plGALShaderCreationDescription& Description);

  virtual ~plGALShader();
};
