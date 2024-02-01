#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/IO/DependencyFile.h>
#include <RmlUiPlugin/RmlUiPluginDLL.h>

struct PL_RMLUIPLUGIN_DLL plRmlUiScaleMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    Fixed,
    WithScreenSize,

    Default = Fixed
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_RMLUIPLUGIN_DLL, plRmlUiScaleMode);

struct PL_RMLUIPLUGIN_DLL plRmlUiResourceDescriptor
{
  plResult Save(plStreamWriter& inout_stream);
  plResult Load(plStreamReader& inout_stream);

  plDependencyFile m_DependencyFile;

  plString m_sRmlFile;
  plEnum<plRmlUiScaleMode> m_ScaleMode;
  plVec2U32 m_ReferenceResolution;
};

using plRmlUiResourceHandle = plTypedResourceHandle<class plRmlUiResource>;

class PL_RMLUIPLUGIN_DLL plRmlUiResource : public plResource
{
  PL_ADD_DYNAMIC_REFLECTION(plRmlUiResource, plResource);
  PL_RESOURCE_DECLARE_COMMON_CODE(plRmlUiResource);
  PL_RESOURCE_DECLARE_CREATEABLE(plRmlUiResource, plRmlUiResourceDescriptor);

public:
  plRmlUiResource();

  const plString& GetRmlFile() const { return m_sRmlFile; }
  const plEnum<plRmlUiScaleMode>& GetScaleMode() const { return m_ScaleMode; }
  const plVec2U32& GetReferenceResolution() const { return m_vReferenceResolution; }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  plString m_sRmlFile;
  plEnum<plRmlUiScaleMode> m_ScaleMode;
  plVec2U32 m_vReferenceResolution = plVec2U32::MakeZero();
};

class plRmlUiResourceLoader : public plResourceLoaderFromFile
{
public:
  virtual bool IsResourceOutdated(const plResource* pResource) const override;
};
