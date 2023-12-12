#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/EditTools/GizmoEditTool.h>
#include <EditorFramework/InputContexts/OrthoGizmoContext.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGameObjectGizmoEditTool, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plGameObjectGizmoEditTool::plGameObjectGizmoEditTool()
{
  plQtDocumentWindow::s_Events.AddEventHandler(plMakeDelegate(&plGameObjectGizmoEditTool::DocumentWindowEventHandler, this));
}

plGameObjectGizmoEditTool::~plGameObjectGizmoEditTool()
{
  plQtDocumentWindow::s_Events.RemoveEventHandler(plMakeDelegate(&plGameObjectGizmoEditTool::DocumentWindowEventHandler, this));
}

void plGameObjectGizmoEditTool::OnConfigured()
{
  GetDocument()->m_GameObjectEvents.AddEventHandler(plMakeDelegate(&plGameObjectGizmoEditTool::GameObjectEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(plMakeDelegate(&plGameObjectGizmoEditTool::CommandHistoryEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(plMakeDelegate(&plGameObjectGizmoEditTool::SelectionManagerEventHandler, this));
  plManipulatorManager::GetSingleton()->m_Events.AddEventHandler(plMakeDelegate(&plGameObjectGizmoEditTool::ManipulatorManagerEventHandler, this));
  GetWindow()->m_EngineWindowEvent.AddEventHandler(plMakeDelegate(&plGameObjectGizmoEditTool::EngineWindowEventHandler, this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(plMakeDelegate(&plGameObjectGizmoEditTool::ObjectStructureEventHandler, this));

  // subscribe to all views that already exist
  for (plQtEngineViewWidget* pView : GetWindow()->GetViewWidgets())
  {
    if (plQtGameObjectViewWidget* pViewWidget = qobject_cast<plQtGameObjectViewWidget*>(pView))
    {
      pViewWidget->m_pOrthoGizmoContext->m_GizmoEvents.AddEventHandler(
        plMakeDelegate(&plGameObjectGizmoEditTool::TransformationGizmoEventHandler, this));
    }
  }
}

void plGameObjectGizmoEditTool::UpdateGizmoSelectionList()
{
  GetDocument()->ComputeTopLevelSelectedGameObjects(m_GizmoSelection);
}

void plGameObjectGizmoEditTool::UpdateGizmoVisibleState()
{
  bool isVisible = false;

  if (IsActive())
  {
    plGameObjectDocument* pDocument = GetDocument();

    const auto& selection = pDocument->GetSelectionManager()->GetSelection();

    if (selection.IsEmpty() || !selection.PeekBack()->GetTypeAccessor().GetType()->IsDerivedFrom<plGameObject>())
      goto done;

    isVisible = true;
    UpdateGizmoTransformation();
  }

done:
  ApplyGizmoVisibleState(isVisible);
}

void plGameObjectGizmoEditTool::UpdateGizmoTransformation()
{
  const auto& LatestSelection = GetDocument()->GetSelectionManager()->GetSelection().PeekBack();

  if (LatestSelection->GetTypeAccessor().GetType() == plGetStaticRTTI<plGameObject>())
  {
    const plTransform tGlobal = GetDocument()->GetGlobalTransform(LatestSelection);

    /// \todo Pivot point
    const plVec3 vPivotPoint =
      tGlobal.m_qRotation * plVec3::ZeroVector(); // LatestSelection->GetEditorTypeAccessor().GetValue("Pivot").ConvertTo<plVec3>();

    plTransform mt;
    mt.SetIdentity();

    if (GetDocument()->GetGizmoWorldSpace() && GetSupportedSpaces() != plEditToolSupportedSpaces::LocalSpaceOnly)
    {
      mt.m_vPosition = tGlobal.m_vPosition + vPivotPoint;
    }
    else
    {
      mt.m_qRotation = tGlobal.m_qRotation;
      mt.m_vPosition = tGlobal.m_vPosition + vPivotPoint;
    }

    ApplyGizmoTransformation(mt);
  }
}

void plGameObjectGizmoEditTool::DocumentWindowEventHandler(const plQtDocumentWindowEvent& e)
{
  if (e.m_Type == plQtDocumentWindowEvent::WindowClosing && e.m_pWindow == GetWindow())
  {
    GetDocument()->m_GameObjectEvents.RemoveEventHandler(plMakeDelegate(&plGameObjectGizmoEditTool::GameObjectEventHandler, this));
    GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(plMakeDelegate(&plGameObjectGizmoEditTool::CommandHistoryEventHandler, this));
    GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(plMakeDelegate(&plGameObjectGizmoEditTool::SelectionManagerEventHandler, this));
    plManipulatorManager::GetSingleton()->m_Events.RemoveEventHandler(
      plMakeDelegate(&plGameObjectGizmoEditTool::ManipulatorManagerEventHandler, this));
    GetWindow()->m_EngineWindowEvent.RemoveEventHandler(plMakeDelegate(&plGameObjectGizmoEditTool::EngineWindowEventHandler, this));
    GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(
      plMakeDelegate(&plGameObjectGizmoEditTool::ObjectStructureEventHandler, this));
  }
}

void plGameObjectGizmoEditTool::UpdateManipulatorVisibility()
{
  plManipulatorManager::GetSingleton()->HideActiveManipulator(GetDocument(), GetDocument()->GetActiveEditTool() != nullptr);
}

void plGameObjectGizmoEditTool::GameObjectEventHandler(const plGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case plGameObjectEvent::Type::ActiveEditToolChanged:
    case plGameObjectEvent::Type::GizmoTransformMayBeInvalid:
      UpdateGizmoVisibleState();
      UpdateManipulatorVisibility();
      break;

    default:
      break;
  }
}

void plGameObjectGizmoEditTool::CommandHistoryEventHandler(const plCommandHistoryEvent& e)
{
  switch (e.m_Type)
  {
    case plCommandHistoryEvent::Type::UndoEnded:
    case plCommandHistoryEvent::Type::RedoEnded:
    case plCommandHistoryEvent::Type::TransactionEnded:
    case plCommandHistoryEvent::Type::TransactionCanceled:
      UpdateGizmoVisibleState();
      break;

    default:
      break;
  }
}

void plGameObjectGizmoEditTool::SelectionManagerEventHandler(const plSelectionManagerEvent& e)
{
  switch (e.m_Type)
  {
    case plSelectionManagerEvent::Type::SelectionCleared:
      m_GizmoSelection.Clear();
      UpdateGizmoVisibleState();
      break;

    case plSelectionManagerEvent::Type::SelectionSet:
    case plSelectionManagerEvent::Type::ObjectAdded:
      PLASMA_ASSERT_DEBUG(m_GizmoSelection.IsEmpty(), "This array should have been cleared when the gizmo lost focus");
      UpdateGizmoVisibleState();
      break;

    default:
      break;
  }
}

void plGameObjectGizmoEditTool::ManipulatorManagerEventHandler(const plManipulatorManagerEvent& e)
{
  if (!IsActive())
    return;

  // make sure the gizmo is deactivated when a manipulator becomes active
  if (e.m_pDocument == GetDocument() && e.m_pManipulator != nullptr && e.m_pSelection != nullptr && !e.m_pSelection->IsEmpty() &&
      !e.m_bHideManipulators)
  {
    GetDocument()->SetActiveEditTool(nullptr);
  }
}

void plGameObjectGizmoEditTool::EngineWindowEventHandler(const PlasmaEngineWindowEvent& e)
{
  if (plQtGameObjectViewWidget* pViewWidget = qobject_cast<plQtGameObjectViewWidget*>(e.m_pView))
  {
    switch (e.m_Type)
    {
      case PlasmaEngineWindowEvent::Type::ViewCreated:
        pViewWidget->m_pOrthoGizmoContext->m_GizmoEvents.AddEventHandler(
          plMakeDelegate(&plGameObjectGizmoEditTool::TransformationGizmoEventHandler, this));
        break;

      default:
        break;
    }
  }
}

void plGameObjectGizmoEditTool::ObjectStructureEventHandler(const plDocumentObjectStructureEvent& e)
{
  if (!IsActive() || m_bInGizmoInteraction)
    return;

  switch (e.m_EventType)
  {
    case plDocumentObjectStructureEvent::Type::AfterObjectRemoved:
      UpdateGizmoVisibleState();
      break;

    default:
      break;
  }
}

void plGameObjectGizmoEditTool::TransformationGizmoEventHandler(const plGizmoEvent& e)
{
  if (!IsActive())
    return;

  plObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();

  switch (e.m_Type)
  {
    case plGizmoEvent::Type::BeginInteractions:
    {
      m_bMergeTransactions = false;

      TransformationGizmoEventHandlerImpl(e);

      UpdateGizmoSelectionList();

      pAccessor->BeginTemporaryCommands("Transform Object");
    }
    break;

    case plGizmoEvent::Type::Interaction:
    {
      m_bInGizmoInteraction = true;
      pAccessor->StartTransaction("Transform Object");

      TransformationGizmoEventHandlerImpl(e);

      m_bInGizmoInteraction = false;
    }
    break;

    case plGizmoEvent::Type::EndInteractions:
    {
      pAccessor->FinishTemporaryCommands();
      m_GizmoSelection.Clear();

      if (m_bMergeTransactions)
        GetDocument()->GetCommandHistory()->MergeLastTwoTransactions(); //#TODO: this should be interleaved transactions
    }
    break;

    case plGizmoEvent::Type::CancelInteractions:
    {
      pAccessor->CancelTemporaryCommands();
      m_GizmoSelection.Clear();
    }
    break;

    default:
      break;
  }
}
