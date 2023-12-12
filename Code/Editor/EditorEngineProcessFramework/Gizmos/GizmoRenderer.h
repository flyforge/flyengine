#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <RendererCore/Pipeline/Renderer.h>

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plGizmoRenderer : public plRenderer
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGizmoRenderer, plRenderer);

public:
  plGizmoRenderer();
  ~plGizmoRenderer();

  // plRenderer implementation
  virtual void GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& types) const override;
  virtual void GetSupportedRenderDataCategories(plHybridArray<plRenderData::Category, 8>& categories) const override;
  virtual void RenderBatch(
    const plRenderViewContext& renderContext, const plRenderPipelinePass* pPass, const plRenderDataBatch& batch) const override;

  static float s_fGizmoScale;
};
