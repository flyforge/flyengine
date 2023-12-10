#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/BoneManipulatorAdapter.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plString plBoneManipulatorAdapter::s_sLastSelectedBone;

plBoneManipulatorAdapter::plBoneManipulatorAdapter() = default;
plBoneManipulatorAdapter::~plBoneManipulatorAdapter() = default;

void plBoneManipulatorAdapter::Finalize()
{
  RetrieveBones();
  ConfigureGizmos();
  MigrateSelection();
}

void plBoneManipulatorAdapter::MigrateSelection()
{
  for (plUInt32 i = 0; i < m_Bones.GetCount(); ++i)
  {
    if (m_Bones[i].m_sName == s_sLastSelectedBone)
    {
      m_Gizmos[i].m_RotateGizmo.SetVisible(true);
      m_Gizmos[i].m_ClickGizmo.SetVisible(false);
      return;
    }
  }

  // keep the last selection, even if it can't be migrated, until something else gets selected
}

void plBoneManipulatorAdapter::Update()
{
  RetrieveBones();
  UpdateGizmoTransform();
}

void plBoneManipulatorAdapter::RotateGizmoEventHandler(const plGizmoEvent& e)
{
  plUInt32 uiGizmo = plInvalidIndex;

  for (plUInt32 gIdx = 0; gIdx < m_Gizmos.GetCount(); ++gIdx)
  {
    if (&m_Gizmos[gIdx].m_RotateGizmo == e.m_pGizmo)
    {
      uiGizmo = gIdx;
      break;
    }
  }

  PLASMA_ASSERT_DEBUG(uiGizmo != plInvalidIndex, "Gizmo event from unknown gizmo.");
  if (uiGizmo == plInvalidIndex)
    return;

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
      plTransform globalGizmo = static_cast<const plGizmo*>(e.m_pGizmo)->GetTransformation();
      globalGizmo.m_vScale.Set(1);

      plMat4 mGizmo = globalGizmo.GetAsMat4();
      mGizmo = GetObjectTransform().GetAsMat4().GetInverse() * mGizmo;

      mGizmo = m_RootTransform.GetAsMat4().GetInverse() * mGizmo;

      mGizmo = m_Gizmos[uiGizmo].m_InverseOffset * mGizmo;

      plQuat rotOnly;
      rotOnly.ReconstructFromMat4(mGizmo);

      SetTransform(uiGizmo, plTransform(mGizmo.GetTranslationVector(), rotOnly));
    }
    break;
  }
}

void plBoneManipulatorAdapter::ClickGizmoEventHandler(const plGizmoEvent& e)
{
  plUInt32 uiGizmo = plInvalidIndex;
  s_sLastSelectedBone.Clear();

  for (plUInt32 gIdx = 0; gIdx < m_Gizmos.GetCount(); ++gIdx)
  {
    if (&m_Gizmos[gIdx].m_ClickGizmo == e.m_pGizmo)
    {
      uiGizmo = gIdx;
      s_sLastSelectedBone = m_Bones[gIdx].m_sName;
      break;
    }
  }

  PLASMA_ASSERT_DEBUG(uiGizmo != plInvalidIndex, "Gizmo event from unknown gizmo.");
  if (uiGizmo == plInvalidIndex)
    return;

  switch (e.m_Type)
  {
    case plGizmoEvent::Type::Interaction:
    {
      for (plUInt32 i = 0; i < m_Gizmos.GetCount(); ++i)
      {
        m_Gizmos[i].m_RotateGizmo.SetVisible(false);
        m_Gizmos[i].m_ClickGizmo.SetVisible(true);
      }

      m_Gizmos[uiGizmo].m_RotateGizmo.SetVisible(true);
      m_Gizmos[uiGizmo].m_ClickGizmo.SetVisible(false);
    }
    break;
    default:
      break;
  }
}

