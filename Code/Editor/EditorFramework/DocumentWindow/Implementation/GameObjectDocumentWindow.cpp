#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorFramework/Manipulators/ManipulatorAdapterRegistry.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

plQtGameObjectDocumentWindow::plQtGameObjectDocumentWindow(plGameObjectDocument* pDocument)
  : plQtEngineDocumentWindow(pDocument)
{
  pDocument->m_GameObjectEvents.AddEventHandler(plMakeDelegate(&plQtGameObjectDocumentWindow::GameObjectEventHandler, this));
  plSnapProvider::s_Events.AddEventHandler(plMakeDelegate(&plQtGameObjectDocumentWindow::SnapProviderEventHandler, this));
}

plQtGameObjectDocumentWindow::~plQtGameObjectDocumentWindow()
{
  GetGameObjectDocument()->m_GameObjectEvents.RemoveEventHandler(plMakeDelegate(&plQtGameObjectDocumentWindow::GameObjectEventHandler, this));
  plSnapProvider::s_Events.RemoveEventHandler(plMakeDelegate(&plQtGameObjectDocumentWindow::SnapProviderEventHandler, this));
}

plGameObjectDocument* plQtGameObjectDocumentWindow::GetGameObjectDocument() const
{
  return static_cast<plGameObjectDocument*>(GetDocument());
}

plWorldSettingsMsgToEngine plQtGameObjectDocumentWindow::GetWorldSettings() const
{
  plWorldSettingsMsgToEngine msg;
  auto pGameObjectDoc = GetGameObjectDocument();
  msg.m_bRenderOverlay = pGameObjectDoc->GetRenderSelectionOverlay();
  msg.m_bRenderShapeIcons = pGameObjectDoc->GetRenderShapeIcons();
  msg.m_bRenderSelectionBoxes = pGameObjectDoc->GetRenderVisualizers();
  msg.m_bAddAmbientLight = pGameObjectDoc->GetAddAmbientLight();
  return msg;
}

plGridSettingsMsgToEngine plQtGameObjectDocumentWindow::GetGridSettings() const
{
  plGridSettingsMsgToEngine msg;

  if (auto pTool = GetGameObjectDocument()->GetActiveEditTool())
  {
    pTool->GetGridSettings(msg);
  }
  else
  {
    plManipulatorAdapterRegistry::GetSingleton()->QueryGridSettings(GetDocument(), msg);
  }

  return msg;
}

void plQtGameObjectDocumentWindow::ProcessMessageEventHandler(const PlasmaEditorEngineDocumentMsg* pMsg)
{
  plQtEngineDocumentWindow::ProcessMessageEventHandler(pMsg);
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plQuerySelectionBBoxResultMsgToEditor>())
  {
    const plQuerySelectionBBoxResultMsgToEditor* msg = static_cast<const plQuerySelectionBBoxResultMsgToEditor*>(pMsg);

    if (msg->m_uiViewID == 0xFFFFFFFF)
    {
      for (auto pView : m_ViewWidgets)
      {
        if (!pView)
          continue;

        if (msg->m_iPurpose == 0)
          HandleFocusOnSelection(msg, static_cast<plQtGameObjectViewWidget*>(pView));
      }
    }
    else
    {
      plQtGameObjectViewWidget* pSceneView = static_cast<plQtGameObjectViewWidget*>(GetViewWidgetByID(msg->m_uiViewID));

      if (!pSceneView)
        return;

      if (msg->m_iPurpose == 0)
        HandleFocusOnSelection(msg, pSceneView);
    }

    return;
  }
}

void plQtGameObjectDocumentWindow::GameObjectEventHandler(const plGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case plGameObjectEvent::Type::TriggerFocusOnSelection_Hovered:
      FocusOnSelectionHoveredView();
      break;

    case plGameObjectEvent::Type::TriggerFocusOnSelection_All:
      FocusOnSelectionAllViews();
      break;

    default:
      break;
  }
}

void plQtGameObjectDocumentWindow::FocusOnSelectionAllViews()
{
  const auto& sel = GetDocument()->GetSelectionManager()->GetSelection();

  if (sel.IsEmpty())
    return;
  if (!sel.PeekBack()->GetTypeAccessor().GetType()->IsDerivedFrom<plGameObject>())
    return;

  plQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = 0;
  GetDocument()->SendMessageToEngine(&msg);
}

void plQtGameObjectDocumentWindow::FocusOnSelectionHoveredView()
{
  const auto& sel = GetDocument()->GetSelectionManager()->GetSelection();

  if (sel.IsEmpty())
    return;
  if (!sel.PeekBack()->GetTypeAccessor().GetType()->IsDerivedFrom<plGameObject>())
    return;

  auto pView = GetHoveredViewWidget();

  if (pView == nullptr)
    return;

  plQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = pView->GetViewID();
  msg.m_iPurpose = 0;
  GetDocument()->SendMessageToEngine(&msg);
}

