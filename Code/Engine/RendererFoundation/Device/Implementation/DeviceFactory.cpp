#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/DeviceFactory.h>

struct CreatorFuncInfo
{
  plGALDeviceFactory::CreatorFunc m_Func;
  plString m_sShaderModel;
  plString m_sShaderCompiler;
};

static plHashTable<plString, CreatorFuncInfo> s_CreatorFuncs;

CreatorFuncInfo* GetCreatorFuncInfo(plStringView sRendererName)
{
  auto pFuncInfo = s_CreatorFuncs.GetValue(sRendererName);
  if (pFuncInfo == nullptr)
  {
    plStringBuilder sPluginName = "plasmaRenderer";
    sPluginName.Append(sRendererName);

    PLASMA_VERIFY(plPlugin::LoadPlugin(sPluginName).Succeeded(), "Renderer plugin '{}' not found", sPluginName);

    pFuncInfo = s_CreatorFuncs.GetValue(sRendererName);
    PLASMA_ASSERT_DEV(pFuncInfo != nullptr, "Renderer '{}' is not registered", sRendererName);
  }

  return pFuncInfo;
}

plInternal::NewInstance<plGALDevice> plGALDeviceFactory::CreateDevice(plStringView sRendererName, plAllocatorBase* pAllocator, const plGALDeviceCreationDescription& desc)
{
  if (auto pFuncInfo = GetCreatorFuncInfo(sRendererName))
  {
    return pFuncInfo->m_Func(pAllocator, desc);
  }

  return plInternal::NewInstance<plGALDevice>(nullptr, pAllocator);
}

void plGALDeviceFactory::GetShaderModelAndCompiler(plStringView sRendererName, const char*& ref_szShaderModel, const char*& ref_szShaderCompiler)
{
  if (auto pFuncInfo = GetCreatorFuncInfo(sRendererName))
  {
    ref_szShaderModel = pFuncInfo->m_sShaderModel;
    ref_szShaderCompiler = pFuncInfo->m_sShaderCompiler;
  }
}

void plGALDeviceFactory::RegisterCreatorFunc(const char* szRendererName, const CreatorFunc& func, const char* szShaderModel, const char* szShaderCompiler)
{
  CreatorFuncInfo funcInfo;
  funcInfo.m_Func = func;
  funcInfo.m_sShaderModel = szShaderModel;
  funcInfo.m_sShaderCompiler = szShaderCompiler;

  PLASMA_VERIFY(s_CreatorFuncs.Insert(szRendererName, funcInfo) == false, "Creator func already registered");
}

void plGALDeviceFactory::UnregisterCreatorFunc(const char* szRendererName)
{
  PLASMA_VERIFY(s_CreatorFuncs.Remove(szRendererName), "Creator func not registered");
}


PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_DeviceFactory);
