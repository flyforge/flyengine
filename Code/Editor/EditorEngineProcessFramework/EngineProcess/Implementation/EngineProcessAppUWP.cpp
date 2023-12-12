#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessAppUWP.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

PlasmaEditorEngineProcessAppUWP::PlasmaEditorEngineProcessAppUWP() {}

PlasmaEditorEngineProcessAppUWP::~PlasmaEditorEngineProcessAppUWP() {}

plViewHandle PlasmaEditorEngineProcessAppUWP::CreateRemoteWindowAndView(plCamera* pCamera)
{
  PLASMA_ASSERT_DEV(IsRemoteMode(), "Incorrect app mode");

  CreateRemoteWindow();

  return PlasmaEditorEngineProcessApp::CreateRemoteWindowAndView(pCamera);
}

plRenderPipelineResourceHandle PlasmaEditorEngineProcessAppUWP::CreateDefaultMainRenderPipeline()
{
  return PlasmaEditorEngineProcessApp::CreateDefaultMainRenderPipeline();
}

plRenderPipelineResourceHandle PlasmaEditorEngineProcessAppUWP::CreateDefaultDebugRenderPipeline()
{
  return CreateDefaultMainRenderPipeline();
}
