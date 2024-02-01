#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/SphereManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plSphereManipulatorAdapter::plSphereManipulatorAdapter() = default;

plSphereManipulatorAdapter::~plSphereManipulatorAdapter() = default;

void plSphereManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  auto* pWindow = plQtDocumentWindow::FindWindowByDocument(pDoc);

  plQtEngineDocumentWindow* pEngineWindow = qobject_cast<plQtEngineDocumentWindow*>(pWindow);
  PL_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform());
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.SetOwner(pEngineWindow, nullptr);

  m_Gizmo.m_GizmoEvents.AddEventHandler(plMakeDelegate(&plSphereManipulatorAdapter::GizmoEventHandler, this));
}

void plSphereManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const plSphereManipulatorAttribute* pAttr = static_cast<const plSphereManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetInnerRadiusProperty().IsEmpty())
  {
    float fValue = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetInnerRadiusProperty()));
    m_Gizmo.SetInnerSphere(true, fValue);
  }

  if (!pAttr->GetOuterRadiusProperty().IsEmpty())
  {
    float fValue = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetOuterRadiusProperty()));
    m_Gizmo.SetOuterSphere(fValue);
  }

  m_Gizmo.SetTransformation(GetObjectTransform());
}

void plSphereManipulatorAdapter::GizmoEventHandler(const plGizmoEvent& e)
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
      const plSphereManipulatorAttribute* pAttr = static_cast<const plSphereManipulatorAttribute*>(m_pManipulatorAttr);

      ChangeProperties(pAttr->GetInnerRadiusProperty(), m_Gizmo.GetInnerRadius(), pAttr->GetOuterRadiusProperty(), m_Gizmo.GetOuterRadius());
    }
    break;
  }
}

void plSphereManipulatorAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform());
}
