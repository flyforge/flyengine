#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class PL_RENDERERFOUNDATION_DLL plGALShader : public plGALObject<plGALShaderCreationDescription>
{
public:
  virtual void SetDebugName(const char* szName) const = 0;

  /// Returns the list of shader resources and their binding information. These must be bound before the shader can be used.
  plArrayPtr<const plShaderResourceBinding> GetBindingMapping() const;
  /// Returns the list of vertex input attributes. Compute shaders return an empty array.
  plArrayPtr<const plShaderVertexInputAttribute> GetVertexInputAttributes() const;

protected:
  friend class plGALDevice;

  virtual plResult InitPlatform(plGALDevice* pDevice) = 0;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) = 0;

  plResult CreateBindingMapping();
  void DestroyBindingMapping();

  plGALShader(const plGALShaderCreationDescription& Description);
  virtual ~plGALShader();

protected:
  plDynamicArray<plShaderResourceBinding> m_BindingMapping;
};
