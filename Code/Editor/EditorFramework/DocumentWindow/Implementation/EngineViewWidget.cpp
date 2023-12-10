#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>

plUInt32 plQtEngineViewWidget::s_uiNextViewID = 0;

plQtEngineViewWidget::InteractionContext plQtEngineViewWidget::s_InteractionContext;

void plObjectPickingResult::Reset()
{
  m_PickedComponent = plUuid();
  m_PickedObject = plUuid();
  m_PickedOther = plUuid();
  m_uiPartIndex = 0;
  m_vPickedPosition.SetZero();
  m_vPickedNormal.SetZero();
  m_vPickingRayStart.SetZero();
}

////////////////////////////////////////////////////////////////////////
// plQtEngineViewWidget public functions
////////////////////////////////////////////////////////////////////////

plSizeU32 plQtEngineViewWidget::s_FixedResolution(0, 0);

plQtEngineViewWidget::plQtEngineViewWidget(QWidget* pParent, plQtEngineDocumentWindow* pDocumentWindow, plEngineViewConfig* pViewConfig)
  : QWidget(pParent)
  , m_pDocumentWindow(pDocumentWindow)
  , m_pViewConfig(pViewConfig)
{
  m_pRestartButtonLayout = nullptr;
  m_pRestartButton = nullptr;

  setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  // setAttribute(Qt::WA_OpaquePaintEvent);
  setAutoFillBackground(false);
  setMouseTracking(true);
  setMinimumSize(64, 64); // prevent the window from becoming zero sized, otherwise the rendering code may crash

  setAttribute(Qt::WA_PaintOnScreen, true);
  setAttribute(Qt::WA_NativeWindow, true);
  setAttribute(Qt::WA_NoSystemBackground);

  installEventFilter(this);

  m_bUpdatePickingData = false;
  m_bInDragAndDropOperation = false;

  m_uiViewID = s_uiNextViewID;
  ++s_uiNextViewID;

  m_fCameraLerp = 1.0f;
  m_fCameraTargetFovOrDim = 70.0f;

  plEditorEngineProcessConnection::s_Events.AddEventHandler(plMakeDelegate(&plQtEngineViewWidget::EngineViewProcessEventHandler, this));

  if (plEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    ShowRestartButton(true);
}


plQtEngineViewWidget::~plQtEngineViewWidget()
{
  plEditorEngineProcessConnection::s_Events.RemoveEventHandler(plMakeDelegate(&plQtEngineViewWidget::EngineViewProcessEventHandler, this));

  {
    // Ensure the engine process swap chain is destroyed before the window.
    plViewDestroyedMsgToEngine msg;
    msg.m_uiViewID = GetViewID();
    // If we fail to send the message the engine process is down and we don't need to clean up.
    if (m_pDocumentWindow->GetDocument()->SendMessageToEngine(&msg))
    {
      // Wait for engine process response
      auto callback = [&](plProcessMessage* pMsg) -> bool {
        auto pResponse = static_cast<plViewDestroyedResponseMsgToEditor*>(pMsg);
        return pResponse->m_DocumentGuid == m_pDocumentWindow->GetDocument()->GetGuid() && pResponse->m_uiViewID == msg.m_uiViewID;
      };
      plProcessCommunicationChannel::WaitForMessageCallback cb = callback;

      if (plEditorEngineProcessConnection::GetSingleton()->WaitForMessage(plGetStaticRTTI<plViewDestroyedResponseMsgToEditor>(), plTime::MakeFromSeconds(5), &cb).Failed())
      {
        plLog::Error("Timeout while waiting for engine process to destroy view.");
      }
    }
  }

  m_pDocumentWindow->RemoveViewWidget(this);
}

void plQtEngineViewWidget::SyncToEngine()
{
  plViewRedrawMsgToEngine cam;
  cam.m_uiRenderMode = m_pViewConfig->m_RenderMode;

  float fov = m_pViewConfig->m_Camera.GetFovOrDim();
  if (m_pViewConfig->m_Camera.IsPerspective())
  {
    plEditorPreferencesUser* pPref = plPreferences::QueryPreferences<plEditorPreferencesUser>();
    fov = pPref->m_fPerspectiveFieldOfView;
  }

  cam.m_uiViewID = GetViewID();
  cam.m_fNearPlane = m_pViewConfig->m_Camera.GetNearPlane();
  cam.m_fFarPlane = m_pViewConfig->m_Camera.GetFarPlane();
  cam.m_iCameraMode = (plInt8)m_pViewConfig->m_Camera.GetCameraMode();
  cam.m_bUseCameraTransformOnDevice = m_pViewConfig->m_bUseCameraTransformOnDevice;
  cam.m_fFovOrDim = fov;
  cam.m_vDirForwards = m_pViewConfig->m_Camera.GetCenterDirForwards();
  cam.m_vDirUp = m_pViewConfig->m_Camera.GetCenterDirUp();
  cam.m_vDirRight = m_pViewConfig->m_Camera.GetCenterDirRight();
  cam.m_vPosition = m_pViewConfig->m_Camera.GetCenterPosition();
  cam.m_ViewMatrix = m_pViewConfig->m_Camera.GetViewMatrix();
  m_pViewConfig->m_Camera.GetProjectionMatrix((float)width() / (float)height(), cam.m_ProjMatrix);

  cam.m_uiHWND = (plUInt64)(winId());
  cam.m_uiWindowWidth = width() * this->devicePixelRatio();
  cam.m_uiWindowHeight = height() * this->devicePixelRatio();
  cam.m_bUpdatePickingData = m_bUpdatePickingData;
  cam.m_bEnablePickingSelected = IsPickingAgainstSelectionAllowed() && (!plEditorInputContext::IsAnyInputContextActive() || plEditorInputContext::GetActiveInputContext()->IsPickingSelectedAllowed());
  cam.m_bEnablePickTransparent = m_bPickTransparent;

  if (s_FixedResolution.HasNonZeroArea())
  {
    cam.m_uiWindowWidth = s_FixedResolution.width;
    cam.m_uiWindowHeight = s_FixedResolution.height;
  }

  m_pDocumentWindow->GetEditorEngineConnection()->SendMessage(&cam);
}


void plQtEngineViewWidget::GetCameraMatrices(plMat4& out_mViewMatrix, plMat4& out_mProjectionMatrix) const
{
  out_mViewMatrix = m_pViewConfig->m_Camera.GetViewMatrix();
  m_pViewConfig->m_Camera.GetProjectionMatrix((float)width() / (float)height(), out_mProjectionMatrix);
}

void plQtEngineViewWidget::UpdateCameraInterpolation()
{
  if (m_fCameraLerp >= 1.0f)
    return;

  const plTime tNow = plTime::Now();
  const plTime tDiff = tNow - m_LastCameraUpdate;
  m_LastCameraUpdate = tNow;

  m_fCameraLerp += tDiff.GetSeconds() * 2.0f;

  if (m_fCameraLerp >= 1.0f)
    m_fCameraLerp = 1.0f;

  plCamera& cam = m_pViewConfig->m_Camera;

  const float fLerpValue = plMath::Sin(plAngle::MakeFromDegree(90.0f * m_fCameraLerp));

  plQuat qRot, qRotFinal;
  qRot = plQuat::MakeShortestRotation(m_vCameraStartDirection, m_vCameraTargetDirection);
  qRotFinal = plQuat::MakeSlerp(plQuat::MakeIdentity(), qRot, fLerpValue);

  const plVec3 vNewDirection = qRotFinal * m_vCameraStartDirection;
  const plVec3 vNewPosition = plMath::Lerp(m_vCameraStartPosition, m_vCameraTargetPosition, fLerpValue);
  const float fNewFovOrDim = plMath::Lerp(m_fCameraStartFovOrDim, m_fCameraTargetFovOrDim, fLerpValue);

  /// \todo Hard coded up vector
  cam.LookAt(vNewPosition, vNewPosition + vNewDirection, m_vCameraUp);
  cam.SetCameraMode(cam.GetCameraMode(), fNewFovOrDim, cam.GetNearPlane(), cam.GetFarPlane());
}

void plQtEngineViewWidget::InterpolateCameraTo(const plVec3& vPosition, const plVec3& vDirection, float fFovOrDim, const plVec3* pNewUpDirection /*= nullptr*/, bool bImmediate /*= false*/)
{
  m_vCameraStartPosition = m_pViewConfig->m_Camera.GetPosition();
  m_vCameraTargetPosition = vPosition;

  m_vCameraStartDirection = m_pViewConfig->m_Camera.GetCenterDirForwards();
  m_vCameraTargetDirection = vDirection;

  if (pNewUpDirection)
    m_vCameraUp = *pNewUpDirection;
  else
    m_vCameraUp = m_pViewConfig->m_Camera.GetCenterDirUp();

  m_vCameraStartDirection.Normalize();
  m_vCameraTargetDirection.Normalize();
  m_vCameraUp.Normalize();


  m_fCameraStartFovOrDim = m_pViewConfig->m_Camera.GetFovOrDim();

  if (fFovOrDim > 0.0f)
    m_fCameraTargetFovOrDim = fFovOrDim;


  PLASMA_ASSERT_DEV(m_fCameraTargetFovOrDim > 0, "Invalid FOV or ortho dimension");

  if (m_vCameraStartPosition == m_vCameraTargetPosition && m_vCameraStartDirection == m_vCameraTargetDirection && m_fCameraStartFovOrDim == m_fCameraTargetFovOrDim)
    return;

  m_LastCameraUpdate = plTime::Now();
  m_fCameraLerp = 0.0f;

  if (bImmediate)
  {
    // make sure the next camera update interpolates all the way
    m_LastCameraUpdate -= plTime::MakeFromSeconds(10);
    m_fCameraLerp = 0.9f;
  }
}

void plQtEngineViewWidget::SetEnablePicking(bool bEnable)
{
  m_bUpdatePickingData = bEnable;
}

void plQtEngineViewWidget::SetPickTransparent(bool bEnable)
{
  if (m_bPickTransparent == bEnable)
    return;

  m_bPickTransparent = bEnable;
  m_LastPickingResult.Reset();
}

void plQtEngineViewWidget::OpenContextMenu(QPoint globalPos)
{
  s_InteractionContext.m_pLastHoveredViewWidget = this;
  s_InteractionContext.m_pLastPickingResult = &m_LastPickingResult;

  OnOpenContextMenu(globalPos);
}


const plObjectPickingResult& plQtEngineViewWidget::PickObject(plUInt16 uiScreenPosX, plUInt16 uiScreenPosY) const
{
  if (!plEditorEngineProcessConnection::GetSingleton()->IsEngineSetup())
  {
    m_LastPickingResult.Reset();
  }
  else
  {
    plViewPickingMsgToEngine msg;
    msg.m_uiViewID = GetViewID();
    msg.m_uiPickPosX = uiScreenPosX * devicePixelRatio();
    msg.m_uiPickPosY = uiScreenPosY * devicePixelRatio();

    GetDocumentWindow()->GetDocument()->SendMessageToEngine(&msg);
  }

  return m_LastPickingResult;
}


plResult plQtEngineViewWidget::PickPlane(plUInt16 uiScreenPosX, plUInt16 uiScreenPosY, const plPlane& plane, plVec3& out_vPosition) const
{
  const auto& cam = m_pViewConfig->m_Camera;

  plMat4 mView = cam.GetViewMatrix();
  plMat4 mProj;
  cam.GetProjectionMatrix((float)width() / (float)height(), mProj);
  plMat4 mViewProj = mProj * mView;
  plMat4 mInvViewProj = mViewProj.GetInverse();

  plVec3 vScreenPos(uiScreenPosX, height() - uiScreenPosY, 0);
  plVec3 vResPos, vResRay;

  if (plGraphicsUtils::ConvertScreenPosToWorldPos(mInvViewProj, 0, 0, width(), height(), vScreenPos, vResPos, &vResRay).Failed())
    return PLASMA_FAILURE;

  if (plane.GetRayIntersection(vResPos, vResRay, nullptr, &out_vPosition))
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}

void plQtEngineViewWidget::HandleViewMessage(const plEditorEngineViewMsg* pMsg)
{
  if (const plViewPickingResultMsgToEditor* pFullMsg = plDynamicCast<const plViewPickingResultMsgToEditor*>(pMsg))
  {
    m_LastPickingResult.m_PickedObject = pFullMsg->m_ObjectGuid;
    m_LastPickingResult.m_PickedComponent = pFullMsg->m_ComponentGuid;
    m_LastPickingResult.m_PickedOther = pFullMsg->m_OtherGuid;
    m_LastPickingResult.m_uiPartIndex = pFullMsg->m_uiPartIndex;
    m_LastPickingResult.m_vPickedPosition = pFullMsg->m_vPickedPosition;
    m_LastPickingResult.m_vPickedNormal = pFullMsg->m_vPickedNormal;
    m_LastPickingResult.m_vPickingRayStart = pFullMsg->m_vPickingRayStartPosition;

    return;
  }
  else if (const plViewMarqueePickingResultMsgToEditor* pFullMsg = plDynamicCast<const plViewMarqueePickingResultMsgToEditor*>(pMsg))
  {
    HandleMarqueePickingResult(pFullMsg);
    return;
  }
}

plPlane plQtEngineViewWidget::GetFallbackPickingPlane(plVec3 vPointOnPlane) const
{
  if (m_pViewConfig->m_Camera.IsPerspective())
  {
    return plPlane::MakeFromNormalAndPoint(plVec3(0, 0, 1), vPointOnPlane);
  }
  else
  {
    return plPlane::MakeFromNormalAndPoint(-m_pViewConfig->m_Camera.GetCenterDirForwards(), vPointOnPlane);
  }
}

void plQtEngineViewWidget::TakeScreenshot(const char* szOutputPath) const
{
  plViewScreenshotMsgToEngine msg;
  msg.m_uiViewID = GetViewID();
  msg.m_sOutputFile = szOutputPath;
  m_pDocumentWindow->GetDocument()->SendMessageToEngine(&msg);
}

////////////////////////////////////////////////////////////////////////
// plQtEngineViewWidget qt overrides
////////////////////////////////////////////////////////////////////////

bool plQtEngineViewWidget::eventFilter(QObject* object, QEvent* event)
{
  if (event->type() == QEvent::Type::ShortcutOverride)
  {
    if (plEditorInputContext::IsAnyInputContextActive())
    {
      // if the active input context does not like other shortcuts,
      // accept this event and thus block further shortcut processing
      // instead Qt will then send a keypress event
      if (plEditorInputContext::GetActiveInputContext()->GetShortcutsDisabled())
        event->accept();
    }
  }

  return false;
}


void plQtEngineViewWidget::paintEvent(QPaintEvent* event)
{
  // event->accept();
}

void plQtEngineViewWidget::resizeEvent(QResizeEvent* event)
{
  m_pDocumentWindow->TriggerRedraw();
}

void plQtEngineViewWidget::keyPressEvent(QKeyEvent* e)
{
  if (e->isAutoRepeat())
    return;

  // if a context is active, it gets exclusive access to the input data
  if (plEditorInputContext::IsAnyInputContextActive())
  {
    if (plEditorInputContext::GetActiveInputContext()->KeyPressEvent(e) == plEditorInput::WasExclusivelyHandled)
      return;
  }

  if (plEditorInputContext::IsAnyInputContextActive())
    return;

  // Override context
  {
    plEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->KeyPressEvent(e) == plEditorInput::WasExclusivelyHandled || plEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->KeyPressEvent(e) == plEditorInput::WasExclusivelyHandled || plEditorInputContext::IsAnyInputContextActive())
      return;
  }

  QWidget::keyPressEvent(e);
}

