#pragma once

#include <RendererFoundation/Resources/Query.h>

class plGALQueryVulkan : public plGALQuery
{
public:
  PLASMA_ALWAYS_INLINE plUInt32 GetID() const;
  PLASMA_ALWAYS_INLINE vk::QueryPool GetPool() const { return nullptr; } // TODO

protected:
  friend class plGALDeviceVulkan;
  friend class plMemoryUtils;

  plGALQueryVulkan(const plGALQueryCreationDescription& Description);
  ~plGALQueryVulkan();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  plUInt32 m_uiID;
};

#include <RendererVulkan/Resources/Implementation/QueryVulkan_inl.h>
