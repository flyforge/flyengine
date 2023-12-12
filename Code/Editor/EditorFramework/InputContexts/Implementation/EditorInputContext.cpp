#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <GuiFoundation/Widgets/WidgetUtils.h>

PlasmaEditorInputContext* PlasmaEditorInputContext::s_pActiveInputContext = nullptr;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(PlasmaEditorInputContext, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PlasmaEditorInputContext::PlasmaEditorInputContext()
{
  m_pOwnerWindow = nullptr;
  m_pOwnerView = nullptr;
  m_bDisableShortcuts = false;
  m_bJustWrappedMouse = false;
}

PlasmaEditorInputContext::~PlasmaEditorInputContext()
{
  if (s_pActiveInputContext == this)
    SetActiveInputContext(nullptr);
}


void PlasmaEditorInputContext::FocusLost(bool bCancel)
{
  DoFocusLost(bCancel);

  // reset mouse mode, if necessary
  SetMouseMode(MouseMode::Normal);

  UpdateStatusBarText(GetOwnerWindow());
}

PlasmaEditorInput PlasmaEditorInputContext::DoKeyPressEvent(QKeyEvent* e)
{
  if (!IsActiveInputContext())
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (e->key() == Qt::Key_Escape)
  {
    FocusLost(true);
    SetActiveInputContext(nullptr);
    return PlasmaEditorInput::WasExclusivelyHandled;
  }

  return PlasmaEditorInput::MayBeHandledByOthers;
}


PlasmaEditorInput PlasmaEditorInputContext::MouseMoveEvent(QMouseEvent* e)
{
  if (m_MouseMode != MouseMode::Normal)
  {
    if (m_bJustWrappedMouse)
    {
      const plVec2I32 curPos(e->globalX(), e->globalY());
      const plVec2I32 diffToOld = curPos - m_vMousePosBeforeWrap;
      const plVec2I32 diffToNew = curPos - m_vExpectedMousePosition;

      if (diffToOld.GetLengthSquared() < diffToNew.GetLengthSquared())
      {
        // this is an invalid message, it was still in the message queue with old coordinates and should be discarded

        return PlasmaEditorInput::WasExclusivelyHandled;
      }

      m_bJustWrappedMouse = false;
    }
  }

  return DoMouseMoveEvent(e);
}

void PlasmaEditorInputContext::MakeActiveInputContext(bool bActive /*= true*/)
{
  if (bActive)
    s_pActiveInputContext = this;
  else
    s_pActiveInputContext = nullptr;
}

void PlasmaEditorInputContext::UpdateActiveInputContext()
{
  if (s_pActiveInputContext != nullptr)
    s_pActiveInputContext->UpdateContext();
}

bool PlasmaEditorInputContext::IsActiveInputContext() const
{
  return s_pActiveInputContext == this;
}

void PlasmaEditorInputContext::SetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView)
{
  m_pOwnerWindow = pOwnerWindow;
  m_pOwnerView = pOwnerView;

  OnSetOwner(m_pOwnerWindow, m_pOwnerView);
}

plQtEngineDocumentWindow* PlasmaEditorInputContext::GetOwnerWindow() const
{
  PLASMA_ASSERT_DEBUG(m_pOwnerWindow != nullptr, "Owner window pointer has not been set");
  return m_pOwnerWindow;
}

plQtEngineViewWidget* PlasmaEditorInputContext::GetOwnerView() const
{
  plQtEngineViewWidget* pView = m_pOwnerView;

  if (pView == nullptr)
  {
    pView = plQtEngineViewWidget::GetInteractionContext().m_pLastHoveredViewWidget;
  }

  PLASMA_ASSERT_DEBUG(pView != nullptr, "Owner view pointer has not been set");
  return pView;
}

plVec2I32 PlasmaEditorInputContext::SetMouseMode(MouseMode newMode)
{
  const QPoint curPos = QCursor::pos();

  if (m_MouseMode == newMode)
    return plVec2I32(curPos.x(), curPos.y());

  m_bJustWrappedMouse = false;

  if (newMode != MouseMode::Normal)
  {
    const QRect dsize = plWidgetUtils::GetClosestScreen(curPos).availableGeometry();

    m_MouseWrapRect.x = dsize.x() + 10;
    m_MouseWrapRect.y = dsize.y() + 10;
    m_MouseWrapRect.width = dsize.width() - 20;
    m_MouseWrapRect.height = dsize.height() - 20;
  }

  if (m_MouseMode == MouseMode::HideAndWrapAtScreenBorders)
  {
    QCursor::setPos(QPoint(m_vMouseRestorePosition.x, m_vMouseRestorePosition.y));
    QApplication::restoreOverrideCursor();
  }

  if (newMode == MouseMode::HideAndWrapAtScreenBorders)
  {
    m_vMouseRestorePosition.Set(curPos.x(), curPos.y());
    QApplication::setOverrideCursor(Qt::BlankCursor);
  }

  m_MouseMode = newMode;

  return plVec2I32(curPos.x(), curPos.y());
}

plVec2I32 PlasmaEditorInputContext::UpdateMouseMode(QMouseEvent* e)
{
  const plVec2I32 curPos(e->globalX(), e->globalY());

  if (m_MouseMode == MouseMode::Normal)
    return curPos;

  plVec2I32 newPos = curPos;

  if (curPos.x > (plInt32)m_MouseWrapRect.Right())
    newPos.x = m_MouseWrapRect.Left() + (curPos.x - m_MouseWrapRect.Right());

  if (curPos.x < (plInt32)m_MouseWrapRect.Left())
    newPos.x = m_MouseWrapRect.Right() - (m_MouseWrapRect.Left() - curPos.x);

  if (curPos.y > (plInt32)m_MouseWrapRect.Bottom())
    newPos.y = m_MouseWrapRect.Top() + (curPos.y - m_MouseWrapRect.Bottom());

  if (curPos.y < (plInt32)m_MouseWrapRect.Top())
    newPos.y = m_MouseWrapRect.Bottom() - (m_MouseWrapRect.Top() - curPos.y);

  if (curPos != newPos)
  {
    // wrap the mouse around the screen borders
    QCursor::setPos(QPoint(newPos.x, newPos.y));

    // store where we expect the next mouse position
    m_vExpectedMousePosition = newPos;
    m_vMousePosBeforeWrap = curPos;

    // next mouse message must be inspected for outliers
    m_bJustWrappedMouse = true;
  }

  return newPos;
}
