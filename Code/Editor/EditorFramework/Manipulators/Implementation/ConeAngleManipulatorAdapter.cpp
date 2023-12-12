#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/ConeAngleManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plConeAngleManipulatorAdapter::plConeAngleManipulatorAdapter() {}

plConeAngleManipulatorAdapter::~plConeAngleManipulatorAdapter() {}

void plConeAngleManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  auto* pWindow = plQtDocumentWindow::FindWindowByDocument(pDoc);

  plQtEngineDocumentWindow* pEngineWindow = qobject_cast<plQtEngineDocumentWindow*>(pWindow);
  PLASMA_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform());
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.SetOwner(pEngineWindow, nullptr);

  m_Gizmo.m_GizmoEvents.AddEventHandler(plMakeDelegate(&plConeAngleManipulatorAdapter::GizmoEventHandler, this));
}

void plConeAngleManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const plConeAngleManipulatorAttribute* pAttr = static_cast<const plConeAngleManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    float fValue = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetRadiusProperty()));
  }

  m_Gizmo.SetRadius(pAttr->m_fScale);

  if (!pAttr->GetAngleProperty().IsEmpty())
  {
    plAngle value = pObjectAccessor->Get<plAngle>(m_pObject, GetProperty(pAttr->GetAngleProperty()));
    m_Gizmo.SetAngle(value);
  }

  m_Gizmo.SetTransformation(GetObjectTransform());
}

void plConeAngleManipulatorAdapter::GizmoEventHandler(const plGizmoEvent& e)
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
      const plConeAngleManipulatorAttribute* pAttr = static_cast<const plConeAngleManipulatorAttribute*>(m_pManipulatorAttr);

      ChangeProperties(pAttr->GetAngleProperty(), m_Gizmo.GetAngle());
    }
    break;
  }
}

void plConeAngleManipulatorAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform());
}