void plBoneManipulatorAdapter::RetrieveBones()
{
  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  const plBoneManipulatorAttribute* pAttr = static_cast<const plBoneManipulatorAttribute*>(m_pManipulatorAttr);

  if (pAttr->GetTransformProperty().IsEmpty())
    return;

  plVariantArray values;

  // Exposed parameters are only stored as diffs in the component. Thus, requesting the exposed parameters only returns those that have been modified. To get all, you need to use the plExposedParameterCommandAccessor which gives you all exposed parameters from the source asset.
  auto pProperty = GetProperty(pAttr->GetTransformProperty());
  if (const plExposedParametersAttribute* pAttrib = pProperty->GetAttributeByType<plExposedParametersAttribute>())
  {
    const plAbstractProperty* pParameterSourceProp = m_pObject->GetType()->FindPropertyByName(pAttrib->GetParametersSource());
    PLASMA_ASSERT_DEV(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", pAttrib->GetParametersSource(), m_pObject->GetType()->GetTypeName());

    plExposedParameterCommandAccessor proxy(pObjectAccessor, pProperty, pParameterSourceProp);
    proxy.GetValues(m_pObject, pProperty, values).AssertSuccess();
    proxy.GetKeys(m_pObject, pProperty, m_Keys).AssertSuccess();
  }
  else
  {
    pObjectAccessor->GetKeys(m_pObject, pProperty, m_Keys).AssertSuccess();
    pObjectAccessor->GetValues(m_pObject, pProperty, values).AssertSuccess();
  }

  m_RootTransform.SetIdentity();

  m_Bones.Clear();
  m_Bones.SetCount(values.GetCount());

  for (plUInt32 i = 0; i < m_Bones.GetCount(); ++i)
  {
    if (values[i].GetReflectedType() == plGetStaticRTTI<plExposedBone>())
    {
      const plExposedBone* pBone = reinterpret_cast<const plExposedBone*>(values[i].GetData());

      if (pBone->m_sName == "<root-transform>")
      {
        m_RootTransform = pBone->m_Transform;
      }

      m_Bones[i] = *pBone;
    }
    else
    {
      //PLASMA_REPORT_FAILURE("Property is not an plExposedBone");
      m_Bones.Clear();
      return;
    }
  }
}

void plBoneManipulatorAdapter::UpdateGizmoTransform()
{
  const plMat4 ownerTransform = GetObjectTransform().GetAsMat4();

  for (plUInt32 i = 0; i < m_Gizmos.GetCount(); ++i)
  {
    auto& gizmo = m_Gizmos[i];

    gizmo.m_Offset = ComputeParentTransform(i);
    gizmo.m_InverseOffset = gizmo.m_Offset.GetInverse();

    plMat4 mGizmo = ownerTransform * m_RootTransform.GetAsMat4() * gizmo.m_Offset * m_Bones[i].m_Transform.GetAsMat4();

    plQuat rotOnly;
    rotOnly.ReconstructFromMat4(mGizmo);

    plTransform tGizmo;
    tGizmo.m_vPosition = mGizmo.GetTranslationVector();
    tGizmo.m_qRotation = rotOnly;

    tGizmo.m_vScale.Set(0.5f);
    gizmo.m_RotateGizmo.SetTransformation(tGizmo);

    tGizmo.m_vScale.Set(0.02f);
    gizmo.m_ClickGizmo.SetTransformation(tGizmo);
  }
}

void plBoneManipulatorAdapter::ConfigureGizmos()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  auto* pWindow = plQtDocumentWindow::FindWindowByDocument(pDoc);
  plQtEngineDocumentWindow* pEngineWindow = qobject_cast<plQtEngineDocumentWindow*>(pWindow);
  PLASMA_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmos.SetCount(m_Bones.GetCount());

  for (plUInt32 i = 0; i < m_Gizmos.GetCount(); ++i)
  {
    auto& gizmo = m_Gizmos[i];

    gizmo.m_Offset = ComputeParentTransform(i);

    auto& rot = gizmo.m_RotateGizmo;
    rot.SetOwner(pEngineWindow, nullptr);
    rot.SetVisible(false);
    rot.m_GizmoEvents.AddEventHandler(plMakeDelegate(&plBoneManipulatorAdapter::RotateGizmoEventHandler, this));

    auto& click = gizmo.m_ClickGizmo;
    click.SetOwner(pEngineWindow, nullptr);
    click.SetVisible(true);
    click.SetColor(plColor::Thistle);
    click.m_GizmoEvents.AddEventHandler(plMakeDelegate(&plBoneManipulatorAdapter::ClickGizmoEventHandler, this));
  }

  UpdateGizmoTransform();
}

void plBoneManipulatorAdapter::SetTransform(plUInt32 uiBone, const plTransform& value)
{
  plExposedBone* pBone = &m_Bones[uiBone];

  pBone->m_Transform.m_qRotation = value.m_qRotation;
  pBone->m_Transform = value;

  plExposedBone bone;
  bone.m_sName = pBone->m_sName;
  bone.m_sParent = pBone->m_sParent;
  bone.m_Transform = value;

  plVariant var;
  var.CopyTypedObject(&bone, plGetStaticRTTI<plExposedBone>());

  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  const plBoneManipulatorAttribute* pAttr = static_cast<const plBoneManipulatorAttribute*>(m_pManipulatorAttr);

  if (pAttr->GetTransformProperty().IsEmpty())
    return;

  auto pProperty = GetProperty(pAttr->GetTransformProperty());
  const plExposedParametersAttribute* pAttrib = pProperty->GetAttributeByType<plExposedParametersAttribute>();

  const plAbstractProperty* pParameterSourceProp = m_pObject->GetType()->FindPropertyByName(pAttrib->GetParametersSource());
  PLASMA_ASSERT_DEV(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", pAttrib->GetParametersSource(), m_pObject->GetType()->GetTypeName());

  plExposedParameterCommandAccessor proxy(pObjectAccessor, pProperty, pParameterSourceProp);

  // for some reason the first command in plExposedParameterCommandAccessor returns failure 'the property X does not exist' and the insert
  // command than fails with 'the property X already exists' ???

  proxy.SetValue(m_pObject, pProperty, var, m_Keys[uiBone]).AssertSuccess();
}

plMat4 plBoneManipulatorAdapter::ComputeFullTransform(plUInt32 uiBone) const
{
  const plMat4 tParent = ComputeParentTransform(uiBone);

  return tParent * m_Bones[uiBone].m_Transform.GetAsMat4();
}

plMat4 plBoneManipulatorAdapter::ComputeParentTransform(plUInt32 uiBone) const
{
  const plString& parent = m_Bones[uiBone].m_sParent;

  if (!parent.IsEmpty())
  {
    for (plUInt32 b = 0; b < m_Bones.GetCount(); ++b)
    {
      if (m_Bones[b].m_sName == parent)
      {
        return ComputeFullTransform(b);
      }
    }
  }

  return plMat4::MakeIdentity();
}
