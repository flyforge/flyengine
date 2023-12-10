#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/PreferencesDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

class plPreferencesObjectManager : public plDocumentObjectManager
{
public:
  virtual void GetCreateableTypes(plHybridArray<const plRTTI*, 32>& ref_types) const override
  {
    for (auto pRtti : m_KnownTypes)
    {
      ref_types.PushBack(pRtti);
    }
  }

  plHybridArray<const plRTTI*, 16> m_KnownTypes;
};


class plPreferencesDocument : public plDocument
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPreferencesDocument, plDocument);


public:
  plPreferencesDocument(plStringView sDocumentPath)
    : plDocument(sDocumentPath, PLASMA_DEFAULT_NEW(plPreferencesObjectManager))
  {
  }

public:
  virtual plDocumentInfo* CreateDocumentInfo() override { return PLASMA_DEFAULT_NEW(plDocumentInfo); }
};

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plPreferencesDocument, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plQtPreferencesDlg::plQtPreferencesDlg(QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);

  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  m_pDocument = PLASMA_DEFAULT_NEW(plPreferencesDocument, "<none>");

  // if this is set, all properties are applied immediately
  // m_pDocument->GetObjectManager()->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plQtPreferencesDlg::PropertyChangedEventHandler,
  // this));
  std::unique_ptr<plQtDocumentTreeModel> pModel(new plQtDocumentTreeModel(m_pDocument->GetObjectManager()));
  pModel->AddAdapter(new plQtDummyAdapter(m_pDocument->GetObjectManager(), plGetStaticRTTI<plDocumentRoot>(), "Children"));
  pModel->AddAdapter(new plQtNamedAdapter(m_pDocument->GetObjectManager(), plPreferences::GetStaticRTTI(), "", "Name"));

  Tree->Initialize(m_pDocument, std::move(pModel));
  Tree->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  Tree->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

  RegisterAllPreferenceTypes();
  AllPreferencesToObject();

  Properties->SetDocument(m_pDocument);

  m_pDocument->GetSelectionManager()->SetSelection(m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
}

plQtPreferencesDlg::~plQtPreferencesDlg()
{
  delete Tree;
  Tree = nullptr;

  delete Properties;
  Properties = nullptr;

  PLASMA_DEFAULT_DELETE(m_pDocument);
}

plUuid plQtPreferencesDlg::NativeToObject(plPreferences* pPreferences)
{
  const plRTTI* pType = pPreferences->GetDynamicRTTI();
  // Write properties to graph.
  plAbstractObjectGraph graph;
  plRttiConverterContext context;
  plRttiConverterWriter conv(&graph, &context, true, true);

  const plUuid guid = plUuid::MakeUuid();
  context.RegisterObject(guid, pType, pPreferences);
  plAbstractObjectNode* pNode = conv.AddObjectToGraph(pType, pPreferences, "root");

  // Read from graph and write into matching document object.
  auto pRoot = m_pDocument->GetObjectManager()->GetRootObject();
  plDocumentObject* pObject = m_pDocument->GetObjectManager()->CreateObject(pType);
  m_pDocument->GetObjectManager()->AddObject(pObject, pRoot, "Children", -1);

  plDocumentObjectConverterReader objectConverter(
    &graph, m_pDocument->GetObjectManager(), plDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
  objectConverter.ApplyPropertiesToObject(pNode, pObject);

  return pObject->GetGuid();
}

void plQtPreferencesDlg::ObjectToNative(plUuid objectGuid, const plDocument* pPrefDocument)
{
  plDocumentObject* pObject = m_pDocument->GetObjectManager()->GetObject(objectGuid);
  const plRTTI* pType = pObject->GetTypeAccessor().GetType();

  // Write object to graph.
  plAbstractObjectGraph graph;
  auto filter = [](const plDocumentObject*, const plAbstractProperty* pProp) -> bool
  {
    if (pProp->GetFlags().IsSet(plPropertyFlags::ReadOnly))
      return false;
    return true;
  };
  plDocumentObjectConverterWriter objectConverter(&graph, m_pDocument->GetObjectManager(), filter);
  plAbstractObjectNode* pNode = objectConverter.AddObjectToGraph(pObject, "root");

  // Read from graph and write to native object.
  plRttiConverterContext context;
  plRttiConverterReader conv(&graph, &context);

  plPreferences* pPreferences = plPreferences::QueryPreferences(pType, pPrefDocument);
  conv.ApplyPropertiesToObject(pNode, pType, pPreferences);

  pPreferences->TriggerPreferencesChangedEvent();
}


void plQtPreferencesDlg::on_ButtonOk_clicked()
{
  ApplyAllChanges();
  accept();
}


void plQtPreferencesDlg::RegisterAllPreferenceTypes()
{
  plPreferencesObjectManager* pManager = static_cast<plPreferencesObjectManager*>(m_pDocument->GetObjectManager());

  plHybridArray<plPreferences*, 16> AllPrefs;
  plPreferences::GatherAllPreferences(AllPrefs);

  for (auto pref : AllPrefs)
  {
    pManager->m_KnownTypes.PushBack(pref->GetDynamicRTTI());
  }
}


void plQtPreferencesDlg::AllPreferencesToObject()
{
  plHybridArray<plPreferences*, 16> AllPrefs;
  plPreferences::GatherAllPreferences(AllPrefs);

  plHybridArray<const plAbstractProperty*, 32> properties;

  plMap<plString, plPreferences*> appPref;
  plMap<plString, plPreferences*> projPref;
  plMap<plString, plPreferences*> docPref;

  for (auto pref : AllPrefs)
  {
    bool noVisibleProperties = true;

    // ignore all objects that have no visible properties
    pref->GetDynamicRTTI()->GetAllProperties(properties);
    for (const plAbstractProperty* prop : properties)
    {
      if (prop->GetAttributeByType<plHiddenAttribute>() != nullptr)
        continue;

      noVisibleProperties = false;
      break;
    }

    if (noVisibleProperties)
      continue;

    switch (pref->GetDomain())
    {
      case plPreferences::Domain::Application:
        appPref[pref->GetName()] = pref;
        break;
      case plPreferences::Domain::Project:
        projPref[pref->GetName()] = pref;
        break;
      case plPreferences::Domain::Document:
        docPref[pref->GetName()] = pref;
        break;
    }
  }

  // create the objects in a certain order

  for (auto it = appPref.GetIterator(); it.IsValid(); ++it)
  {
    m_DocumentBinding[NativeToObject(it.Value())] = it.Value()->GetDocumentAssociation();
  }

  for (auto it = projPref.GetIterator(); it.IsValid(); ++it)
  {
    m_DocumentBinding[NativeToObject(it.Value())] = it.Value()->GetDocumentAssociation();
  }

  for (auto it = docPref.GetIterator(); it.IsValid(); ++it)
  {
    m_DocumentBinding[NativeToObject(it.Value())] = it.Value()->GetDocumentAssociation();
  }
}

void plQtPreferencesDlg::PropertyChangedEventHandler(const plDocumentObjectPropertyEvent& e)
{
  const plUuid guid = e.m_pObject->GetGuid();
  PLASMA_ASSERT_DEV(m_DocumentBinding.Contains(guid), "Object GUID is not in the known list!");

  ObjectToNative(guid, m_DocumentBinding[guid]);
}

void plQtPreferencesDlg::ApplyAllChanges()
{
  for (auto it = m_DocumentBinding.GetIterator(); it.IsValid(); ++it)
  {
    ObjectToNative(it.Key(), it.Value());
  }
}
