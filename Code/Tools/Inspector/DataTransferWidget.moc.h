#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Strings/String.h>
#include <Inspector/ui_DataTransferWidget.h>
#include <ads/DockWidget.h>

class plQtDataWidget : public ads::CDockWidget, public Ui_DataTransferWidget
{
public:
  Q_OBJECT

public:
  plQtDataWidget(QWidget* pParent = 0);

  static plQtDataWidget* s_pWidget;

private Q_SLOTS:
  virtual void on_ButtonRefresh_clicked();
  virtual void on_ComboTransfers_currentIndexChanged(int index);
  virtual void on_ComboItems_currentIndexChanged(int index);
  virtual void on_ButtonSave_clicked();
  virtual void on_ButtonOpen_clicked();

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();

private:
  struct TransferDataObject
  {
    plString m_sMimeType;
    plString m_sExtension;
    plContiguousMemoryStreamStorage m_Storage;
    plString m_sFileName;
  };

  struct TransferData
  {
    plMap<plString, TransferDataObject> m_Items;
  };

  bool SaveToFile(TransferDataObject& item, plStringView sFile);

  TransferDataObject* GetCurrentItem();
  TransferData* GetCurrentTransfer();

  plMap<plString, TransferData> m_Transfers;
};