void plQtEngineViewWidget::keyReleaseEvent(QKeyEvent* e)
{
  if (e->isAutoRepeat())
    return;

  // if a context is active, it gets exclusive access to the input data
  if (plEditorInputContext::IsAnyInputContextActive())
  {
    if (plEditorInputContext::GetActiveInputContext()->KeyReleaseEvent(e) == plEditorInput::WasExclusivelyHandled)
      return;
  }

  if (plEditorInputContext::IsAnyInputContextActive())
    return;

  // Override context
  {
    plEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->KeyReleaseEvent(e) == plEditorInput::WasExclusivelyHandled || plEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->KeyReleaseEvent(e) == plEditorInput::WasExclusivelyHandled || plEditorInputContext::IsAnyInputContextActive())
      return;
  }

  QWidget::keyReleaseEvent(e);
}

void plQtEngineViewWidget::mousePressEvent(QMouseEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (plEditorInputContext::IsAnyInputContextActive())
  {
    if (plEditorInputContext::GetActiveInputContext()->MousePressEvent(e) == plEditorInput::WasExclusivelyHandled)
    {
      e->accept();
      return;
    }
  }

  if (plEditorInputContext::IsAnyInputContextActive())
  {
    e->accept();
    return;
  }

  // Override context
  {
    plEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->MousePressEvent(e) == plEditorInput::WasExclusivelyHandled || plEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->MousePressEvent(e) == plEditorInput::WasExclusivelyHandled || plEditorInputContext::IsAnyInputContextActive())
    {
      e->accept();
      return;
    }
  }

  QWidget::mousePressEvent(e);
}

