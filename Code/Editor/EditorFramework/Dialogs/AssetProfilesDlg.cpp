#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Dialogs/AssetProfilesDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

class plAssetProfilesObjectManager : public plDocumentObjectManager
{
public:
  virtual void GetCreateableTypes(plHybridArray<const plRTTI*, 32>& ref_types) const override { ref_types.PushBack(plGetStaticRTTI<plPlatformProfile>()); }
};

class plAssetProfilesDocument : public plDocument
{
  PL_ADD_DYNAMIC_REFLECTION(plAssetProfilesDocument, plDocument);

public:
  plAssetProfilesDocument(plStringView sDocumentPath)
    : plDocument(sDocumentPath, PL_DEFAULT_NEW(plAssetProfilesObjectManager))
  {
  }

public:
  virtual plDocumentInfo* CreateDocumentInfo() override { return PL_DEFAULT_NEW(plDocumentInfo); }
};

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAssetProfilesDocument, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

class plQtAssetConfigAdapter : public plQtNameableAdapter
{
public:
  plQtAssetConfigAdapter(const plQtAssetProfilesDlg* pDialog, const plDocumentObjectManager* pTree, const plRTTI* pType)
    : plQtNameableAdapter(pTree, pType, "", "Name")
  {
    m_pDialog = pDialog;
  }

  virtual QVariant data(const plDocumentObject* pObject, int iRow, int iColumn, int iRole) const override
  {
    if (iColumn == 0)
    {
      if (iRole == Qt::DecorationRole)
      {
        const plInt32 iPlatform = pObject->GetTypeAccessor().GetValue("Platform").ConvertTo<plInt32>();

        switch (iPlatform)
        {
          case plProfileTargetPlatform::PC:
            return plQtUiServices::GetSingleton()->GetCachedIconResource(":EditorFramework/Icons/PlatformWindows.svg");

          case plProfileTargetPlatform::UWP:
            return plQtUiServices::GetSingleton()->GetCachedIconResource(":EditorFramework/Icons/PlatformWindows.svg"); // TODO: icon

          case plProfileTargetPlatform::Android:
            return plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/PlatformAndroid.svg");
        }
      }

      if (iRole == Qt::DisplayRole)
      {
        QString name = plQtNameableAdapter::data(pObject, iRow, iColumn, iRole).toString();

        if (iRow == plAssetCurator::GetSingleton()->GetActiveAssetProfileIndex())
        {
          name += " (active)";
        }
        else if (iRow == m_pDialog->m_uiActiveConfig)
        {
          name += " (switch to)";
        }

        return name;
      }
    }

    return plQtNameableAdapter::data(pObject, iRow, iColumn, iRole);
  }

private:
  const plQtAssetProfilesDlg* m_pDialog = nullptr;
};

plQtAssetProfilesDlg::plQtAssetProfilesDlg(QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);

  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  // do not allow to delete or rename the first item
  DeleteButton->setEnabled(false);
  RenameButton->setEnabled(false);

  m_pDocument = PL_DEFAULT_NEW(plAssetProfilesDocument, "<none>");
  m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(plMakeDelegate(&plQtAssetProfilesDlg::SelectionEventHandler, this));

  std::unique_ptr<plQtDocumentTreeModel> pModel(new plQtDocumentTreeModel(m_pDocument->GetObjectManager()));
  pModel->AddAdapter(new plQtDummyAdapter(m_pDocument->GetObjectManager(), plGetStaticRTTI<plDocumentRoot>(), "Children"));
  pModel->AddAdapter(new plQtAssetConfigAdapter(this, m_pDocument->GetObjectManager(), plPlatformProfile::GetStaticRTTI()));

  Tree->Initialize(m_pDocument, std::move(pModel));
  Tree->SetAllowDragDrop(false);
  Tree->SetAllowDeleteObjects(false);
  Tree->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  Tree->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

  connect(Tree, &QTreeView::doubleClicked, this, &plQtAssetProfilesDlg::OnItemDoubleClicked);

  AllAssetProfilesToObject();

  Properties->SetDocument(m_pDocument);

  auto& rootChildArray = m_pDocument->GetObjectManager()->GetRootObject()->GetChildren();

  if (!rootChildArray.IsEmpty())
  {
    m_pDocument->GetSelectionManager()->SetSelection(rootChildArray[plAssetCurator::GetSingleton()->GetActiveAssetProfileIndex()]);
  }
}