void plQtGameObjectDocumentWindow::HandleFocusOnSelection(const plQuerySelectionBBoxResultMsgToEditor* pMsg, plQtGameObjectViewWidget* pSceneView)
{
  const plVec3 vPivotPoint = pMsg->m_vCenter;

  const plCamera& cam = pSceneView->m_pViewConfig->m_Camera;

  plVec3 vNewCameraPosition = cam.GetCenterPosition();
  plVec3 vNewCameraDirection = cam.GetDirForwards();
  float fNewFovOrDim = cam.GetFovOrDim();

  if (pSceneView->width() == 0 || pSceneView->height() == 0)
    return;

  const float fApsectRation = (float)pSceneView->width() / (float)pSceneView->height();

  plBoundingBox bbox;

  // clamp the bbox of the selection to ranges that won't break down due to float precision
  {
    bbox.SetCenterAndHalfExtents(pMsg->m_vCenter, pMsg->m_vHalfExtents);
    bbox.m_vMin = bbox.m_vMin.CompMax(plVec3(-1000.0f));
    bbox.m_vMax = bbox.m_vMax.CompMin(plVec3(+1000.0f));
  }

  const plVec3 vCurrentOrbitPoint = pSceneView->m_pCameraMoveContext->GetOrbitPoint();
  const bool bZoomIn = vPivotPoint.IsEqual(vCurrentOrbitPoint, 0.1f);

  if (cam.GetCameraMode() == plCameraMode::PerspectiveFixedFovX || cam.GetCameraMode() == plCameraMode::PerspectiveFixedFovY)
  {
    const float maxExt = pMsg->m_vHalfExtents.GetLength();
    const float fMinDistance = cam.GetNearPlane() * 1.1f + maxExt;

    {
      plPlane p;
      p.SetFromNormalAndPoint(vNewCameraDirection, vNewCameraPosition);

      // at some distance the floating point precision gets so crappy that the camera movement breaks
      // therefore we clamp it to a 'reasonable' distance here
      const float distBest = plMath::Min(plMath::Abs(p.GetDistanceTo(vPivotPoint)), 500.0f);

      vNewCameraPosition = vPivotPoint - vNewCameraDirection * plMath::Max(fMinDistance, distBest);
    }

    // only zoom in on the object, if the target position is already identical (action executed twice)
    if (!pMsg->m_vHalfExtents.IsZero(plMath::DefaultEpsilon<float>()) && bZoomIn)
    {
      const plAngle fovX = cam.GetFovX(fApsectRation);
      const plAngle fovY = cam.GetFovY(fApsectRation);

      const float fRadius = bbox.GetBoundingSphere().m_fRadius * 1.5f;

      const float dist1 = fRadius / plMath::Sin(fovX * 0.75);
      const float dist2 = fRadius / plMath::Sin(fovY * 0.75);
      const float distBest = plMath::Max(dist1, dist2);

      vNewCameraPosition = vPivotPoint - vNewCameraDirection * plMath::Max(fMinDistance, distBest);
    }
  }
  else
  {
    vNewCameraPosition = pMsg->m_vCenter;

    // only zoom in on the object, if the target position is already identical (action executed twice)
    if (bZoomIn)
    {

      const plVec3 right = cam.GetDirRight();
      const plVec3 up = cam.GetDirUp();

      const float fSizeFactor = 2.0f;

      const float fRequiredWidth = plMath::Abs(right.Dot(bbox.GetHalfExtents()) * 2.0f) * fSizeFactor;
      const float fRequiredHeight = plMath::Abs(up.Dot(bbox.GetHalfExtents()) * 2.0f) * fSizeFactor;

      float fDimWidth, fDimHeight;

      if (cam.GetCameraMode() == plCameraMode::OrthoFixedHeight)
      {
        fDimHeight = cam.GetFovOrDim();
        fDimWidth = fDimHeight * fApsectRation;
      }
      else
      {
        fDimWidth = cam.GetFovOrDim();
        fDimHeight = fDimWidth / fApsectRation;
      }

      const float fScaleWidth = fRequiredWidth / fDimWidth;
      const float fScaleHeight = fRequiredHeight / fDimHeight;

      const float fScaleDim = plMath::Max(fScaleWidth, fScaleHeight);

      if (fScaleDim > 0.0f)
      {
        fNewFovOrDim *= fScaleDim;
      }
    }
  }

  pSceneView->m_pCameraMoveContext->SetOrbitPoint(vPivotPoint);
  pSceneView->InterpolateCameraTo(vNewCameraPosition, vNewCameraDirection, fNewFovOrDim);
}

void plQtGameObjectDocumentWindow::SnapProviderEventHandler(const plSnapProviderEvent& e)
{
  switch (e.m_Type)
  {
    case plSnapProviderEvent::Type::RotationSnapChanged:
      ShowTemporaryStatusBarMsg(plFmt(plStringUtf8(L"Snapping Angle: {0}°").GetData(), plSnapProvider::GetRotationSnapValue().GetDegree()));
      break;

    case plSnapProviderEvent::Type::ScaleSnapChanged:
      ShowTemporaryStatusBarMsg(plFmt("Snapping Value: {0}", plSnapProvider::GetScaleSnapValue()));
      break;

    case plSnapProviderEvent::Type::TranslationSnapChanged:
      ShowTemporaryStatusBarMsg(plFmt("Snapping Value: {0}", plSnapProvider::GetTranslationSnapValue()));
      break;
  }
}
