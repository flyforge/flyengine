#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Manipulators/NonUniformBoxManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plNonUniformBoxManipulatorAdapter::plNonUniformBoxManipulatorAdapter() = default;
plNonUniformBoxManipulatorAdapter::~plNonUniformBoxManipulatorAdapter() = default;

void plNonUniformBoxManipulatorAdapter::QueryGridSettings(plGridSettingsMsgToEngine& out_gridSettings)
{
  out_gridSettings.m_vGridCenter = m_Gizmo.GetTransformation().m_vPosition;

  // if density != 0, it is enabled at least in ortho mode
  out_gridSettings.m_fGridDensity = plSnapProvider::GetTranslationSnapValue();

  // to be active in perspective mode, tangents have to be non-zero
  out_gridSettings.m_vGridTangent1.SetZero();
  out_gridSettings.m_vGridTangent2.SetZero();
}

void plNonUniformBoxManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  auto* pWindow = plQtDocumentWindow::FindWindowByDocument(pDoc);

  plQtEngineDocumentWindow* pEngineWindow = qobject_cast<plQtEngineDocumentWindow*>(pWindow);
  PL_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform());

  m_Gizmo.SetOwner(pEngineWindow, nullptr);
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.m_GizmoEvents.AddEventHandler(plMakeDelegate(&plNonUniformBoxManipulatorAdapter::GizmoEventHandler, this));
}

void plNonUniformBoxManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const plNonUniformBoxManipulatorAttribute* pAttr = static_cast<const plNonUniformBoxManipulatorAttribute*>(m_pManipulatorAttr);

  if (pAttr->HasSixAxis())
  {
    const float fNegX = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetNegXProperty()));
    const float fPosX = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetPosXProperty()));
    const float fNegY = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetNegYProperty()));
    const float fPosY = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetPosYProperty()));
    const float fNegZ = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetNegZProperty()));
    const float fPosZ = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetPosZProperty()));

    m_Gizmo.SetSize(plVec3(fNegX, fNegY, fNegZ), plVec3(fPosX, fPosY, fPosZ));
  }
  else
  {
    const float fSizeX = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetSizeXProperty()));
    const float fSizeY = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetSizeYProperty()));
    const float fSizeZ = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetSizeZProperty()));

    m_Gizmo.SetSize(plVec3(fSizeX, fSizeY, fSizeZ) * 0.5f, plVec3(fSizeX, fSizeY, fSizeZ) * 0.5f, true);
  }

  m_Gizmo.SetTransformation(GetObjectTransform());
}

void plNonUniformBoxManipulatorAdapter::GizmoEventHandler(const plGizmoEvent& e)
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
      const plNonUniformBoxManipulatorAttribute* pAttr = static_cast<const plNonUniformBoxManipulatorAttribute*>(m_pManipulatorAttr);

      const plVec3 neg = m_Gizmo.GetNegSize();
      const plVec3 pos = m_Gizmo.GetPosSize();

      if (pAttr->HasSixAxis())
      {
        ChangeProperties(pAttr->GetNegXProperty(), neg.x, pAttr->GetPosXProperty(), pos.x, pAttr->GetNegYProperty(), neg.y, pAttr->GetPosYProperty(),
          pos.y, pAttr->GetNegZProperty(), neg.z, pAttr->GetPosZProperty(), pos.z);
      }
      else
      {
        ChangeProperties(pAttr->GetSizeXProperty(), pos.x * 2, pAttr->GetSizeYProperty(), pos.y * 2, pAttr->GetSizeZProperty(), pos.z * 2);
      }
    }
    break;
  }
}

void plNonUniformBoxManipulatorAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform());
}