void plQtEngineViewWidget::mouseReleaseEvent(QMouseEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (plEditorInputContext::IsAnyInputContextActive())
  {
    if (plEditorInputContext::GetActiveInputContext()->MouseReleaseEvent(e) == plEditorInput::WasExclusivelyHandled)
    {
      e->accept();
      return;
    }
  }

  if (plEditorInputContext::IsAnyInputContextActive())
  {
    e->accept();
    return;
  }

  // Override context
  {
    plEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->MouseReleaseEvent(e) == plEditorInput::WasExclusivelyHandled || plEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->MouseReleaseEvent(e) == plEditorInput::WasExclusivelyHandled || plEditorInputContext::IsAnyInputContextActive())
    {
      e->accept();
      return;
    }
  }

  QWidget::mouseReleaseEvent(e);
}

void plQtEngineViewWidget::mouseMoveEvent(QMouseEvent* e)
{
  s_InteractionContext.m_pLastHoveredViewWidget = this;
  s_InteractionContext.m_pLastPickingResult = &m_LastPickingResult;

  // kick off the picking
  PickObject(e->pos().x(), e->pos().y());

  // if a context is active, it gets exclusive access to the input data
  if (plEditorInputContext::IsAnyInputContextActive())
  {
    if (plEditorInputContext::GetActiveInputContext()->MouseMoveEvent(e) == plEditorInput::WasExclusivelyHandled)
    {
      e->accept();
      return;
    }
  }

  if (plEditorInputContext::IsAnyInputContextActive())
  {
    e->accept();
    return;
  }

  // Override context
  {
    plEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->MouseMoveEvent(e) == plEditorInput::WasExclusivelyHandled || plEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->MouseMoveEvent(e) == plEditorInput::WasExclusivelyHandled || plEditorInputContext::IsAnyInputContextActive())
    {
      e->accept();
      return;
    }
  }

  QWidget::mouseMoveEvent(e);
}

