#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorManager.h>
#include <Core/ActorSystem/ActorPluginWindow.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Texture/Image/Image.h>

PlasmaEngineProcessViewContext::PlasmaEngineProcessViewContext(PlasmaEngineProcessDocumentContext* pContext)
  : m_pDocumentContext(pContext)
{
  m_uiViewID = 0xFFFFFFFF;
}

PlasmaEngineProcessViewContext::~PlasmaEngineProcessViewContext()
{
  plRenderWorld::DeleteView(m_hView);
  m_hView.Invalidate();

  plActorManager::GetSingleton()->DestroyAllActors(this);
}

void PlasmaEngineProcessViewContext::SetViewID(plUInt32 id)
{
  PLASMA_ASSERT_DEBUG(m_uiViewID == 0xFFFFFFFF, "View ID may only be set once");
  m_uiViewID = id;
}

void PlasmaEngineProcessViewContext::HandleViewMessage(const PlasmaEditorEngineViewMsg* pMsg)
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP) || PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plViewRedrawMsgToEngine>())
  {
    const plViewRedrawMsgToEngine* pMsg2 = static_cast<const plViewRedrawMsgToEngine*>(pMsg);

    SetCamera(pMsg2);

    if (pMsg2->m_uiWindowWidth > 0 && pMsg2->m_uiWindowHeight > 0)
    {
#  if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
      HandleWindowUpdate(reinterpret_cast<plWindowHandle>(pMsg2->m_uiHWND), pMsg2->m_uiWindowWidth, pMsg2->m_uiWindowHeight);
#  else
      plWindowHandle windowHandle;
      windowHandle.type = plWindowHandle::Type::XCB;
      windowHandle.xcbWindow.m_Window = static_cast<plUInt32>(pMsg2->m_uiHWND);
      windowHandle.xcbWindow.m_pConnection = nullptr;
      HandleWindowUpdate(windowHandle, pMsg2->m_uiWindowWidth, pMsg2->m_uiWindowHeight);
#  endif
      Redraw(true);
    }
  }
  else if (const plViewScreenshotMsgToEngine* msg = plDynamicCast<const plViewScreenshotMsgToEngine*>(pMsg))
  {
    plImage img;
    plActorPluginWindow* pWindow = m_pEditorWndActor->GetPlugin<plActorPluginWindow>();
    pWindow->GetOutputTarget()->CaptureImage(img).IgnoreResult();

    img.SaveTo(msg->m_sOutputFile).IgnoreResult();
  }
#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
  PLASMA_REPORT_FAILURE("This code path should never be executed on UWP.");
#else
#  error "Unsupported platform."
#endif
}

void PlasmaEngineProcessViewContext::SendViewMessage(PlasmaEditorEngineViewMsg* pViewMsg)
{
  pViewMsg->m_DocumentGuid = GetDocumentContext()->GetDocumentGuid();
  pViewMsg->m_uiViewID = m_uiViewID;

  GetDocumentContext()->SendProcessMessage(pViewMsg);
}

