
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class PL_RENDERERFOUNDATION_DLL plGALVertexDeclaration : public plGALObject<plGALVertexDeclarationCreationDescription>
{
public:
protected:
  friend class plGALDevice;

  virtual plResult InitPlatform(plGALDevice* pDevice) = 0;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) = 0;

  plGALVertexDeclaration(const plGALVertexDeclarationCreationDescription& Description);

  virtual ~plGALVertexDeclaration();
};
