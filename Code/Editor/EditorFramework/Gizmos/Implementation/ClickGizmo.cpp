#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/ClickGizmo.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plClickGizmo, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plClickGizmo::plClickGizmo()
{
  m_hShape.ConfigureHandle(this, plEngineGizmoHandleType::Sphere, plColor::White, plGizmoFlags::Pickable);

  SetVisible(false);
  SetTransformation(plTransform::MakeIdentity());
}

void plClickGizmo::SetColor(const plColor& color)
{
  m_hShape.SetColor(color);
}

void plClickGizmo::OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hShape);
}

void plClickGizmo::OnVisibleChanged(bool bVisible)
{
  m_hShape.SetVisible(bVisible);
}

void plClickGizmo::OnTransformationChanged(const plTransform& transform)
{
  m_hShape.SetTransformation(transform);
}

void plClickGizmo::DoFocusLost(bool bCancel)
{
  plViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);
}

plEditorInput plClickGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return plEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return plEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle != &m_hShape)
    return plEditorInput::MayBeHandledByOthers;

  plViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  SetActiveInputContext(this);

  return plEditorInput::WasExclusivelyHandled;
}

plEditorInput plClickGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return plEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return plEditorInput::WasExclusivelyHandled;

  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = plGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return plEditorInput::WasExclusivelyHandled;
}
