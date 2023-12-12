#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetProfilesDlg.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>

#include <QDialog>

class plAssetProfilesDocument;
class plPlatformProfile;
class plQtDocumentTreeView;
class plDocument;
struct plDocumentObjectPropertyEvent;

class PLASMA_EDITORFRAMEWORK_DLL plQtAssetProfilesDlg : public QDialog, public Ui_plQtAssetProfilesDlg
{
public:
  Q_OBJECT

public:
  plQtAssetProfilesDlg(QWidget* parent);
  ~plQtAssetProfilesDlg();

  plUInt32 m_uiActiveConfig = 0;

private Q_SLOTS:
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked();
  void OnItemDoubleClicked(QModelIndex idx);
  void on_AddButton_clicked();
  void on_DeleteButton_clicked();
  void on_RenameButton_clicked();
  void on_SwitchToButton_clicked();

private:
  struct Binding
  {
    enum class State
    {
      None,
      Added,
      Deleted
    };

    State m_State = State::None;
    plPlatformProfile* m_pProfile = nullptr;
  };

  bool DetermineNewProfileName(QWidget* parent, plString& result);
  bool CheckProfileNameUniqueness(const char* szName);
  void AllAssetProfilesToObject();
  void PropertyChangedEventHandler(const plDocumentObjectPropertyEvent& e);
  void ApplyAllChanges();
  plUuid NativeToObject(plPlatformProfile* pProfile);
  void ObjectToNative(plUuid objectGuid, plPlatformProfile* pProfile);
  void SelectionEventHandler(const plSelectionManagerEvent& e);

  plAssetProfilesDocument* m_pDocument;
  plMap<plUuid, Binding> m_ProfileBindings;
};

