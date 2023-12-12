#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/TransformManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plTransformManipulatorAdapter::plTransformManipulatorAdapter() {}

plTransformManipulatorAdapter::~plTransformManipulatorAdapter() {}

void plTransformManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  auto* pWindow = plQtDocumentWindow::FindWindowByDocument(pDoc);

  plQtEngineDocumentWindow* pEngineWindow = qobject_cast<plQtEngineDocumentWindow*>(pWindow);
  PLASMA_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_TranslateGizmo.SetTransformation(GetObjectTransform());
  m_RotateGizmo.SetTransformation(GetObjectTransform());
  m_ScaleGizmo.SetTransformation(GetObjectTransform());

  const plTransformManipulatorAttribute* pAttr = static_cast<const plTransformManipulatorAttribute*>(m_pManipulatorAttr);
  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  if (!pAttr->GetTranslateProperty().IsEmpty())
  {
    m_bHideTranslate = GetProperty(pAttr->GetTranslateProperty())->GetFlags().IsSet(plPropertyFlags::ReadOnly);
  }

  if (!pAttr->GetRotateProperty().IsEmpty())
  {
    m_bHideRotate = GetProperty(pAttr->GetRotateProperty())->GetFlags().IsSet(plPropertyFlags::ReadOnly);
  }

  if (!pAttr->GetScaleProperty().IsEmpty())
  {
    m_bHideScale = GetProperty(pAttr->GetScaleProperty())->GetFlags().IsSet(plPropertyFlags::ReadOnly);
  }

  m_TranslateGizmo.SetOwner(pEngineWindow, nullptr);
  m_TranslateGizmo.SetVisible(m_bManipulatorIsVisible && !m_bHideTranslate);
  m_RotateGizmo.SetOwner(pEngineWindow, nullptr);
  m_RotateGizmo.SetVisible(m_bManipulatorIsVisible && !m_bHideRotate);
  m_ScaleGizmo.SetOwner(pEngineWindow, nullptr);
  m_ScaleGizmo.SetVisible(m_bManipulatorIsVisible && !m_bHideScale);

  m_TranslateGizmo.m_GizmoEvents.AddEventHandler(plMakeDelegate(&plTransformManipulatorAdapter::GizmoEventHandler, this));
  m_RotateGizmo.m_GizmoEvents.AddEventHandler(plMakeDelegate(&plTransformManipulatorAdapter::GizmoEventHandler, this));
  m_ScaleGizmo.m_GizmoEvents.AddEventHandler(plMakeDelegate(&plTransformManipulatorAdapter::GizmoEventHandler, this));
}

void plTransformManipulatorAdapter::Update()
{
  UpdateGizmoTransform();
}

void plTransformManipulatorAdapter::GizmoEventHandler(const plGizmoEvent& e)
{
  const plTransformManipulatorAttribute* pAttr = static_cast<const plTransformManipulatorAttribute*>(m_pManipulatorAttr);
  plObjectAccessorBase* pAccessor = GetObjectAccessor();
  switch (e.m_Type)
  {
    case plGizmoEvent::Type::BeginInteractions:
      m_vOldScale = GetScale();
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
      if (e.m_pGizmo == &m_TranslateGizmo || e.m_pGizmo == &m_RotateGizmo || e.m_pGizmo == &m_ScaleGizmo)
      {
        const plTransform tParent = GetObjectTransform();
        const plTransform tGlobal = static_cast<const plGizmo*>(e.m_pGizmo)->GetTransformation();
        plTransform tLocal;
        tLocal.SetLocalTransform(tParent, tGlobal);
        if (e.m_pGizmo == &m_TranslateGizmo)
        {
          ChangeProperties(pAttr->GetTranslateProperty(), tLocal.m_vPosition);
        }
        else if (e.m_pGizmo == &m_RotateGizmo)
        {
          ChangeProperties(pAttr->GetRotateProperty(), tLocal.m_qRotation);
        }
        else if (e.m_pGizmo == &m_ScaleGizmo)
        {
          plVec3 vNewScale = m_vOldScale.CompMul(m_ScaleGizmo.GetScalingResult());
          ChangeProperties(pAttr->GetScaleProperty(), vNewScale);
        }
      }
    }
    break;
  }
}


void plTransformManipulatorAdapter::UpdateGizmoTransform()
{
  const plTransformManipulatorAttribute* pAttr = static_cast<const plTransformManipulatorAttribute*>(m_pManipulatorAttr);

  m_TranslateGizmo.SetVisible(m_bManipulatorIsVisible && !m_bHideTranslate);
  m_RotateGizmo.SetVisible(m_bManipulatorIsVisible && !m_bHideRotate);
  m_ScaleGizmo.SetVisible(m_bManipulatorIsVisible && !m_bHideScale);

  const plVec3 vPos = GetTranslation();
  const plQuat vRot = GetRotation();
  const plVec3 vScale = GetScale();

  const plTransform tParent = GetObjectTransform();
  plTransform tLocal;
  tLocal.m_vPosition = vPos;
  tLocal.m_qRotation = vRot;
  tLocal.m_vScale = vScale;
  plTransform tGlobal;
  tGlobal.SetGlobalTransform(tParent, tLocal);
  // Let's not apply scaling to the gizmos.
  tGlobal.m_vScale = plVec3(1, 1, 1);

  m_TranslateGizmo.SetTransformation(tGlobal);
  m_RotateGizmo.SetTransformation(tGlobal);
  m_ScaleGizmo.SetTransformation(tGlobal);
}

plVec3 plTransformManipulatorAdapter::GetTranslation()
{
  const plTransformManipulatorAttribute* pAttr = static_cast<const plTransformManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetTranslateProperty().IsEmpty())
  {
    plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
    return pObjectAccessor->Get<plVec3>(m_pObject, GetProperty(pAttr->GetTranslateProperty()));
  }

  return plVec3(0);
}

plQuat plTransformManipulatorAdapter::GetRotation()
{
  const plTransformManipulatorAttribute* pAttr = static_cast<const plTransformManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetRotateProperty().IsEmpty())
  {
    plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
    return pObjectAccessor->Get<plQuat>(m_pObject, GetProperty(pAttr->GetRotateProperty()));
  }

  return plQuat::IdentityQuaternion();
}

plVec3 plTransformManipulatorAdapter::GetScale()
{
  const plTransformManipulatorAttribute* pAttr = static_cast<const plTransformManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetScaleProperty().IsEmpty())
  {
    plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
    return pObjectAccessor->Get<plVec3>(m_pObject, GetProperty(pAttr->GetScaleProperty()));
  }

  return plVec3(1);
}

plTransform plTransformManipulatorAdapter::GetOffsetTransform() const
{
  plTransform offset;
  offset.SetIdentity();

  if (const plTransformManipulatorAttribute* pAttr = plDynamicCast<const plTransformManipulatorAttribute*>(m_pManipulatorAttr))
  {
    if (!pAttr->GetGetOffsetTranslationProperty().IsEmpty())
    {
      plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
      offset.m_vPosition = pObjectAccessor->Get<plVec3>(m_pObject, GetProperty(pAttr->GetGetOffsetTranslationProperty()));
    }

    if (!pAttr->GetGetOffsetRotationProperty().IsEmpty())
    {
      plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
      offset.m_qRotation = pObjectAccessor->Get<plQuat>(m_pObject, GetProperty(pAttr->GetGetOffsetRotationProperty()));
    }
  }

  return offset;
}
