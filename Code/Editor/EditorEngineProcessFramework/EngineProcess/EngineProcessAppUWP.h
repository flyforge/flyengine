#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plEditorEngineProcessAppUWP : public plEditorEngineProcessApp
{
public:
  plEditorEngineProcessAppUWP();
  ~plEditorEngineProcessAppUWP();

  virtual plViewHandle CreateRemoteWindowAndView(plCamera* pCamera) override;

  virtual plRenderPipelineResourceHandle CreateDefaultMainRenderPipeline() override;
  virtual plRenderPipelineResourceHandle CreateDefaultDebugRenderPipeline() override;
};
