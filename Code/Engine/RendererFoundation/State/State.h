
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>

class PLASMA_RENDERERFOUNDATION_DLL plGALBlendState : public plGALObject<plGALBlendStateCreationDescription>
{
public:
protected:
  plGALBlendState(const plGALBlendStateCreationDescription& Description);

  virtual ~plGALBlendState();

  virtual plResult InitPlatform(plGALDevice* pDevice) = 0;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) = 0;
};

class PLASMA_RENDERERFOUNDATION_DLL plGALDepthStencilState : public plGALObject<plGALDepthStencilStateCreationDescription>
{
public:
protected:
  plGALDepthStencilState(const plGALDepthStencilStateCreationDescription& Description);

  virtual ~plGALDepthStencilState();

  virtual plResult InitPlatform(plGALDevice* pDevice) = 0;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) = 0;
};

class PLASMA_RENDERERFOUNDATION_DLL plGALRasterizerState : public plGALObject<plGALRasterizerStateCreationDescription>
{
public:
protected:
  plGALRasterizerState(const plGALRasterizerStateCreationDescription& Description);

  virtual ~plGALRasterizerState();

  virtual plResult InitPlatform(plGALDevice* pDevice) = 0;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) = 0;
};

class PLASMA_RENDERERFOUNDATION_DLL plGALSamplerState : public plGALObject<plGALSamplerStateCreationDescription>
{
public:
protected:
  plGALSamplerState(const plGALSamplerStateCreationDescription& Description);

  virtual ~plGALSamplerState();

  virtual plResult InitPlatform(plGALDevice* pDevice) = 0;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) = 0;
};
