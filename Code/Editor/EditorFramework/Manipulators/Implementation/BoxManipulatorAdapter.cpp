#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Manipulators/BoxManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plBoxManipulatorAdapter::plBoxManipulatorAdapter() = default;
plBoxManipulatorAdapter::~plBoxManipulatorAdapter() = default;

void plBoxManipulatorAdapter::QueryGridSettings(plGridSettingsMsgToEngine& out_gridSettings)
{
  out_gridSettings.m_vGridCenter = m_Gizmo.GetTransformation().m_vPosition;

  // if density != 0, it is enabled at least in ortho mode
  out_gridSettings.m_fGridDensity = plSnapProvider::GetTranslationSnapValue();

  // to be active in perspective mode, tangents have to be non-zero
  out_gridSettings.m_vGridTangent1.SetZero();
  out_gridSettings.m_vGridTangent2.SetZero();
}

void plBoxManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  auto* pWindow = plQtDocumentWindow::FindWindowByDocument(pDoc);

  plQtEngineDocumentWindow* pEngineWindow = qobject_cast<plQtEngineDocumentWindow*>(pWindow);
  PLASMA_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform());

  m_Gizmo.SetOwner(pEngineWindow, nullptr);
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.m_GizmoEvents.AddEventHandler(plMakeDelegate(&plBoxManipulatorAdapter::GizmoEventHandler, this));
}

void plBoxManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const plBoxManipulatorAttribute* pAttr = static_cast<const plBoxManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetSizeProperty().IsEmpty())
  {
    plVec3 vSize = pObjectAccessor->Get<plVec3>(m_pObject, GetProperty(pAttr->GetSizeProperty()));
    vSize *= pAttr->m_fSizeScale;
    vSize *= 0.5f;

    m_vOldSize = vSize;

    m_Gizmo.SetSize(vSize, vSize, false);
  }

  m_vPositionOffset.SetZero();

  if (!pAttr->GetOffsetProperty().IsEmpty())
  {
    m_vPositionOffset = pObjectAccessor->Get<plVec3>(m_pObject, GetProperty(pAttr->GetOffsetProperty()));
  }

  m_qRotation.SetIdentity();

  if (!pAttr->GetRotationProperty().IsEmpty())
  {
    m_qRotation = pObjectAccessor->Get<plQuat>(m_pObject, GetProperty(pAttr->GetRotationProperty()));
  }

  UpdateGizmoTransform();
}

void plBoxManipulatorAdapter::GizmoEventHandler(const plGizmoEvent& e)
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
      const plBoxManipulatorAttribute* pAttr = static_cast<const plBoxManipulatorAttribute*>(m_pManipulatorAttr);

      const char* szSizeProperty = pAttr->GetSizeProperty();
      const plVec3 vNewSizeNeg = m_Gizmo.GetNegSize();
      const plVec3 vNewSizePos = m_Gizmo.GetPosSize();
      const plVec3 vNewSize = (vNewSizeNeg + vNewSizePos) / pAttr->m_fSizeScale;

      plVariant oldSize;

      plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

      pObjectAccessor->GetValue(m_pObject, GetProperty(szSizeProperty), oldSize).AssertSuccess();

      plVariant newValue = vNewSize;

      pObjectAccessor->StartTransaction("Change Properties");

      if (!plStringUtils::IsNullOrEmpty(szSizeProperty))
      {
        pObjectAccessor->SetValue(m_pObject, GetProperty(szSizeProperty), newValue).AssertSuccess();
      }

      if (pAttr->m_bRecenterParent)
      {
        const plDocumentObject* pParent = m_pObject->GetParent();

        if (const plGameObjectDocument* pGameDoc = plDynamicCast<const plGameObjectDocument*>(pParent->GetDocumentObjectManager()->GetDocument()))
        {
          plTransform tParent = pGameDoc->GetGlobalTransform(pParent);

          if (m_vOldSize.x != vNewSizeNeg.x)
            tParent.m_vPosition -= tParent.m_qRotation * plVec3((vNewSizeNeg.x - m_vOldSize.x) * 0.5f, 0, 0);
          if (m_vOldSize.x != vNewSizePos.x)
            tParent.m_vPosition += tParent.m_qRotation * plVec3((vNewSizePos.x - m_vOldSize.x) * 0.5f, 0, 0);

          if (m_vOldSize.y != vNewSizeNeg.y)
            tParent.m_vPosition -= tParent.m_qRotation * plVec3(0, (vNewSizeNeg.y - m_vOldSize.y) * 0.5f, 0);
          if (m_vOldSize.y != vNewSizePos.y)
            tParent.m_vPosition += tParent.m_qRotation * plVec3(0, (vNewSizePos.y - m_vOldSize.y) * 0.5f, 0);

          if (m_vOldSize.z != vNewSizeNeg.z)
            tParent.m_vPosition -= tParent.m_qRotation * plVec3(0, 0, (vNewSizeNeg.z - m_vOldSize.z) * 0.5f);
          if (m_vOldSize.z != vNewSizePos.z)
            tParent.m_vPosition += tParent.m_qRotation * plVec3(0, 0, (vNewSizePos.z - m_vOldSize.z) * 0.5f);

          pGameDoc->SetGlobalTransform(pParent, tParent, TransformationChanges::Translation);
        }
      }

      pObjectAccessor->FinishTransaction();
    }

    break;
  }
}

void plBoxManipulatorAdapter::UpdateGizmoTransform()
{
  plTransform t;
  t.m_vScale.Set(1);
  t.m_vPosition = m_vPositionOffset;
  t.m_qRotation = m_qRotation;

  m_Gizmo.SetTransformation(GetObjectTransform() * t);
}
