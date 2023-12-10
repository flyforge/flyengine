#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/RendererCoreDLL.h>

using plShaderResourceHandle = plTypedResourceHandle<class plShaderResource>;

struct plShaderResourceDescriptor
{
};

class PLASMA_RENDERERCORE_DLL plShaderResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plShaderResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plShaderResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plShaderResource, plShaderResourceDescriptor);

public:
  plShaderResource();

  bool IsShaderValid() const { return m_bShaderResourceIsValid; }

  plArrayPtr<const plHashedString> GetUsedPermutationVars() const { return m_PermutationVarsUsed; }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  plHybridArray<plHashedString, 16> m_PermutationVarsUsed;
  bool m_bShaderResourceIsValid;
};
