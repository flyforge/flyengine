#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/EditTools/StandardGizmoEditTools.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorFramework/InputContexts/OrthoGizmoContext.h>
#include <EditorFramework/Preferences/ScenePreferences.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTranslateGizmoEditTool, 1, plRTTIDefaultAllocator<plTranslateGizmoEditTool>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plTranslateGizmoEditTool::plTranslateGizmoEditTool()
{
  m_TranslateGizmo.m_GizmoEvents.AddEventHandler(plMakeDelegate(&plTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

plTranslateGizmoEditTool::~plTranslateGizmoEditTool()
{
  m_TranslateGizmo.m_GizmoEvents.RemoveEventHandler(plMakeDelegate(&plTranslateGizmoEditTool::TransformationGizmoEventHandler, this));

  auto& events = plPreferences::QueryPreferences<plScenePreferencesUser>(GetDocument())->m_ChangedEvent;

  if (events.HasEventHandler(plMakeDelegate(&plTranslateGizmoEditTool::OnPreferenceChange, this)))
    events.RemoveEventHandler(plMakeDelegate(&plTranslateGizmoEditTool::OnPreferenceChange, this));
}

void plTranslateGizmoEditTool::OnActiveChanged(bool bIsActive)
{
  if (bIsActive)
  {
    m_TranslateGizmo.UpdateStatusBarText(GetWindow());
  }
}

void plTranslateGizmoEditTool::OnConfigured()
{
  SUPER::OnConfigured();

  m_TranslateGizmo.SetOwner(GetWindow(), nullptr);

  plPreferences::QueryPreferences<plScenePreferencesUser>(GetDocument())
    ->m_ChangedEvent.AddEventHandler(plMakeDelegate(&plTranslateGizmoEditTool::OnPreferenceChange, this));
}

void plTranslateGizmoEditTool::ApplyGizmoVisibleState(bool visible)
{
  m_TranslateGizmo.SetVisible(visible);
}

void plTranslateGizmoEditTool::ApplyGizmoTransformation(const plTransform& transform)
{
  m_TranslateGizmo.SetTransformation(transform);
}

void plTranslateGizmoEditTool::TransformationGizmoEventHandlerImpl(const plGizmoEvent& e)
{
  plObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();
  switch (e.m_Type)
  {
    case plGizmoEvent::Type::BeginInteractions:
    {
      const bool bDuplicate =
        QApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ShiftModifier) && GetGizmoInterface()->CanDuplicateSelection();

      // duplicate the object when shift is held while dragging the item
      if (bDuplicate && (e.m_pGizmo == &m_TranslateGizmo || e.m_pGizmo->GetDynamicRTTI()->IsDerivedFrom<plOrthoGizmoContext>()))
      {
        m_bMergeTransactions = true;
        GetGizmoInterface()->DuplicateSelection();
      }

      if (e.m_pGizmo == &m_TranslateGizmo && QApplication::keyboardModifiers() & Qt::KeyboardModifier::ControlModifier)
      {
        m_TranslateGizmo.SetMovementMode(plTranslateGizmo::MovementMode::MouseDiff);
      }
    }
    break;

    case plGizmoEvent::Type::Interaction:
    {
      auto pDocument = GetDocument();
      plTransform tNew;

      if (e.m_pGizmo == &m_TranslateGizmo)
      {
        const plVec3 vTranslate = m_TranslateGizmo.GetTranslationResult();

        for (plUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_vPosition += vTranslate;

          if (GetDocument()->GetGizmoMoveParentOnly())
            pDocument->SetGlobalTransformParentOnly(obj.m_pObject, tNew, TransformationChanges::Translation);
          else
            pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Translation);
        }

        if (e.m_pGizmo == &m_TranslateGizmo && QApplication::keyboardModifiers() & Qt::KeyboardModifier::ControlModifier)
        {
          m_TranslateGizmo.SetMovementMode(plTranslateGizmo::MovementMode::MouseDiff);

          auto* pFocusedView = GetWindow()->GetFocusedViewWidget();
          if (pFocusedView != nullptr)
          {
            const plVec3 d = m_TranslateGizmo.GetTranslationDiff();
            pFocusedView->m_pViewConfig->m_Camera.MoveGlobally(d.x, d.y, d.z);
          }
        }
        else
        {
          m_TranslateGizmo.SetMovementMode(plTranslateGizmo::MovementMode::ScreenProjection);
        }
      }

      if (e.m_pGizmo->GetDynamicRTTI()->IsDerivedFrom<plOrthoGizmoContext>())
      {
        const plOrthoGizmoContext* pOrtho = static_cast<const plOrthoGizmoContext*>(e.m_pGizmo);

        const plVec3 vTranslate = pOrtho->GetTranslationResult();

        for (plUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_vPosition += vTranslate;

          pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Translation);
        }

        if (QApplication::keyboardModifiers() & Qt::KeyboardModifier::ControlModifier)
        {
          // move the camera with the translated object

          auto* pFocusedView = GetWindow()->GetFocusedViewWidget();
          if (pFocusedView != nullptr)
          {
            const plVec3 d = pOrtho->GetTranslationDiff();
            pFocusedView->m_pViewConfig->m_Camera.MoveGlobally(d.x, d.y, d.z);
          }
        }
      }

      pAccessor->FinishTransaction();
    }
    break;

    default:
      break;
  }
}

