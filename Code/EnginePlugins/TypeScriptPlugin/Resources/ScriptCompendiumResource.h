#pragma once

#include <TypeScriptPlugin/TypeScriptPluginDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>

class plStreamWriter;
class plStreamReader;

struct PL_TYPESCRIPTPLUGIN_DLL plScriptCompendiumResourceDesc
{
  plMap<plString, plString> m_PathToSource;

  struct ComponentTypeInfo
  {
    plString m_sComponentTypeName;
    plString m_sComponentFilePath;

    plResult Serialize(plStreamWriter& inout_stream) const;
    plResult Deserialize(plStreamReader& inout_stream);
  };

  plMap<plUuid, ComponentTypeInfo> m_AssetGuidToInfo;

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);
};

class PL_TYPESCRIPTPLUGIN_DLL plScriptCompendiumResource : public plResource
{
  PL_ADD_DYNAMIC_REFLECTION(plScriptCompendiumResource, plResource);
  PL_RESOURCE_DECLARE_COMMON_CODE(plScriptCompendiumResource);

public:
  plScriptCompendiumResource();
  ~plScriptCompendiumResource();

  const plScriptCompendiumResourceDesc& GetDescriptor() const { return m_Desc; }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* pStream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  plScriptCompendiumResourceDesc m_Desc;
};

using plScriptCompendiumResourceHandle = plTypedResourceHandle<class plScriptCompendiumResource>;
