#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorManager.h>
#include <Core/ActorSystem/ActorPluginWindow.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>

PLASMA_IMPLEMENT_SINGLETON(PlasmaEditorEngineProcessApp);

PlasmaEditorEngineProcessApp::PlasmaEditorEngineProcessApp()
  : m_SingletonRegistrar(this)
{
}

PlasmaEditorEngineProcessApp::~PlasmaEditorEngineProcessApp()
{
  DestroyRemoteWindow();
}

void PlasmaEditorEngineProcessApp::SetRemoteMode()
{
  m_Mode = PlasmaEditorEngineProcessMode::Remote;

  CreateRemoteWindow();
}

void PlasmaEditorEngineProcessApp::CreateRemoteWindow()
{
  PLASMA_ASSERT_DEV(IsRemoteMode(), "Incorrect app mode");

  if (m_pActor != nullptr)
    return;

  plUniquePtr<plActor> pActor = PLASMA_DEFAULT_NEW(plActor, "Engine View", this);
  m_pActor = pActor.Borrow();

  // create window
  {
    plUniquePtr<plRemoteProcessWindow> pWindow = PLASMA_DEFAULT_NEW(plRemoteProcessWindow);

    plWindowCreationDesc desc;
    desc.m_uiWindowNumber = 0;
    desc.m_bClipMouseCursor = true;
    desc.m_bShowMouseCursor = true;
    desc.m_RenderResolution = plSizeU32(1024, 768);
    desc.m_Resolution = plSizeU32(1024, 768);
    desc.m_WindowMode = plWindowMode::WindowFixedResolution;
    desc.m_Title = "Engine View";

    pWindow->Initialize(desc).IgnoreResult();

    plUniquePtr<plActorPluginWindowOwner> pWindowPlugin = PLASMA_DEFAULT_NEW(plActorPluginWindowOwner);
    pWindowPlugin->m_pWindow = std::move(pWindow);

    m_pActor->AddPlugin(std::move(pWindowPlugin));
  }

  plActorManager::GetSingleton()->AddActor(std::move(pActor));
}

void PlasmaEditorEngineProcessApp::DestroyRemoteWindow()
{
  if (!m_hRemoteView.IsInvalidated())
  {
    plRenderWorld::DeleteView(m_hRemoteView);
    m_hRemoteView.Invalidate();
  }

  if (plActorManager::GetSingleton())
  {
    plActorManager::GetSingleton()->DestroyAllActors(this);
  }

  m_pActor = nullptr;
}

plRenderPipelineResourceHandle PlasmaEditorEngineProcessApp::CreateDefaultMainRenderPipeline()
{
  // EditorRenderPipeline.plRenderPipelineAsset
  return plResourceManager::LoadResource<plRenderPipelineResource>("{ da463c4d-c984-4910-b0b7-a0b3891d0448 }");
}

plRenderPipelineResourceHandle PlasmaEditorEngineProcessApp::CreateDefaultDebugRenderPipeline()
{
  // DebugRenderPipeline.plRenderPipelineAsset
  return plResourceManager::LoadResource<plRenderPipelineResource>("{ 0416eb3e-69c0-4640-be5b-77354e0e37d7 }");
}

plViewHandle PlasmaEditorEngineProcessApp::CreateRemoteWindowAndView(plCamera* pCamera)
{
  PLASMA_ASSERT_DEV(IsRemoteMode(), "Incorrect app mode");

  CreateRemoteWindow();

  if (m_hRemoteView.IsInvalidated())
  {
    plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

    plActorPluginWindowOwner* pWindowPlugin = m_pActor->GetPlugin<plActorPluginWindowOwner>();

    // create output target
    {
      plUniquePtr<plWindowOutputTargetGAL> pOutput = PLASMA_DEFAULT_NEW(plWindowOutputTargetGAL);

      plGALWindowSwapChainCreationDescription desc;
      desc.m_pWindow = pWindowPlugin->m_pWindow.Borrow();
      desc.m_BackBufferFormat = plGALResourceFormat::RGBAUByteNormalizedsRGB;
      desc.m_bAllowScreenshots = true;

      pOutput->CreateSwapchain(desc);

      pWindowPlugin->m_pWindowOutputTarget = std::move(pOutput);
    }

    // get swapchain
    plGALSwapChainHandle hSwapChain;
    {
      plWindowOutputTargetGAL* pOutputTarget = static_cast<plWindowOutputTargetGAL*>(pWindowPlugin->m_pWindowOutputTarget.Borrow());
      hSwapChain = pOutputTarget->m_hSwapChain;
    }

    // setup view
    {
      plView* pView = nullptr;
      m_hRemoteView = plRenderWorld::CreateView("Remote Process", pView);

      // EditorRenderPipeline.plRenderPipelineAsset
      pView->SetRenderPipelineResource(plResourceManager::LoadResource<plRenderPipelineResource>("{ da463c4d-c984-4910-b0b7-a0b3891d0448 }"));

      const plSizeU32 wndSize = pWindowPlugin->m_pWindow->GetClientAreaSize();

      pView->SetSwapChain(hSwapChain);
      pView->SetViewport(plRectFloat(0.0f, 0.0f, (float)wndSize.width, (float)wndSize.height));
      pView->SetCamera(pCamera);
    }
  }

  return m_hRemoteView;
}