void plTranslateGizmoEditTool::OnPreferenceChange(plPreferences* pref)
{
  plScenePreferencesUser* pPref = plDynamicCast<plScenePreferencesUser*>(pref);

  m_TranslateGizmo.SetCameraSpeed(plCameraMoveContext::ConvertCameraSpeed(pPref->GetCameraSpeed()));
}

void plTranslateGizmoEditTool::GetGridSettings(plGridSettingsMsgToEngine& ref_msg)
{
  auto pSceneDoc = GetDocument();
  plScenePreferencesUser* pPreferences = plPreferences::QueryPreferences<plScenePreferencesUser>(GetDocument());

  // if density != 0, it is enabled at least in ortho mode
  ref_msg.m_fGridDensity = plSnapProvider::GetTranslationSnapValue() * (pSceneDoc->GetGizmoWorldSpace() ? 1.0f : -1.0f); // negative density = local space

  // to be active in perspective mode, tangents have to be non-zero
  ref_msg.m_vGridTangent1.SetZero();
  ref_msg.m_vGridTangent2.SetZero();

  plTranslateGizmo& translateGizmo = m_TranslateGizmo;

  if (pPreferences->GetShowGrid() && translateGizmo.IsVisible())
  {
    ref_msg.m_vGridCenter = translateGizmo.GetStartPosition();

    if (translateGizmo.GetTranslateMode() == plTranslateGizmo::TranslateMode::Axis)
      ref_msg.m_vGridCenter = translateGizmo.GetTransformation().m_vPosition;

    if (pSceneDoc->GetGizmoWorldSpace())
    {
      plSnapProvider::SnapTranslation(ref_msg.m_vGridCenter);

      switch (translateGizmo.GetLastPlaneInteraction())
      {
        case plTranslateGizmo::PlaneInteraction::PlaneX:
          ref_msg.m_vGridCenter.y = plMath::RoundToMultiple(ref_msg.m_vGridCenter.y, plSnapProvider::GetTranslationSnapValue() * 10);
          ref_msg.m_vGridCenter.z = plMath::RoundToMultiple(ref_msg.m_vGridCenter.z, plSnapProvider::GetTranslationSnapValue() * 10);
          break;
        case plTranslateGizmo::PlaneInteraction::PlaneY:
          ref_msg.m_vGridCenter.x = plMath::RoundToMultiple(ref_msg.m_vGridCenter.x, plSnapProvider::GetTranslationSnapValue() * 10);
          ref_msg.m_vGridCenter.z = plMath::RoundToMultiple(ref_msg.m_vGridCenter.z, plSnapProvider::GetTranslationSnapValue() * 10);
          break;
        case plTranslateGizmo::PlaneInteraction::PlaneZ:
          ref_msg.m_vGridCenter.x = plMath::RoundToMultiple(ref_msg.m_vGridCenter.x, plSnapProvider::GetTranslationSnapValue() * 10);
          ref_msg.m_vGridCenter.y = plMath::RoundToMultiple(ref_msg.m_vGridCenter.y, plSnapProvider::GetTranslationSnapValue() * 10);
          break;
      }
    }

    switch (translateGizmo.GetLastPlaneInteraction())
    {
      case plTranslateGizmo::PlaneInteraction::PlaneX:
        ref_msg.m_vGridTangent1 = translateGizmo.GetTransformation().m_qRotation * plVec3(0, 1, 0);
        ref_msg.m_vGridTangent2 = translateGizmo.GetTransformation().m_qRotation * plVec3(0, 0, 1);
        break;
      case plTranslateGizmo::PlaneInteraction::PlaneY:
        ref_msg.m_vGridTangent1 = translateGizmo.GetTransformation().m_qRotation * plVec3(1, 0, 0);
        ref_msg.m_vGridTangent2 = translateGizmo.GetTransformation().m_qRotation * plVec3(0, 0, 1);
        break;
      case plTranslateGizmo::PlaneInteraction::PlaneZ:
        ref_msg.m_vGridTangent1 = translateGizmo.GetTransformation().m_qRotation * plVec3(1, 0, 0);
        ref_msg.m_vGridTangent2 = translateGizmo.GetTransformation().m_qRotation * plVec3(0, 1, 0);
        break;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRotateGizmoEditTool, 1, plRTTIDefaultAllocator<plRotateGizmoEditTool>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plRotateGizmoEditTool::plRotateGizmoEditTool()
{
  m_RotateGizmo.m_GizmoEvents.AddEventHandler(plMakeDelegate(&plTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

plRotateGizmoEditTool::~plRotateGizmoEditTool()
{
  m_RotateGizmo.m_GizmoEvents.RemoveEventHandler(plMakeDelegate(&plTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

void plRotateGizmoEditTool::OnConfigured()
{
  SUPER::OnConfigured();

  m_RotateGizmo.SetOwner(GetWindow(), nullptr);
}

void plRotateGizmoEditTool::ApplyGizmoVisibleState(bool visible)
{
  m_RotateGizmo.SetVisible(visible);
}

void plRotateGizmoEditTool::ApplyGizmoTransformation(const plTransform& transform)
{
  m_RotateGizmo.SetTransformation(transform);
}

void plRotateGizmoEditTool::TransformationGizmoEventHandlerImpl(const plGizmoEvent& e)
{
  plObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();
  switch (e.m_Type)
  {
    case plGizmoEvent::Type::BeginInteractions:
    {
      const bool bDuplicate =
        QApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ShiftModifier) && GetGizmoInterface()->CanDuplicateSelection();

      // duplicate the object when shift is held while dragging the item
      if (e.m_pGizmo == &m_RotateGizmo && bDuplicate)
      {
        m_bMergeTransactions = true;
        GetGizmoInterface()->DuplicateSelection();
      }
    }
    break;

    case plGizmoEvent::Type::Interaction:
    {
      auto pDocument = GetDocument();
      plTransform tNew;

      if (e.m_pGizmo == &m_RotateGizmo)
      {
        const plQuat qRotation = m_RotateGizmo.GetRotationResult();
        const plVec3 vPivot = m_RotateGizmo.GetTransformation().m_vPosition;

        for (plUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_qRotation = qRotation * obj.m_GlobalTransform.m_qRotation;
          tNew.m_vPosition = vPivot + qRotation * (obj.m_GlobalTransform.m_vPosition - vPivot);

          if (GetDocument()->GetGizmoMoveParentOnly())
            pDocument->SetGlobalTransformParentOnly(obj.m_pObject, tNew, TransformationChanges::Rotation | TransformationChanges::Translation);
          else
            pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Rotation | TransformationChanges::Translation);
        }
      }

      if (e.m_pGizmo->GetDynamicRTTI()->IsDerivedFrom<plOrthoGizmoContext>())
      {
        const plOrthoGizmoContext* pOrtho = static_cast<const plOrthoGizmoContext*>(e.m_pGizmo);

        const plQuat qRotation = pOrtho->GetRotationResult();

        // const plVec3 vPivot(0);

        for (plUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_qRotation = qRotation * obj.m_GlobalTransform.m_qRotation;
          // tNew.m_vPosition = vPivot + qRotation * (obj.m_GlobalTransform.m_vPosition - vPivot);

          pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Rotation);
        }
      }

      pAccessor->FinishTransaction();
    }
    break;

    default:
      break;
  }
}

void plRotateGizmoEditTool::OnActiveChanged(bool bIsActive)
{
  if (bIsActive)
  {
    m_RotateGizmo.UpdateStatusBarText(GetWindow());
  }
}

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plScaleGizmoEditTool, 1, plRTTIDefaultAllocator<plScaleGizmoEditTool>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plScaleGizmoEditTool::plScaleGizmoEditTool()
{
  m_ScaleGizmo.m_GizmoEvents.AddEventHandler(plMakeDelegate(&plTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

plScaleGizmoEditTool::~plScaleGizmoEditTool()
{
  m_ScaleGizmo.m_GizmoEvents.RemoveEventHandler(plMakeDelegate(&plTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

void plScaleGizmoEditTool::OnActiveChanged(bool bIsActive)
{
  if (bIsActive)
  {
    m_ScaleGizmo.UpdateStatusBarText(GetWindow());
  }
}

void plScaleGizmoEditTool::OnConfigured()
{
  SUPER::OnConfigured();

  m_ScaleGizmo.SetOwner(GetWindow(), nullptr);
}

void plScaleGizmoEditTool::ApplyGizmoVisibleState(bool visible)
{
  m_ScaleGizmo.SetVisible(visible);
}

void plScaleGizmoEditTool::ApplyGizmoTransformation(const plTransform& transform)
{
  m_ScaleGizmo.SetTransformation(transform);
}

void plScaleGizmoEditTool::TransformationGizmoEventHandlerImpl(const plGizmoEvent& e)
{
  plObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();
  switch (e.m_Type)
  {
    case plGizmoEvent::Type::Interaction:
    {
      plTransform tNew;

      bool bCancel = false;

      if (e.m_pGizmo == &m_ScaleGizmo)
      {
        const plVec3 vScale = m_ScaleGizmo.GetScalingResult();
        if (vScale.x == vScale.y && vScale.x == vScale.z)
        {
          for (plUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
          {
            const auto& obj = m_GizmoSelection[sel];
            float fNewScale = obj.m_fLocalUniformScaling * vScale.x;

            if (pAccessor->SetValue(obj.m_pObject, "LocalUniformScaling", fNewScale).m_Result.Failed())
            {
              bCancel = true;
              break;
            }
          }
        }
        else
        {
          for (plUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
          {
            const auto& obj = m_GizmoSelection[sel];
            plVec3 vNewScale = obj.m_vLocalScaling.CompMul(vScale);

            if (pAccessor->SetValue(obj.m_pObject, "LocalScaling", vNewScale).m_Result.Failed())
            {
              bCancel = true;
              break;
            }
          }
        }
      }

      if (e.m_pGizmo->GetDynamicRTTI()->IsDerivedFrom<plOrthoGizmoContext>())
      {
        const plOrthoGizmoContext* pOrtho = static_cast<const plOrthoGizmoContext*>(e.m_pGizmo);

        const float fScale = pOrtho->GetScalingResult();
        for (plUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];
          const float fNewScale = obj.m_fLocalUniformScaling * fScale;

          if (pAccessor->SetValue(obj.m_pObject, "LocalUniformScaling", fNewScale).m_Result.Failed())
          {
            bCancel = true;
            break;
          }
        }
      }

      if (bCancel)
        pAccessor->CancelTransaction();
      else
        pAccessor->FinishTransaction();
    }
    break;

    default:
      break;
  }
}

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDragToPositionGizmoEditTool, 1, plRTTIDefaultAllocator<plDragToPositionGizmoEditTool>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plDragToPositionGizmoEditTool::plDragToPositionGizmoEditTool()
{

  m_DragToPosGizmo.m_GizmoEvents.AddEventHandler(plMakeDelegate(&plTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

plDragToPositionGizmoEditTool::~plDragToPositionGizmoEditTool()
{
  m_DragToPosGizmo.m_GizmoEvents.RemoveEventHandler(plMakeDelegate(&plTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

void plDragToPositionGizmoEditTool::OnActiveChanged(bool bIsActive)
{
  if (bIsActive)
  {
    m_DragToPosGizmo.UpdateStatusBarText(GetWindow());
  }
}

void plDragToPositionGizmoEditTool::OnConfigured()
{
  SUPER::OnConfigured();

  m_DragToPosGizmo.SetOwner(GetWindow(), nullptr);
}

void plDragToPositionGizmoEditTool::ApplyGizmoVisibleState(bool visible)
{
  m_DragToPosGizmo.SetVisible(visible);
}

void plDragToPositionGizmoEditTool::ApplyGizmoTransformation(const plTransform& transform)
{
  m_DragToPosGizmo.SetTransformation(transform);
}

void plDragToPositionGizmoEditTool::TransformationGizmoEventHandlerImpl(const plGizmoEvent& e)
{
  plObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();
  switch (e.m_Type)
  {
    case plGizmoEvent::Type::BeginInteractions:
    {
      const bool bDuplicate =
        QApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ShiftModifier) && GetGizmoInterface()->CanDuplicateSelection();

      // duplicate the object when shift is held while dragging the item
      if (e.m_pGizmo == &m_DragToPosGizmo && bDuplicate)
      {
        m_bMergeTransactions = true;
        GetGizmoInterface()->DuplicateSelection();
      }
    }
    break;

    case plGizmoEvent::Type::Interaction:
    {
      auto pDocument = GetDocument();
      plTransform tNew;

      if (e.m_pGizmo == &m_DragToPosGizmo)
      {
        const plVec3 vTranslate = m_DragToPosGizmo.GetTranslationResult();
        const plQuat qRot = m_DragToPosGizmo.GetRotationResult();

        for (plUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_vPosition += vTranslate;

          if (m_DragToPosGizmo.ModifiesRotation())
          {
            tNew.m_qRotation = qRot;
          }

          if (GetDocument()->GetGizmoMoveParentOnly())
            pDocument->SetGlobalTransformParentOnly(obj.m_pObject, tNew, TransformationChanges::Rotation | TransformationChanges::Translation);
          else
            pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Translation | TransformationChanges::Rotation);
        }
      }

      pAccessor->FinishTransaction();
    }
    break;

    default:
      break;
  }
}