void PlasmaEngineProcessViewContext::HandleWindowUpdate(plWindowHandle hWnd, plUInt16 uiWidth, plUInt16 uiHeight)
{
  PLASMA_LOG_BLOCK("PlasmaEngineProcessViewContext::HandleWindowUpdate");

  if (m_pEditorWndActor != nullptr)
  {
    // Update window size
    plActorPluginWindow* pWindowPlugin = m_pEditorWndActor->GetPlugin<plActorPluginWindow>();

    const plSizeU32 wndSize = pWindowPlugin->GetWindow()->GetClientAreaSize();

    PLASMA_ASSERT_DEV(pWindowPlugin->GetWindow()->GetNativeWindowHandle() == hWnd, "Editor view handle must never change. View needs to be destroyed and recreated.");

    if (wndSize.width == uiWidth && wndSize.height == uiHeight)
      return;

    if (static_cast<PlasmaEditorProcessViewWindow*>(pWindowPlugin->GetWindow())->UpdateWindow(hWnd, uiWidth, uiHeight).Failed())
    {
      plLog::Error("Failed to update Editor Process View Window");
    }
    return;
  }

  {
    // Create new actor
    plUniquePtr<plActor> pActor = PLASMA_DEFAULT_NEW(plActor, "EditorView", this);
    m_pEditorWndActor = pActor.Borrow();

    plUniquePtr<plActorPluginWindowOwner> pWindowPlugin = PLASMA_DEFAULT_NEW(plActorPluginWindowOwner);

    // create window
    {
      plUniquePtr<PlasmaEditorProcessViewWindow> pWindow = PLASMA_DEFAULT_NEW(PlasmaEditorProcessViewWindow);
      if (pWindow->UpdateWindow(hWnd, uiWidth, uiHeight).Succeeded())
      {
        pWindowPlugin->m_pWindow = std::move(pWindow);
      }
      else
      {
        plLog::Error("Failed to create Editor Process View Window");
      }
    }

    // create output target
    {
      plUniquePtr<plWindowOutputTargetGAL> pOutput = PLASMA_DEFAULT_NEW(plWindowOutputTargetGAL, [this](plGALSwapChainHandle hSwapChain, plSizeU32 size) {
        OnSwapChainChanged(hSwapChain, size);
      });

      plGALWindowSwapChainCreationDescription desc;
      desc.m_pWindow = pWindowPlugin->m_pWindow.Borrow();
      desc.m_BackBufferFormat = plGALResourceFormat::RGBAUByteNormalizedsRGB;
      desc.m_bAllowScreenshots = true;

      pOutput->CreateSwapchain(desc);

      pWindowPlugin->m_pWindowOutputTarget = std::move(pOutput);
    }

    // setup render target
    {
      plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
      plWindowOutputTargetGAL* pOutput = static_cast<plWindowOutputTargetGAL*>(pWindowPlugin->m_pWindowOutputTarget.Borrow());

      const plSizeU32 wndSize = pWindowPlugin->m_pWindow->GetClientAreaSize();
      SetupRenderTarget(pOutput->m_hSwapChain, nullptr, static_cast<plUInt16>(wndSize.width), static_cast<plUInt16>(wndSize.height));
    }

    pActor->AddPlugin(std::move(pWindowPlugin));
    plActorManager::GetSingleton()->AddActor(std::move(pActor));
  }
}

void PlasmaEngineProcessViewContext::OnSwapChainChanged(plGALSwapChainHandle hSwapChain, plSizeU32 size)
{
  plView* pView = nullptr;
  if (plRenderWorld::TryGetView(m_hView, pView))
  {
    pView->SetViewport(plRectFloat(0.0f, 0.0f, (float)size.width, (float)size.height));
    pView->ForceUpdate();
  }
}

void PlasmaEngineProcessViewContext::SetupRenderTarget(plGALSwapChainHandle hSwapChain, const plGALRenderTargets* renderTargets, plUInt16 uiWidth, plUInt16 uiHeight)
{
  PLASMA_LOG_BLOCK("PlasmaEngineProcessViewContext::SetupRenderTarget");
  PLASMA_ASSERT_DEV(hSwapChain.IsInvalidated() || renderTargets == nullptr, "hSwapChain and renderTargetSetup are mutually exclusive.");

  // setup view
  {
    if (m_hView.IsInvalidated())
    {
      m_hView = CreateView();
    }

    plView* pView = nullptr;
    if (plRenderWorld::TryGetView(m_hView, pView))
    {
      if (!hSwapChain.IsInvalidated())
        pView->SetSwapChain(hSwapChain);
      else
        pView->SetRenderTargets(*renderTargets);
      pView->SetViewport(plRectFloat(0.0f, 0.0f, (float)uiWidth, (float)uiHeight));
    }
  }
}

