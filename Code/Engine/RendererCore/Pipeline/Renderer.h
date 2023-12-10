#pragma once

#include <RendererCore/Pipeline/RenderData.h>

/// \brief This is the base class for types that handle rendering of different object types.
///
/// E.g. there are different renderers for meshes, particle effects, light sources, etc.
class PLASMA_RENDERERCORE_DLL plRenderer : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRenderer, plReflectedClass);

public:
  virtual void GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const = 0;
  virtual void GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& ref_categories) const = 0;

  virtual void RenderBatch(const plRenderViewContext& renderViewContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const = 0;
};
