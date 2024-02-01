#pragma once

#include <Foundation/Types/Delegate.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct PL_RENDERERFOUNDATION_DLL plGALDeviceFactory
{
  using CreatorFunc = plDelegate<plInternal::NewInstance<plGALDevice>(plAllocator*, const plGALDeviceCreationDescription&)>;

  static plInternal::NewInstance<plGALDevice> CreateDevice(plStringView sRendererName, plAllocator* pAllocator, const plGALDeviceCreationDescription& desc);

  static void GetShaderModelAndCompiler(plStringView sRendererName, const char*& ref_szShaderModel, const char*& ref_szShaderCompiler);

  static void RegisterCreatorFunc(const char* szRendererName, const CreatorFunc& func, const char* szShaderModel, const char* szShaderCompiler);
  static void UnregisterCreatorFunc(const char* szRendererName);
};