void PlasmaEngineProcessViewContext::Redraw(bool bRenderEditorGizmos)
{
  auto pState = plGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameStateLinkedToWorld(GetDocumentContext()->GetWorld());

  if (pState != nullptr)
  {
    pState->ScheduleRendering();
  }
  // setting to only update one view ?
  // else

  plView* pView = nullptr;
  if (plRenderWorld::TryGetView(m_hView, pView))
  {
    const plTag& tagEditor = plTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

    if (!bRenderEditorGizmos)
    {
      // exclude all editor objects from rendering in proper game views
      pView->m_ExcludeTags.Set(tagEditor);
    }
    else
    {
      pView->m_ExcludeTags.Remove(tagEditor);
    }

    plRenderWorld::AddMainView(m_hView);
  }
}

bool PlasmaEngineProcessViewContext::FocusCameraOnObject(plCamera& camera, const plBoundingBoxSphere& objectBounds, float fFov, const plVec3& vViewDir)
{
  if (!objectBounds.IsValid())
    return false;

  plVec3 vDir = vViewDir;
  bool bChanged = false;
  plVec3 vCameraPos = camera.GetCenterPosition();
  plVec3 vCenterPos = objectBounds.GetSphere().m_vCenter;

  const float fDist = plMath::Max(0.1f, objectBounds.GetSphere().m_fRadius) / plMath::Sin(plAngle::Degree(fFov / 2));
  vDir.Normalize();
  plVec3 vNewCameraPos = vCenterPos - vDir * fDist;
  if (!vNewCameraPos.IsEqual(vCameraPos, 0.01f))
  {
    vCameraPos = vNewCameraPos;
    bChanged = true;
  }

  if (bChanged)
  {
    if (!vNewCameraPos.IsValid())
      return false;

    camera.SetCameraMode(plCameraMode::PerspectiveFixedFovX, fFov, 0.1f, 1000.0f);
    camera.LookAt(vNewCameraPos, vCenterPos, plVec3(0.0f, 0.0f, 1.0f));
  }

  return bChanged;
}

void PlasmaEngineProcessViewContext::SetCamera(const plViewRedrawMsgToEngine* pMsg)
{
  plViewRenderMode::Enum renderMode = (plViewRenderMode::Enum)pMsg->m_uiRenderMode;

  plView* pView = nullptr;
  if (plRenderWorld::TryGetView(m_hView, pView) && pView->GetWorld() != nullptr)
  {
    if (renderMode == plViewRenderMode::None)
    {
      pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());
    }
    else
    {
      pView->SetRenderPipelineResource(CreateDebugRenderPipeline());
    }
  }

  if (m_Camera.GetCameraMode() != plCameraMode::Stereo)
  {
    bool bCameraIsActive = false;
    if (pView && pView->GetWorld())
    {
      plEnum<plCameraUsageHint> usageHint = pView->GetCameraUsageHint();
      plCameraComponent* pComp = pView->GetWorld()->GetOrCreateComponentManager<plCameraComponentManager>()->GetCameraByUsageHint(usageHint);
      bCameraIsActive = pComp != nullptr && pComp->IsActive();
    }

    // Camera mode should be controlled by a matching camera component if one exists.
    if (!bCameraIsActive)
    {
      plCameraMode::Enum cameraMode = (plCameraMode::Enum)pMsg->m_iCameraMode;
      m_Camera.SetCameraMode(cameraMode, pMsg->m_fFovOrDim, pMsg->m_fNearPlane, pMsg->m_fFarPlane);
    }

    // prevent too large values
    // sometimes this can happen when imported data is badly scaled and thus way too large
    // then adding dirForwards result in no change and we run into other asserts later
    plVec3 pos = pMsg->m_vPosition;
    pos.x = plMath::Clamp(pos.x, -1000000.0f, +1000000.0f);
    pos.y = plMath::Clamp(pos.y, -1000000.0f, +1000000.0f);
    pos.z = plMath::Clamp(pos.z, -1000000.0f, +1000000.0f);

    m_Camera.LookAt(pos, pos + pMsg->m_vDirForwards, pMsg->m_vDirUp);
  }

  if (pView)
  {
    pView->SetViewRenderMode(renderMode);

    bool bUseDepthPrePass = renderMode != plViewRenderMode::WireframeColor && renderMode != plViewRenderMode::WireframeMonochrome;
    pView->SetRenderPassProperty("DepthPrePass", "Active", bUseDepthPrePass);
    pView->SetRenderPassProperty("AOPass", "Active", bUseDepthPrePass); // Also disable SSAO to save some performance

    // by default this stuff is disabled, derived classes can enable it
    pView->SetRenderPassProperty("EditorSelectionPass", "Active", false);
    pView->SetExtractorProperty("EditorShapeIconsExtractor", "Active", false);
  }
}