void plQtEngineViewWidget::wheelEvent(QWheelEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (plEditorInputContext::IsAnyInputContextActive())
  {
    if (plEditorInputContext::GetActiveInputContext()->WheelEvent(e) == plEditorInput::WasExclusivelyHandled)
      return;
  }

  if (plEditorInputContext::IsAnyInputContextActive())
    return;

  // Override context
  {
    plEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->WheelEvent(e) == plEditorInput::WasExclusivelyHandled || plEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->WheelEvent(e) == plEditorInput::WasExclusivelyHandled || plEditorInputContext::IsAnyInputContextActive())
      return;
  }

  QWidget::wheelEvent(e);
}

void plQtEngineViewWidget::focusOutEvent(QFocusEvent* e)
{
  if (plEditorInputContext::IsAnyInputContextActive())
  {
    plEditorInputContext::GetActiveInputContext()->FocusLost(false);
    plEditorInputContext::SetActiveInputContext(nullptr);
  }

  QWidget::focusOutEvent(e);
}


void plQtEngineViewWidget::dragEnterEvent(QDragEnterEvent* e)
{
  m_bInDragAndDropOperation = true;
}


void plQtEngineViewWidget::dragLeaveEvent(QDragLeaveEvent* e)
{
  m_bInDragAndDropOperation = false;
}


