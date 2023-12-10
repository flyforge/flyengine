#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessAppUWP.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

plEditorEngineProcessAppUWP::plEditorEngineProcessAppUWP() = default;

plEditorEngineProcessAppUWP::~plEditorEngineProcessAppUWP() = default;

plViewHandle plEditorEngineProcessAppUWP::CreateRemoteWindowAndView(plCamera* pCamera)
{
  PLASMA_ASSERT_DEV(IsRemoteMode(), "Incorrect app mode");

  CreateRemoteWindow();

  return plEditorEngineProcessApp::CreateRemoteWindowAndView(pCamera);
}

plRenderPipelineResourceHandle plEditorEngineProcessAppUWP::CreateDefaultMainRenderPipeline()
{
  return plEditorEngineProcessApp::CreateDefaultMainRenderPipeline();
}

plRenderPipelineResourceHandle plEditorEngineProcessAppUWP::CreateDefaultDebugRenderPipeline()
{
  return CreateDefaultMainRenderPipeline();
}
