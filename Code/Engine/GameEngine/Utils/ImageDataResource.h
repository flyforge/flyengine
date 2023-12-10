#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/World/Declarations.h>
#include <Texture/Image/Image.h>

struct plImageDataResourceDescriptor
{
  plImage m_Image;

  // plResult Serialize(plStreamWriter& stream) const;
  // plResult Deserialize(plStreamReader& stream);
};

class PLASMA_GAMEENGINE_DLL plImageDataResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plImageDataResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plImageDataResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plImageDataResource, plImageDataResourceDescriptor);

public:
  plImageDataResource();
  ~plImageDataResource();

  const plImageDataResourceDescriptor& GetDescriptor() const { return *m_pDescriptor; }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  plUniquePtr<plImageDataResourceDescriptor> m_pDescriptor;
};

using plImageDataResourceHandle = plTypedResourceHandle<plImageDataResource>;