void plQtEngineViewWidget::dropEvent(QDropEvent* e)
{
  m_bInDragAndDropOperation = false;
}


////////////////////////////////////////////////////////////////////////
// plQtEngineViewWidget protected functions
////////////////////////////////////////////////////////////////////////

void plQtEngineViewWidget::EngineViewProcessEventHandler(const plEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case plEditorEngineProcessConnection::Event::Type::ProcessCrashed:
    {
      ShowRestartButton(true);
    }
    break;

    case plEditorEngineProcessConnection::Event::Type::ProcessStarted:
    {
      ShowRestartButton(false);
    }
    break;

    case plEditorEngineProcessConnection::Event::Type::ProcessShutdown:
      break;

    case plEditorEngineProcessConnection::Event::Type::ProcessMessage:
      break;

    case plEditorEngineProcessConnection::Event::Type::Invalid:
      PLASMA_ASSERT_DEV(false, "Invalid message should never happen");
      break;

    case plEditorEngineProcessConnection::Event::Type::ProcessRestarted:
      break;
  }
}

void plQtEngineViewWidget::ShowRestartButton(bool bShow)
{
  plQtScopedUpdatesDisabled _(this);

  if (m_pRestartButtonLayout == nullptr && bShow == true)
  {
    m_pRestartButtonLayout = new QHBoxLayout(this);
    m_pRestartButtonLayout->setContentsMargins(0, 0, 0, 0);

    setLayout(m_pRestartButtonLayout);

    m_pRestartButton = new QPushButton(this);
    m_pRestartButton->setText("Restart Engine View Process");
    m_pRestartButton->setVisible(plEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed());
    m_pRestartButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_pRestartButton->connect(m_pRestartButton, &QPushButton::clicked, this, &plQtEngineViewWidget::SlotRestartEngineProcess);

    m_pRestartButtonLayout->addWidget(m_pRestartButton);
  }

  if (m_pRestartButton)
  {
    m_pRestartButton->setVisible(bShow);

    if (bShow)
      m_pRestartButton->update();
  }
}


