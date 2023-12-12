
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class PLASMA_RENDERERFOUNDATION_DLL plGALUnorderedAccessView : public plGALObject<plGALUnorderedAccessViewCreationDescription>
{
public:
  PLASMA_ALWAYS_INLINE plGALResourceBase* GetResource() const { return m_pResource; }

  PLASMA_ALWAYS_INLINE bool ShouldUnsetResourceView() const { return m_bUnsetResourceView; }

protected:
  friend class plGALDevice;

  plGALUnorderedAccessView(plGALResourceBase* pResource, const plGALUnorderedAccessViewCreationDescription& description);

  ~plGALUnorderedAccessView() override;

  virtual plResult InitPlatform(plGALDevice* pDevice) = 0;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) = 0;

  plGALResourceBase* m_pResource;
  bool m_bUnsetResourceView;
};
