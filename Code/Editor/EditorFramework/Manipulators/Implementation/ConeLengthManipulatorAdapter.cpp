#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/ConeLengthManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plConeLengthManipulatorAdapter::plConeLengthManipulatorAdapter() = default;

plConeLengthManipulatorAdapter::~plConeLengthManipulatorAdapter() = default;

void plConeLengthManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  auto* pWindow = plQtDocumentWindow::FindWindowByDocument(pDoc);

  plQtEngineDocumentWindow* pEngineWindow = qobject_cast<plQtEngineDocumentWindow*>(pWindow);
  PL_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform());
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.SetOwner(pEngineWindow, nullptr);

  m_Gizmo.m_GizmoEvents.AddEventHandler(plMakeDelegate(&plConeLengthManipulatorAdapter::GizmoEventHandler, this));
}

void plConeLengthManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const plConeLengthManipulatorAttribute* pAttr = static_cast<const plConeLengthManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    float fValue = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetRadiusProperty()));
    m_Gizmo.SetRadius(fValue);
  }

  m_Gizmo.SetTransformation(GetObjectTransform());
}

void plConeLengthManipulatorAdapter::GizmoEventHandler(const plGizmoEvent& e)
{
  switch (e.m_Type)
  {
    case plGizmoEvent::Type::BeginInteractions:
      BeginTemporaryInteraction();
      break;

    case plGizmoEvent::Type::CancelInteractions:
      CancelTemporayInteraction();
      break;

    case plGizmoEvent::Type::EndInteractions:
      EndTemporaryInteraction();
      break;

    case plGizmoEvent::Type::Interaction:
    {
      const plConeLengthManipulatorAttribute* pAttr = static_cast<const plConeLengthManipulatorAttribute*>(m_pManipulatorAttr);

      ChangeProperties(pAttr->GetRadiusProperty(), m_Gizmo.GetRadius());
    }
    break;
  }
}

void plConeLengthManipulatorAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform());
}