////////////////////////////////////////////////////////////////////////
// plQtEngineViewWidget private slots
////////////////////////////////////////////////////////////////////////

void plQtEngineViewWidget::SlotRestartEngineProcess()
{
  plEditorEngineProcessConnection::GetSingleton()->RestartProcess().IgnoreResult();
}


////////////////////////////////////////////////////////////////////////
// plQtViewWidgetContainer
////////////////////////////////////////////////////////////////////////

plQtViewWidgetContainer::plQtViewWidgetContainer(QWidget* pParent, plQtEngineViewWidget* pViewWidget, const char* szToolBarMapping)
  : QWidget(pParent)
{
  setBackgroundRole(QPalette::Base);
  setAutoFillBackground(true);

  m_pLayout = new QVBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  m_pLayout->setSpacing(0);
  setLayout(m_pLayout);

  m_pViewWidget = pViewWidget;
  m_pViewWidget->setParent(this);

  if (!plStringUtils::IsNullOrEmpty(szToolBarMapping))
  {
    // Add Tool Bar
    plQtToolBarActionMapView* pToolBar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = szToolBarMapping;
    context.m_pDocument = pViewWidget->GetDocumentWindow()->GetDocument();
    context.m_pWindow = m_pViewWidget;
    pToolBar->SetActionContext(context);
    m_pLayout->addWidget(pToolBar, 0);
  }

  m_pLayout->addWidget(m_pViewWidget, 1);
}

plQtViewWidgetContainer::~plQtViewWidgetContainer() = default;
