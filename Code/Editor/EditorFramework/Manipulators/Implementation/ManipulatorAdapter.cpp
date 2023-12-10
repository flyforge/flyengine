#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plManipulatorAdapter::plManipulatorAdapter()
{
  m_pManipulatorAttr = nullptr;
  m_pObject = nullptr;
  m_bManipulatorIsVisible = true;

  plQtDocumentWindow::s_Events.AddEventHandler(plMakeDelegate(&plManipulatorAdapter::DocumentWindowEventHandler, this));
}

plManipulatorAdapter::~plManipulatorAdapter()
{
  plQtDocumentWindow::s_Events.RemoveEventHandler(plMakeDelegate(&plManipulatorAdapter::DocumentWindowEventHandler, this));

  if (m_pObject)
  {
    m_pObject->GetDocumentObjectManager()->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plManipulatorAdapter::DocumentObjectPropertyEventHandler, this));
    m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument()->m_DocumentObjectMetaData->m_DataModifiedEvent.RemoveEventHandler(plMakeDelegate(&plManipulatorAdapter::DocumentObjectMetaDataEventHandler, this));
  }
}

void plManipulatorAdapter::SetManipulator(const plManipulatorAttribute* pAttribute, const plDocumentObject* pObject)
{
  m_pManipulatorAttr = pAttribute;
  m_pObject = pObject;

  auto& meta = *m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument()->m_DocumentObjectMetaData;

  m_pObject->GetDocumentObjectManager()->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plManipulatorAdapter::DocumentObjectPropertyEventHandler, this));
  meta.m_DataModifiedEvent.AddEventHandler(plMakeDelegate(&plManipulatorAdapter::DocumentObjectMetaDataEventHandler, this));

  {
    auto pMeta = meta.BeginReadMetaData(m_pObject->GetGuid());
    m_bManipulatorIsVisible = !pMeta->m_bHidden;
    meta.EndReadMetaData();
  }

  Finalize();

  Update();
}

void plManipulatorAdapter::DocumentObjectPropertyEventHandler(const plDocumentObjectPropertyEvent& e)
{
  if (e.m_EventType == plDocumentObjectPropertyEvent::Type::PropertySet)
  {
    if (e.m_pObject == m_pObject)
    {
      if (e.m_sProperty == m_pManipulatorAttr->m_sProperty1 || e.m_sProperty == m_pManipulatorAttr->m_sProperty2 || e.m_sProperty == m_pManipulatorAttr->m_sProperty3 || e.m_sProperty == m_pManipulatorAttr->m_sProperty4 || e.m_sProperty == m_pManipulatorAttr->m_sProperty5 ||
          e.m_sProperty == m_pManipulatorAttr->m_sProperty6)
      {
        Update();
      }
    }
  }
}

void plManipulatorAdapter::DocumentWindowEventHandler(const plQtDocumentWindowEvent& e)
{
  if (e.m_Type == plQtDocumentWindowEvent::BeforeRedraw && e.m_pWindow->GetDocument() == m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument())
  {
    UpdateGizmoTransform();
  }
}

void plManipulatorAdapter::DocumentObjectMetaDataEventHandler(const plObjectMetaData<plUuid, plDocumentObjectMetaData>::EventData& e)
{
  if ((e.m_uiModifiedFlags & plDocumentObjectMetaData::HiddenFlag) != 0 && e.m_ObjectKey == m_pObject->GetGuid())
  {
    m_bManipulatorIsVisible = !e.m_pValue->m_bHidden;

    Update();
  }
}

plTransform plManipulatorAdapter::GetOffsetTransform() const
{
  return plTransform::MakeIdentity();
}

plTransform plManipulatorAdapter::GetObjectTransform() const
{
  plTransform tObj;
  m_pObject->GetDocumentObjectManager()->GetDocument()->ComputeObjectTransformation(m_pObject, tObj).IgnoreResult();

  const plTransform offset = GetOffsetTransform();

  plTransform tGlobal = plTransform::MakeGlobalTransform(tObj, offset);

  return tGlobal;
}

plObjectAccessorBase* plManipulatorAdapter::GetObjectAccessor() const
{
  return m_pObject->GetDocumentObjectManager()->GetDocument()->GetObjectAccessor();
}

const plAbstractProperty* plManipulatorAdapter::GetProperty(const char* szProperty) const
{
  return m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(szProperty);
}

void plManipulatorAdapter::BeginTemporaryInteraction()
{
  GetObjectAccessor()->BeginTemporaryCommands("Adjust Object");
}

void plManipulatorAdapter::EndTemporaryInteraction()
{
  GetObjectAccessor()->FinishTemporaryCommands();
}

void plManipulatorAdapter::CancelTemporayInteraction()
{
  GetObjectAccessor()->CancelTemporaryCommands();
}

void plManipulatorAdapter::ClampProperty(const char* szProperty, plVariant& value) const
{
  plResult status(PLASMA_FAILURE);
  const double fCur = value.ConvertTo<double>(&status);

  if (status.Failed())
    return;

  const plClampValueAttribute* pClamp = GetProperty(szProperty)->GetAttributeByType<plClampValueAttribute>();
  if (pClamp == nullptr)
    return;

  if (pClamp->GetMinValue().IsValid())
  {
    const double fMin = pClamp->GetMinValue().ConvertTo<double>(&status);
    if (status.Succeeded())
    {
      if (fCur < fMin)
        value = pClamp->GetMinValue();
    }
  }

  if (pClamp->GetMaxValue().IsValid())
  {
    const double fMax = pClamp->GetMaxValue().ConvertTo<double>(&status);
    if (status.Succeeded())
    {
      if (fCur > fMax)
        value = pClamp->GetMaxValue();
    }
  }
}

void plManipulatorAdapter::ChangeProperties(const char* szProperty1, plVariant value1, const char* szProperty2 /*= nullptr*/, plVariant value2 /*= plVariant()*/, const char* szProperty3 /*= nullptr*/, plVariant value3 /*= plVariant()*/, const char* szProperty4 /*= nullptr*/,
  plVariant value4 /*= plVariant()*/, const char* szProperty5 /*= nullptr*/, plVariant value5 /*= plVariant()*/, const char* szProperty6 /*= nullptr*/, plVariant value6 /*= plVariant()*/)
{
  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  pObjectAccessor->StartTransaction("Change Properties");

  if (!plStringUtils::IsNullOrEmpty(szProperty1))
  {
    ClampProperty(szProperty1, value1);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty1), value1).AssertSuccess();
  }

  if (!plStringUtils::IsNullOrEmpty(szProperty2))
  {
    ClampProperty(szProperty2, value2);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty2), value2).AssertSuccess();
  }

  if (!plStringUtils::IsNullOrEmpty(szProperty3))
  {
    ClampProperty(szProperty3, value3);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty3), value3).AssertSuccess();
  }

  if (!plStringUtils::IsNullOrEmpty(szProperty4))
  {
    ClampProperty(szProperty4, value4);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty4), value4).AssertSuccess();
  }

  if (!plStringUtils::IsNullOrEmpty(szProperty5))
  {
    ClampProperty(szProperty5, value5);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty5), value5).AssertSuccess();
  }

  if (!plStringUtils::IsNullOrEmpty(szProperty6))
  {
    ClampProperty(szProperty6, value6);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty6), value6).AssertSuccess();
  }

  pObjectAccessor->FinishTransaction();
}
