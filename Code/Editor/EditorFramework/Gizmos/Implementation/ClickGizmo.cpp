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
  m_hShape.ConfigureHandle(this, PlasmaEngineGizmoHandleType::Sphere, plColor::White, plGizmoFlags::Pickable);

  SetVisible(false);
  SetTransformation(plTransform::IdentityTransform());
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

PlasmaEditorInput plClickGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return PlasmaEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle != &m_hShape)
    return PlasmaEditorInput::MayBeHandledByOthers;

  plViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  SetActiveInputContext(this);

  return PlasmaEditorInput::WasExclusivelyHandled;
}

PlasmaEditorInput plClickGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return PlasmaEditorInput::WasExclusivelyHandled;

  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = plGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return PlasmaEditorInput::WasExclusivelyHandled;
}