plRenderPipelineResourceHandle PlasmaEngineProcessViewContext::CreateDefaultRenderPipeline()
{
  return PlasmaEditorEngineProcessApp::GetSingleton()->CreateDefaultMainRenderPipeline();
}

plRenderPipelineResourceHandle PlasmaEngineProcessViewContext::CreateDebugRenderPipeline()
{
  return PlasmaEditorEngineProcessApp::GetSingleton()->CreateDefaultDebugRenderPipeline();
}

void PlasmaEngineProcessViewContext::DrawSimpleGrid() const
{
  plDynamicArray<plDebugRenderer::Line> lines;
  lines.Reserve(2 * (10 + 1 + 10) + 4);

  const plColor xAxisColor = plColorScheme::LightUI(plColorScheme::Red) * 0.7f;
  const plColor yAxisColor = plColorScheme::LightUI(plColorScheme::Green) * 0.7f;
  const plColor gridColor = plColorScheme::LightUI(plColorScheme::Gray) * 0.5f;

  // arrows

  const float f = 1.0f;

  {
    auto& l = lines.ExpandAndGetRef();
    l.m_start.Set(f, 0.0f, 0.0f);
    l.m_end.Set(f - 0.25f, 0.25f, 0.0f);
    l.m_startColor = xAxisColor;
    l.m_endColor = xAxisColor;
  }

  {
    auto& l = lines.ExpandAndGetRef();
    l.m_start.Set(f, 0.0f, 0.0f);
    l.m_end.Set(f - 0.25f, -0.25f, 0.0f);
    l.m_startColor = xAxisColor;
    l.m_endColor = xAxisColor;
  }

  {
    auto& l = lines.ExpandAndGetRef();
    l.m_start.Set(0.0f, f, 0.0f);
    l.m_end.Set(0.25f, f - 0.25f, 0.0f);
    l.m_startColor = yAxisColor;
    l.m_endColor = yAxisColor;
  }

  {
    auto& l = lines.ExpandAndGetRef();
    l.m_start.Set(0.0f, f, 0.0f);
    l.m_end.Set(-0.25f, f - 0.25f, 0.0f);
    l.m_startColor = yAxisColor;
    l.m_endColor = yAxisColor;
  }

  {
    const float x = 10.0f;

    for (plInt32 y = -10; y <= +10; ++y)
    {
      auto& line = lines.ExpandAndGetRef();

      line.m_start.Set((float)-x, (float)y, 0.0f);
      line.m_end.Set((float)+x, (float)y, 0.0f);

      if (y == 0)
      {
        line.m_startColor = xAxisColor;
      }
      else
      {
        line.m_startColor = gridColor;
      }

      line.m_endColor = line.m_startColor;
    }
  }

  {
    const float y = 10.0f;

    for (plInt32 x = -10; x <= +10; ++x)
    {
      auto& line = lines.ExpandAndGetRef();

      line.m_start.Set((float)x, (float)-y, 0.0f);
      line.m_end.Set((float)x, (float)+y, 0.0f);

      if (x == 0)
      {
        line.m_startColor = yAxisColor;
      }
      else
      {
        line.m_startColor = gridColor;
      }

      line.m_endColor = line.m_startColor;
    }
  }

  plDebugRenderer::DrawLines(m_hView, lines, plColor::White);
}

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <EditorEngineProcessFramework/EngineProcess/Implementation/Win/EngineProcessViewContext_win.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
#  include <EditorEngineProcessFramework/EngineProcess/Implementation/Linux/EngineProcessViewContext_linux.h>
#else
#  error Platform not supported
#endif
