#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL PlasmaEditorEngineProcessAppUWP : public PlasmaEditorEngineProcessApp
{
public:
  PlasmaEditorEngineProcessAppUWP();
  ~PlasmaEditorEngineProcessAppUWP();

  virtual plViewHandle CreateRemoteWindowAndView(plCamera* pCamera) override;

  virtual plRenderPipelineResourceHandle CreateDefaultMainRenderPipeline() override;
  virtual plRenderPipelineResourceHandle CreateDefaultDebugRenderPipeline() override;
};
