#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

class QHBoxLayout;
class QPushButton;
class QMenu;
class plQtSearchableMenu;

class PL_GUIFOUNDATION_DLL plQtAddSubElementButton : public plQtPropertyWidget
{
  Q_OBJECT

public:
  plQtAddSubElementButton();

  static bool s_bShowInDevelopmentFeatures;

protected:
  virtual void DoPrepareToDie() override {}

private Q_SLOTS:
  void onMenuAboutToShow();
  void on_Button_clicked();
  void OnMenuAction();

private:
  virtual void OnInit() override;
  void OnAction(const plRTTI* pRtti);

  QMenu* CreateCategoryMenu(const char* szCategory, plMap<plString, QMenu*>& existingMenus);

  QHBoxLayout* m_pLayout;
  QPushButton* m_pButton;

  plSet<const plRTTI*> m_SupportedTypes;

  bool m_bNoMoreElementsAllowed = false;
  QMenu* m_pMenu = nullptr;
  plQtSearchableMenu* m_pSearchableMenu = nullptr;
  plUInt32 m_uiMaxElements = 0; // 0 means unlimited
  bool m_bPreventDuplicates = false;

  // used to remember the last search term entered into the searchable menu
  // this should probably be per 'distinguishable menu', but currently it is just global
  static plString s_sLastMenuSearch;
};

