#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/ui_LogWidget.h>
#include <QWidget>

class plQtLogModel;
class plQtSearchWidget;

/// \brief The application wide panel that shows the engine log output and the editor log output
class PLASMA_GUIFOUNDATION_DLL plQtLogWidget : public QWidget, public Ui_LogWidget
{
  Q_OBJECT

public:
  plQtLogWidget(QWidget* pParent);
  ~plQtLogWidget();

  void ShowControls(bool bShow);

  plQtLogModel* GetLog();
  plQtSearchWidget* GetSearchWidget();
  void SetLogLevel(plLogMsgType::Enum logLevel);
  plLogMsgType::Enum GetLogLevel() const;

  virtual bool eventFilter(QObject* pObject, QEvent* pEvent) override;

private Q_SLOTS:
  void on_ButtonClearLog_clicked();
  void on_Search_textChanged(const QString& text);
  void on_ComboFilter_currentIndexChanged(int index);

private:
  plQtLogModel* m_pLog;
  void ScrollToBottomIfAtEnd(int iNumElements);
};

