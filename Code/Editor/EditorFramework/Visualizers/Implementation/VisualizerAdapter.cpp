#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Visualizers/VisualizerAdapter.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>

plVisualizerAdapter::plVisualizerAdapter()
{
  m_pVisualizerAttr = nullptr;
  m_pObject = nullptr;
  m_bVisualizerIsVisible = true;

  plQtDocumentWindow::s_Events.AddEventHandler(plMakeDelegate(&plVisualizerAdapter::DocumentWindowEventHandler, this));
}

plVisualizerAdapter::~plVisualizerAdapter()
{
  plQtDocumentWindow::s_Events.RemoveEventHandler(plMakeDelegate(&plVisualizerAdapter::DocumentWindowEventHandler, this));

  if (m_pObject)
  {
    m_pObject->GetDocumentObjectManager()->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plVisualizerAdapter::DocumentObjectPropertyEventHandler, this));
    m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument()->m_DocumentObjectMetaData->m_DataModifiedEvent.RemoveEventHandler(plMakeDelegate(&plVisualizerAdapter::DocumentObjectMetaDataEventHandler, this));
  }
}

void plVisualizerAdapter::SetVisualizer(const plVisualizerAttribute* pAttribute, const plDocumentObject* pObject)
{
  m_pVisualizerAttr = pAttribute;
  m_pObject = pObject;

  auto& meta = *m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument()->m_DocumentObjectMetaData;

  m_pObject->GetDocumentObjectManager()->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plVisualizerAdapter::DocumentObjectPropertyEventHandler, this));
  meta.m_DataModifiedEvent.AddEventHandler(plMakeDelegate(&plVisualizerAdapter::DocumentObjectMetaDataEventHandler, this));

  {
    auto pMeta = meta.BeginReadMetaData(m_pObject->GetGuid());
    m_bVisualizerIsVisible = !pMeta->m_bHidden;
    meta.EndReadMetaData();
  }

  Finalize();

  Update();
}



void plVisualizerAdapter::DocumentObjectPropertyEventHandler(const plDocumentObjectPropertyEvent& e)
{
  if (e.m_EventType == plDocumentObjectPropertyEvent::Type::PropertySet)
  {
    if (e.m_pObject == m_pObject)
    {
      if (e.m_sProperty == m_pVisualizerAttr->m_sProperty1 || e.m_sProperty == m_pVisualizerAttr->m_sProperty2 || e.m_sProperty == m_pVisualizerAttr->m_sProperty3 || e.m_sProperty == m_pVisualizerAttr->m_sProperty4 || e.m_sProperty == m_pVisualizerAttr->m_sProperty5)
      {
        Update();
      }
    }
  }
}

void plVisualizerAdapter::DocumentWindowEventHandler(const plQtDocumentWindowEvent& e)
{
  if (e.m_Type == plQtDocumentWindowEvent::BeforeRedraw && e.m_pWindow->GetDocument() == m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument())
  {
    UpdateGizmoTransform();
  }
}

void plVisualizerAdapter::DocumentObjectMetaDataEventHandler(const plObjectMetaData<plUuid, plDocumentObjectMetaData>::EventData& e)
{
  if ((e.m_uiModifiedFlags & plDocumentObjectMetaData::HiddenFlag) != 0 && e.m_ObjectKey == m_pObject->GetGuid())
  {
    m_bVisualizerIsVisible = !e.m_pValue->m_bHidden;

    Update();
  }
}

plTransform plVisualizerAdapter::GetObjectTransform() const
{
  plTransform t;
  m_pObject->GetDocumentObjectManager()->GetDocument()->ComputeObjectTransformation(m_pObject, t).IgnoreResult();

  return t;
}

plObjectAccessorBase* plVisualizerAdapter::GetObjectAccessor() const
{
  return m_pObject->GetDocumentObjectManager()->GetDocument()->GetObjectAccessor();
}

const plAbstractProperty* plVisualizerAdapter::GetProperty(const char* szProperty) const
{
  return m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(szProperty);
}
