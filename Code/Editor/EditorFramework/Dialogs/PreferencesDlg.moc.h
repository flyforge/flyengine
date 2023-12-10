#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_PreferencesDlg.h>
#include <Foundation/Strings/String.h>
#include <QDialog>

class plPreferencesDocument;
class plPreferences;
class plQtDocumentTreeView;

class PLASMA_EDITORFRAMEWORK_DLL plQtPreferencesDlg : public QDialog, public Ui_plQtPreferencesDlg
{
public:
  Q_OBJECT

public:
  plQtPreferencesDlg(QWidget* pParent);
  ~plQtPreferencesDlg();

  plUuid NativeToObject(plPreferences* pPreferences);
  void ObjectToNative(plUuid objectGuid, const plDocument* pPrefDocument);


private Q_SLOTS:
  void on_ButtonOk_clicked();
  void on_ButtonCancel_clicked() { reject(); }

private:
  void RegisterAllPreferenceTypes();
  void AllPreferencesToObject();
  void PropertyChangedEventHandler(const plDocumentObjectPropertyEvent& e);
  void ApplyAllChanges();

  plPreferencesDocument* m_pDocument;
  plMap<plUuid, const plDocument*> m_DocumentBinding;
};

