#pragma once

#include <Foundation/Types/Delegate.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct PLASMA_RENDERERFOUNDATION_DLL plGALDeviceFactory
{
  using CreatorFunc = plDelegate<plInternal::NewInstance<plGALDevice>(plAllocatorBase*, const plGALDeviceCreationDescription&)>;

  static plInternal::NewInstance<plGALDevice> CreateDevice(plStringView sRendererName, plAllocatorBase* pAllocator, const plGALDeviceCreationDescription& desc);

  static void GetShaderModelAndCompiler(plStringView sRendererName, const char*& ref_szShaderModel, const char*& ref_szShaderCompiler);

  static void RegisterCreatorFunc(const char* szRendererName, const CreatorFunc& func, const char* szShaderModel, const char* szShaderCompiler);
  static void UnregisterCreatorFunc(const char* szRendererName);
};
