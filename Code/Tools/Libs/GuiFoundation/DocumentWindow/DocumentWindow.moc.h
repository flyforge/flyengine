#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Status.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Document/DocumentManager.h>

#include <QMainWindow>

class plQtContainerWindow;
class plDocument;
class plQtDocumentWindow;
class QLabel;
class QToolButton;

struct plQtDocumentWindowEvent
{
  enum Type
  {
    WindowClosing,           ///< Sent shortly before the window is being deleted
    WindowClosed,            ///< Sent AFTER the window has been deleted. The pointer is given, but not valid anymore!
    WindowDecorationChanged, ///< Window title or icon has changed
    BeforeRedraw,            ///< Sent shortly before the content of the window is being redrawn
  };

  Type m_Type;
  plQtDocumentWindow* m_pWindow;
};

/// \brief Base class for all document windows. Handles the most basic document window management.
class PLASMA_GUIFOUNDATION_DLL plQtDocumentWindow : public QMainWindow
{
  Q_OBJECT

public:
  static plEvent<const plQtDocumentWindowEvent&> s_Events;

public:
  plQtDocumentWindow(plDocument* pDocument);
  plQtDocumentWindow(const char* szUniqueName);
  virtual ~plQtDocumentWindow();

  void EnsureVisible();

  virtual plString GetWindowIcon() const;
  virtual plString GetDisplayName() const { return GetUniqueName(); }
  virtual plString GetDisplayNameShort() const;

  const char* GetUniqueName() const { return m_sUniqueName; }

  /// \brief The 'GroupName' is used for serializing window layouts. It should be unique among different window types.
  virtual const char* GetWindowLayoutGroupName() const = 0;

  plDocument* GetDocument() const { return m_pDocument; }

  plStatus SaveDocument();

  bool CanCloseWindow();
  void CloseDocumentWindow();

  void ScheduleRestoreWindowLayout();

  bool IsVisibleInContainer() const { return m_bIsVisibleInContainer; }
  void SetTargetFramerate(plInt16 uiTargetFPS);

  void TriggerRedraw();

  virtual void RequestWindowTabContextMenu(const QPoint& GlobalPos);

  static const plDynamicArray<plQtDocumentWindow*>& GetAllDocumentWindows() { return s_AllDocumentWindows; }

  static plQtDocumentWindow* FindWindowByDocument(const plDocument* pDocument);
  plQtContainerWindow* GetContainerWindow() const;

  /// \brief Shows the given message for the given duration in the statusbar, then shows the permanent message again.
  void ShowTemporaryStatusBarMsg(const plFormatString& sText, plTime duration = plTime::Seconds(5));

  /// \brief Sets which text to show permanently in the statusbar. Set an empty string to clear the message.
  void SetPermanentStatusBarMsg(const plFormatString& sText);

  /// \brief For unit tests to take a screenshot of the window (may include multiple views) to do image comparisons.
  virtual void CreateImageCapture(const char* szOutputPath);

  /// \brief In 'safe' mode we want to prevent the documents from using the stored window layout state
  static bool s_bAllowRestoreWindowLayout;

protected:
  virtual void showEvent(QShowEvent* event) override;
  virtual void hideEvent(QHideEvent* event) override;
  virtual bool event(QEvent* event) override;
  virtual bool eventFilter(QObject* obj, QEvent* e) override;

  void FinishWindowCreation();

private Q_SLOTS:
  void SlotRestoreLayout();
  void SlotRedraw();
  void SlotQueuedDelete();
  void OnPermanentGlobalStatusClicked(bool);
  void OnStatusBarMessageChanged(const QString& sNewText);

private:
  void SaveWindowLayout();
  void RestoreWindowLayout();
  void DisableWindowLayoutSaving();

  void ShutdownDocumentWindow();

private:
  friend class plQtContainerWindow;

  void SetVisibleInContainer(bool bVisible);

  bool m_bIsVisibleInContainer = false;
  bool m_bRedrawIsTriggered = false;
  bool m_bIsDrawingATM = false;
  bool m_bTriggerRedrawQueued = false;
  bool m_bAllowSaveWindowLayout = true;
  plInt16 m_iTargetFramerate = 0;
  plDocument* m_pDocument = nullptr;
  plQtContainerWindow* m_pContainerWindow = nullptr;
  QLabel* m_pPermanentDocumentStatusText = nullptr;
  QToolButton* m_pPermanentGlobalStatusButton = nullptr;

private:
  void Constructor();
  void DocumentManagerEventHandler(const plDocumentManager::Event& e);
  void DocumentEventHandler(const plDocumentEvent& e);
  void UIServicesEventHandler(const plQtUiServices::Event& e);
  void UIServicesTickEventHandler(const plQtUiServices::TickEvent& e);

  virtual void InternalDeleteThis() { delete this; }
  virtual bool InternalCanCloseWindow();
  virtual void InternalCloseDocumentWindow();
  virtual void InternalVisibleInContainerChanged(bool bVisible) {}
  virtual void InternalRedraw() {}

  plString m_sUniqueName;

  static plDynamicArray<plQtDocumentWindow*> s_AllDocumentWindows;
};