plQtAssetProfilesDlg::~plQtAssetProfilesDlg()
{
  delete Tree;
  Tree = nullptr;

  delete Properties;
  Properties = nullptr;

  PL_DEFAULT_DELETE(m_pDocument);
}

plUuid plQtAssetProfilesDlg::NativeToObject(plPlatformProfile* pProfile)
{
  const plRTTI* pType = pProfile->GetDynamicRTTI();
  // Write properties to graph.
  plAbstractObjectGraph graph;
  plRttiConverterContext context;
  plRttiConverterWriter conv(&graph, &context, true, true);

  const plUuid guid = plUuid::MakeUuid();
  context.RegisterObject(guid, pType, pProfile);
  plAbstractObjectNode* pNode = conv.AddObjectToGraph(pType, pProfile, "root");

  // Read from graph and write into matching document object.
  auto pRoot = m_pDocument->GetObjectManager()->GetRootObject();
  plDocumentObject* pObject = m_pDocument->GetObjectManager()->CreateObject(pType);
  m_pDocument->GetObjectManager()->AddObject(pObject, pRoot, "Children", -1);

  plDocumentObjectConverterReader objectConverter(&graph, m_pDocument->GetObjectManager(), plDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
  objectConverter.ApplyPropertiesToObject(pNode, pObject);

  return pObject->GetGuid();
}

void plQtAssetProfilesDlg::ObjectToNative(plUuid objectGuid, plPlatformProfile* pProfile)
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

  conv.ApplyPropertiesToObject(pNode, pType, pProfile);
}


