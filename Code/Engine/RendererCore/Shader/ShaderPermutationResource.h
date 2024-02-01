#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Time/Timestamp.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>

using plShaderPermutationResourceHandle = plTypedResourceHandle<class plShaderPermutationResource>;
using plShaderStateResourceHandle = plTypedResourceHandle<class plShaderStateResource>;

struct plShaderPermutationResourceDescriptor
{
};

class PL_RENDERERCORE_DLL plShaderPermutationResource : public plResource
{
  PL_ADD_DYNAMIC_REFLECTION(plShaderPermutationResource, plResource);
  PL_RESOURCE_DECLARE_COMMON_CODE(plShaderPermutationResource);
  PL_RESOURCE_DECLARE_CREATEABLE(plShaderPermutationResource, plShaderPermutationResourceDescriptor);

public:
  plShaderPermutationResource();

  plGALShaderHandle GetGALShader() const { return m_hShader; }
  const plGALShaderByteCode* GetShaderByteCode(plGALShaderStage::Enum stage) const { return m_ByteCodes[stage]; }

  plGALBlendStateHandle GetBlendState() const { return m_hBlendState; }
  plGALDepthStencilStateHandle GetDepthStencilState() const { return m_hDepthStencilState; }
  plGALRasterizerStateHandle GetRasterizerState() const { return m_hRasterizerState; }

  bool IsShaderValid() const { return m_bShaderPermutationValid; }

  plArrayPtr<const plPermutationVar> GetPermutationVars() const { return m_PermutationVars; }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;
  virtual plResourceTypeLoader* GetDefaultResourceTypeLoader() const override;

private:
  friend class plShaderManager;

  plSharedPtr<const plGALShaderByteCode> m_ByteCodes[plGALShaderStage::ENUM_COUNT];

  bool m_bShaderPermutationValid;
  plGALShaderHandle m_hShader;

  plGALBlendStateHandle m_hBlendState;
  plGALDepthStencilStateHandle m_hDepthStencilState;
  plGALRasterizerStateHandle m_hRasterizerState;

  plHybridArray<plPermutationVar, 16> m_PermutationVars;
};


class plShaderPermutationResourceLoader : public plResourceTypeLoader
{
public:
  virtual plResourceLoadData OpenDataStream(const plResource* pResource) override;
  virtual void CloseDataStream(const plResource* pResource, const plResourceLoadData& loaderData) override;

  virtual bool IsResourceOutdated(const plResource* pResource) const override;

private:
  plResult RunCompiler(const plResource* pResource, plShaderPermutationBinary& BinaryInfo, bool bForce);
};
