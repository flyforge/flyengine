
#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/RefCounted.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// \brief This class wraps shader byte code storage.
/// Since byte code can have different requirements for alignment, padding etc. this class manages it.
/// Also since byte code is shared between multiple shaders (e.g. same vertex shaders for different pixel shaders)
/// the instances of the byte codes are reference counted.
class PLASMA_RENDERERFOUNDATION_DLL plGALShaderByteCode : public plRefCounted
{
public:
  plGALShaderByteCode();

  plGALShaderByteCode(const plArrayPtr<const plUInt8>& byteCode);

  inline const void* GetByteCode() const;

  inline plUInt32 GetSize() const;

  inline bool IsValid() const;

protected:
  void CopyFrom(const plArrayPtr<const plUInt8>& pByteCode);

  plDynamicArray<plUInt8> m_Source;
};

#include <RendererFoundation/Shader/Implementation/ShaderByteCode_inl.h>
