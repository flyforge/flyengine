#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

#include <QEvent>
#include <QObject>
#include <QPointer>
#include <QSharedPointer>
#include <QWidget>
#include <QWidgetAction>

class QAction;
class QMenu;
class QLabel;
class QSlider;
class plAction;

/// \brief Glue class that maps plActions to QActions. QActions are only created if the plAction is actually mapped somewhere. Document and Global actions are manually executed and don't solely rely on Qt's ShortcutContext setting to prevent ambiguous action shortcuts.
class PLASMA_GUIFOUNDATION_DLL plQtProxy : public QObject
{
  Q_OBJECT

public:
  plQtProxy();
  virtual ~plQtProxy();

  virtual void Update() = 0;

  virtual void SetAction(plAction* pAction);
  plAction* GetAction() { return m_pAction; }

  /// \brief Converts the QKeyEvent into a shortcut and tries to find a matching action in the document and global action list.
  ///
  /// Document actions are not mapped as ShortcutContext::WindowShortcut because docking allows for multiple documents to be mapped into the same window. Instead, ShortcutContext::WidgetWithChildrenShortcut is used to prevent ambiguous action shortcuts and the actions are executed manually via filtering QEvent::ShortcutOverride at the dock widget level.
  /// The function always has to be called two times:
  /// A: QEvent::ShortcutOverride: Only check with bTestOnly = true that we want to override the shortcut. This will instruct Qt to send the event as a regular key press event to the widget that accepted the override.
  /// B: QEvent::keyPressEvent: Execute the actual action with bTestOnly = false;
  ///
  /// \param pDocument The document for which matching actions should be searched for. If null, only global actions are searched.
  /// \param pEvent The key event that should be converted into a shortcut.
  /// \param bTestOnly Accept the event and return true but don't execute the action. Use this inside QEvent::ShortcutOverride.
  /// \return Whether the key event was consumed and an action executed.
  static bool TriggerDocumentAction(plDocument* pDocument, QKeyEvent* pEvent, bool bTestOnly);

  static plRttiMappedObjectFactory<plQtProxy>& GetFactory();
  static QSharedPointer<plQtProxy> GetProxy(plActionContext& ref_context, plActionDescriptorHandle hAction);

protected:
  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, QtProxies);
  static plRttiMappedObjectFactory<plQtProxy> s_Factory;
  static plMap<plActionDescriptorHandle, QWeakPointer<plQtProxy>> s_GlobalActions;
  static plMap<const plDocument*, plMap<plActionDescriptorHandle, QWeakPointer<plQtProxy>>> s_DocumentActions;
  static plMap<QWidget*, plMap<plActionDescriptorHandle, QWeakPointer<plQtProxy>>> s_WindowActions;
  static QObject* s_pSignalProxy;

protected:
  plAction* m_pAction;
};

class PLASMA_GUIFOUNDATION_DLL plQtActionProxy : public plQtProxy
{
  Q_OBJECT

public:
  virtual QAction* GetQAction() = 0;
};

class PLASMA_GUIFOUNDATION_DLL plQtCategoryProxy : public plQtProxy
{
  Q_OBJECT
public:
  virtual void Update() override {}
};

class PLASMA_GUIFOUNDATION_DLL plQtMenuProxy : public plQtProxy
{
  Q_OBJECT

public:
  plQtMenuProxy();
  ~plQtMenuProxy();

  virtual void Update() override;
  virtual void SetAction(plAction* pAction) override;

  virtual QMenu* GetQMenu();

protected:
  QMenu* m_pMenu;
};

class PLASMA_GUIFOUNDATION_DLL plQtButtonProxy : public plQtActionProxy
{
  Q_OBJECT

public:
  plQtButtonProxy();
  ~plQtButtonProxy();

  virtual void Update() override;
  virtual void SetAction(plAction* pAction) override;

  virtual QAction* GetQAction() override;

private Q_SLOTS:
  void OnTriggered();

private:
  void StatusUpdateEventHandler(plAction* pAction);

private:
  QPointer<QAction> m_pQtAction;
};


class PLASMA_GUIFOUNDATION_DLL plQtDynamicMenuProxy : public plQtMenuProxy
{
  Q_OBJECT

public:
  virtual void SetAction(plAction* pAction) override;

private Q_SLOTS:
  void SlotMenuAboutToShow();
  void SlotMenuEntryTriggered();

private:
  plHybridArray<plDynamicMenuAction::Item, 16> m_Entries;
};

class PLASMA_GUIFOUNDATION_DLL plQtDynamicActionAndMenuProxy : public plQtDynamicMenuProxy
{
  Q_OBJECT

public:
  plQtDynamicActionAndMenuProxy();
  ~plQtDynamicActionAndMenuProxy();

  virtual void Update() override;
  virtual void SetAction(plAction* pAction) override;
  virtual QAction* GetQAction();

private Q_SLOTS:
  void OnTriggered();

private:
  void StatusUpdateEventHandler(plAction* pAction);

private:
  QPointer<QAction> m_pQtAction;
};


class PLASMA_GUIFOUNDATION_DLL plQtLabeledSlider : public QWidget
{
  Q_OBJECT

public:
  plQtLabeledSlider(QWidget* pParent);

  QLabel* m_pLabel;
  QSlider* m_pSlider;
};


class PLASMA_GUIFOUNDATION_DLL plQtSliderWidgetAction : public QWidgetAction
{
  Q_OBJECT

public:
  plQtSliderWidgetAction(QWidget* pParent);
  void setMinimum(int value);
  void setMaximum(int value);
  void setValue(int value);

Q_SIGNALS:
  void valueChanged(int value);

private Q_SLOTS:
  void OnValueChanged(int value);

protected:
  virtual QWidget* createWidget(QWidget* parent) override;
  virtual bool eventFilter(QObject* obj, QEvent* e) override;

  plInt32 m_iMinimum;
  plInt32 m_iMaximum;
  plInt32 m_iValue;
};

class PLASMA_GUIFOUNDATION_DLL plQtSliderProxy : public plQtActionProxy
{
  Q_OBJECT

public:
  plQtSliderProxy();
  ~plQtSliderProxy();

  virtual void Update() override;
  virtual void SetAction(plAction* pAction) override;

  virtual QAction* GetQAction() override;

private Q_SLOTS:
  void OnValueChanged(int value);

private:
  void StatusUpdateEventHandler(plAction* pAction);

private:
  QPointer<plQtSliderWidgetAction> m_pQtAction;
};

