#pragma once

#include <RendererCore/Pipeline/Declarations.h>

class PL_RENDERERCORE_DLL plFrameDataProviderBase : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plFrameDataProviderBase, plReflectedClass);

protected:
  plFrameDataProviderBase();

  virtual void* UpdateData(const plRenderViewContext& renderViewContext, const plExtractedRenderData& extractedData) = 0;

  void* GetData(const plRenderViewContext& renderViewContext);

private:
  friend class plRenderPipeline;

  const plRenderPipeline* m_pOwnerPipeline = nullptr;
  void* m_pData = nullptr;
  plUInt64 m_uiLastUpdateFrame = 0;
};

template <typename T>
class plFrameDataProvider : public plFrameDataProviderBase
{
public:
  T* GetData(const plRenderViewContext& renderViewContext) { return static_cast<T*>(plFrameDataProviderBase::GetData(renderViewContext)); }
};