void plQtAssetProfilesDlg::SelectionEventHandler(const plSelectionManagerEvent& e)
{
  const auto& selection = m_pDocument->GetSelectionManager()->GetSelection();

  const bool bAllowModification = !selection.IsEmpty() && (selection[0] != m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

  DeleteButton->setEnabled(bAllowModification);
  RenameButton->setEnabled(bAllowModification);
}

void plQtAssetProfilesDlg::on_ButtonOk_clicked()
{
  ApplyAllChanges();

  // SaveAssetProfileRuntimeConfig
  for (plUInt32 i = 0; i < plAssetCurator::GetSingleton()->GetNumAssetProfiles(); ++i)
  {
    plStringBuilder sProfileRuntimeDataFile;

    plPlatformProfile* pProfile = plAssetCurator::GetSingleton()->GetAssetProfile(i);

    sProfileRuntimeDataFile.Set(":project/RuntimeConfigs/", pProfile->GetConfigName(), ".plProfile");

    pProfile->SaveForRuntime(sProfileRuntimeDataFile).IgnoreResult();
  }

  accept();

  plAssetCurator::GetSingleton()->SaveAssetProfiles().IgnoreResult();
}

void plQtAssetProfilesDlg::on_ButtonCancel_clicked()
{
  m_uiActiveConfig = plAssetCurator::GetSingleton()->GetActiveAssetProfileIndex();
  reject();
}

void plQtAssetProfilesDlg::OnItemDoubleClicked(QModelIndex idx)
{
  if (m_uiActiveConfig == idx.row())
    return;

  const QModelIndex oldIdx = Tree->model()->index(m_uiActiveConfig, 0);

  m_uiActiveConfig = idx.row();

  QVector<int> roles;
  roles.push_back(Qt::DisplayRole);
  Tree->model()->dataChanged(idx, idx, roles);
  Tree->model()->dataChanged(oldIdx, oldIdx, roles);
}

bool plQtAssetProfilesDlg::CheckProfileNameUniqueness(const char* szName)
{
  if (plStringUtils::IsNullOrEmpty(szName))
  {
    plQtUiServices::GetSingleton()->MessageBoxInformation("Empty strings are not allowed as profile names.");
    return false;
  }

  if (!plStringUtils::IsValidIdentifierName(szName))
  {
    plQtUiServices::GetSingleton()->MessageBoxInformation("Profile names may only contain characters, digits and underscores.");
    return false;
  }

  const auto& objects = m_pDocument->GetObjectManager()->GetRootObject()->GetChildren();
  for (const plDocumentObject* pObject : objects)
  {
    if (pObject->GetTypeAccessor().GetValue("Name").ConvertTo<plString>().IsEqual_NoCase(szName))
    {
      plQtUiServices::GetSingleton()->MessageBoxInformation("A profile with this name already exists.");
      return false;
    }
  }

  return true;
}

bool plQtAssetProfilesDlg::DetermineNewProfileName(QWidget* parent, plString& result)
{
  while (true)
  {
    bool ok = false;
    result = QInputDialog::getText(parent, "Profile Name", "New Name:", QLineEdit::Normal, "", &ok).toUtf8().data();

    if (!ok)
      return false;

    if (CheckProfileNameUniqueness(result))
      return true;
  }
}

void plQtAssetProfilesDlg::on_AddButton_clicked()
{
  plString sProfileName;
  if (!DetermineNewProfileName(this, sProfileName))
    return;

  plPlatformProfile profile;
  profile.SetConfigName(sProfileName);
  profile.AddMissingConfigs();

  auto& binding = m_ProfileBindings[NativeToObject(&profile)];
  binding.m_pProfile = nullptr;
  binding.m_State = Binding::State::Added;

  // select the new profile
  m_pDocument->GetSelectionManager()->SetSelection(m_pDocument->GetObjectManager()->GetRootObject()->GetChildren().PeekBack());
}

void plQtAssetProfilesDlg::on_DeleteButton_clicked()
{
  const auto& sel = m_pDocument->GetSelectionManager()->GetSelection();
  if (sel.IsEmpty())
    return;

  // do not allow to delete the first object
  if (sel[0] == m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0])
    return;

  if (plQtUiServices::GetSingleton()->MessageBoxQuestion(plFmt("Delete the selected profile?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
    return;

  m_ProfileBindings[sel[0]->GetGuid()].m_State = Binding::State::Deleted;

  m_pDocument->GetCommandHistory()->StartTransaction("Delete Profile");

  plRemoveObjectCommand cmd;
  cmd.m_Object = sel[0]->GetGuid();

  m_pDocument->GetCommandHistory()->AddCommand(cmd).AssertSuccess();

  m_pDocument->GetCommandHistory()->FinishTransaction();
}

void plQtAssetProfilesDlg::on_RenameButton_clicked()
{
  const auto& sel = m_pDocument->GetSelectionManager()->GetSelection();
  if (sel.IsEmpty())
    return;

  // do not allow to rename the first object
  if (sel[0] == m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0])
    return;

  plString sProfileName;
  if (!DetermineNewProfileName(this, sProfileName))
    return;

  m_pDocument->GetCommandHistory()->StartTransaction("Rename Profile");

  plSetObjectPropertyCommand cmd;
  cmd.m_Object = sel[0]->GetGuid();
  cmd.m_sProperty = "Name";
  cmd.m_NewValue = sProfileName;

  m_pDocument->GetCommandHistory()->AddCommand(cmd).AssertSuccess();

  m_pDocument->GetCommandHistory()->FinishTransaction();
}

void plQtAssetProfilesDlg::on_SwitchToButton_clicked()
{
  const auto& sel = Tree->selectionModel()->selectedRows();
  if (sel.isEmpty())
    return;

  OnItemDoubleClicked(sel[0]);
}

void plQtAssetProfilesDlg::AllAssetProfilesToObject()
{
  m_uiActiveConfig = plAssetCurator::GetSingleton()->GetActiveAssetProfileIndex();

  m_ProfileBindings.Clear();

  for (plUInt32 i = 0; i < plAssetCurator::GetSingleton()->GetNumAssetProfiles(); ++i)
  {
    auto* pProfile = plAssetCurator::GetSingleton()->GetAssetProfile(i);

    m_ProfileBindings[NativeToObject(pProfile)].m_pProfile = pProfile;
  }
}

void plQtAssetProfilesDlg::PropertyChangedEventHandler(const plDocumentObjectPropertyEvent& e)
{
  const plUuid guid = e.m_pObject->GetGuid();
  PL_ASSERT_DEV(m_ProfileBindings.Contains(guid), "Object GUID is not in the known list!");

  ObjectToNative(guid, m_ProfileBindings[guid].m_pProfile);
}

void plQtAssetProfilesDlg::ApplyAllChanges()
{
  for (auto it = m_ProfileBindings.GetIterator(); it.IsValid(); ++it)
  {
    const auto& binding = it.Value();

    plPlatformProfile* pProfile = binding.m_pProfile;

    if (binding.m_State == Binding::State::Deleted)
    {
      plAssetCurator::GetSingleton()->DeleteAssetProfile(pProfile).IgnoreResult();
      continue;
    }

    if (binding.m_State == Binding::State::Added)
    {
      // create a new profile object and synchronize the state directly into that
      pProfile = plAssetCurator::GetSingleton()->CreateAssetProfile();
    }

    ObjectToNative(it.Key(), pProfile);
  }
}
