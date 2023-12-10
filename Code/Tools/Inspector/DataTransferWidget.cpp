#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <Inspector/DataTransferWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QTableWidget>
#include <QTemporaryFile>
#include <QUrl>
#include <qdesktopservices.h>

plQtDataWidget* plQtDataWidget::s_pWidget = nullptr;

plQtDataWidget::plQtDataWidget(QWidget* pParent)
  : ads::CDockWidget("Data Transfer Widget", pParent)
{
  /// \todo Improve Data Transfer UI

  s_pWidget = this;

  setupUi(this);
  setWidget(DataTransferWidgetFrame);

  setIcon(QIcon(":/Icons/Icons/database_refresh.svg"));

  ResetStats();
}

void plQtDataWidget::ResetStats()
{
  m_Transfers.Clear();
  ComboTransfers->clear();
  ComboItems->clear();
}

void plQtDataWidget::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  plTelemetryMessage msg;

  while (plTelemetry::RetrieveMessage('TRAN', msg) == PLASMA_SUCCESS)
  {
    if (msg.GetMessageID() == ' CLR')
    {
      s_pWidget->ResetStats();
    }

    if (msg.GetMessageID() == 'ENBL')
    {
      plString sName;
      msg.GetReader() >> sName;

      // this will create the item, do not remove!
      TransferData& td = s_pWidget->m_Transfers[sName];
      PLASMA_IGNORE_UNUSED(td);

      s_pWidget->ComboTransfers->addItem(sName.GetData());
    }

    if (msg.GetMessageID() == 'DSBL')
    {
      plString sName;
      msg.GetReader() >> sName;

      auto it = s_pWidget->m_Transfers.Find(sName);

      if (it.IsValid())
      {
        plInt32 iIndex = s_pWidget->ComboTransfers->findText(sName.GetData());

        if (iIndex >= 0)
          s_pWidget->ComboTransfers->removeItem(iIndex);
      }
    }

    if (msg.GetMessageID() == 'DATA')
    {
      plString sBelongsTo, sName, sMimeType, sExtension;

      msg.GetReader() >> sBelongsTo;
      msg.GetReader() >> sName;
      msg.GetReader() >> sMimeType;
      msg.GetReader() >> sExtension;

      auto Transfer = s_pWidget->m_Transfers.Find(sBelongsTo);

      if (Transfer.IsValid())
      {
        TransferDataObject& tdo = Transfer.Value().m_Items[sName];
        tdo.m_sMimeType = sMimeType;
        tdo.m_sExtension = sExtension;

        plMemoryStreamWriter Writer(&tdo.m_Storage);

        // copy the entire memory stream over and store it for later
        while (true)
        {
          plUInt8 uiTemp[1024];
          const plUInt32 uiRead = msg.GetReader().ReadBytes(uiTemp, 1024);

          if (uiRead == 0)
            break;

          Writer.WriteBytes(uiTemp, uiRead).IgnoreResult();
        }

        s_pWidget->on_ComboTransfers_currentIndexChanged(s_pWidget->ComboTransfers->currentIndex());
      }
    }
  }
}

void plQtDataWidget::on_ButtonRefresh_clicked()
{
  if (ComboTransfers->currentIndex() < 0)
    return;

  const plStringBuilder sName = ComboTransfers->currentText().toUtf8().data();

  auto it = m_Transfers.Find(sName);

  if (!it.IsValid())
    return;

  LabelImage->setPixmap(QPixmap());
  LabelImage->setText("Waiting for data transfer...");

  plTelemetryMessage msg;
  msg.SetMessageID('DTRA', 'REQ');
  msg.GetWriter() << it.Key();
  plTelemetry::SendToServer(msg);
}

void plQtDataWidget::on_ComboTransfers_currentIndexChanged(int index)
{
  ComboItems->clear();

  if (index < 0)
    return;

  plStringBuilder sName = ComboTransfers->currentText().toUtf8().data();

  auto itTransfer = m_Transfers.Find(sName);

  if (!itTransfer.IsValid())
    return;

  {
    plQtScopedUpdatesDisabled _1(ComboItems);

    for (auto itItem = itTransfer.Value().m_Items.GetIterator(); itItem.IsValid(); ++itItem)
    {
      ComboItems->addItem(itItem.Key().GetData());
    }

    ComboItems->setCurrentIndex(0);
  }

  on_ComboItems_currentIndexChanged(ComboItems->currentIndex());
}

plQtDataWidget::TransferDataObject* plQtDataWidget::GetCurrentItem()
{
  auto Transfer = GetCurrentTransfer();

  if (Transfer == nullptr)
    return nullptr;

  plString sItem = ComboItems->currentText().toUtf8().data();

  auto itItem = Transfer->m_Items.Find(sItem);
  if (!itItem.IsValid())
    return nullptr;

  return &itItem.Value();
}

plQtDataWidget::TransferData* plQtDataWidget::GetCurrentTransfer()
{
  plString sTransfer = ComboTransfers->currentText().toUtf8().data();

  auto itTransfer = m_Transfers.Find(sTransfer);
  if (!itTransfer.IsValid())
    return nullptr;

  return &itTransfer.Value();
}

void plQtDataWidget::on_ComboItems_currentIndexChanged(int index)
{
  if (index < 0)
    return;

  auto pItem = GetCurrentItem();

  if (!pItem)
    return;

  const plString sMime = pItem->m_sMimeType;
  auto& Stream = pItem->m_Storage;

  plMemoryStreamReader Reader(&Stream);

  if (sMime == "image/rgba8")
  {
    plUInt32 uiWidth, uiHeight;
    Reader >> uiWidth;
    Reader >> uiHeight;

    plDynamicArray<plUInt8> Image;
    Image.SetCountUninitialized(uiWidth * uiHeight * 4);

    Reader.ReadBytes(&Image[0], Image.GetCount());

    QImage i(&Image[0], uiWidth, uiHeight, QImage::Format_ARGB32);

    LabelImage->setPixmap(QPixmap::fromImage(i));
  }
  else if (sMime == "text/xml" || sMime == "application/json" || sMime == "text/plain")
  {
    const plUInt32 uiMaxBytes = plMath::Min<plUInt32>(1024 * 16, Reader.GetByteCount32());

    plHybridArray<plUInt8, 1024> Temp;
    Temp.SetCountUninitialized(uiMaxBytes + 1);

    Reader.ReadBytes(Temp.GetData(), uiMaxBytes);
    Temp[uiMaxBytes] = '\0';

    LabelImage->setText((const char*)Temp.GetData());
  }
  else
  {
    plStringBuilder sText;
    sText.Format("Cannot display data of Mime-Type '{0}'", sMime);

    LabelImage->setText(sText.GetData());
  }
}

bool plQtDataWidget::SaveToFile(TransferDataObject& item, plStringView sFile)
{
  auto& Stream = item.m_Storage;
  plMemoryStreamReader Reader(&Stream);

  plStringBuilder tmp;
  QFile FileOut(sFile.GetData(tmp));
  if (!FileOut.open(QIODevice::WriteOnly))
  {
    QMessageBox::warning(this, QLatin1String("Error writing to file"), QLatin1String("Could not open the specified file for writing."), QMessageBox::Ok, QMessageBox::Ok);
    return false;
  }

  plHybridArray<plUInt8, 1024> Temp;
  Temp.SetCountUninitialized(Reader.GetByteCount32());

  Reader.ReadBytes(&Temp[0], Reader.GetByteCount32());

  if (!Temp.IsEmpty())
    FileOut.write((const char*)&Temp[0], Temp.GetCount());

  FileOut.close();
  return true;
}

void plQtDataWidget::on_ButtonSave_clicked()
{
  auto pItem = GetCurrentItem();

  if (!pItem)
  {
    QMessageBox::information(this, QLatin1String("plInspector"), QLatin1String("No valid item selected."), QMessageBox::Ok, QMessageBox::Ok);
    return;
  }

  QString sFilter;

  if (!pItem->m_sExtension.IsEmpty())
  {
    sFilter = "Default (*.";
    sFilter.append(pItem->m_sExtension.GetData());
    sFilter.append(");;");
  }

  sFilter.append("All Files (*.*)");

  QString sResult = QFileDialog::getSaveFileName(this, QLatin1String("Save Data"), pItem->m_sFileName.GetData(), sFilter);

  if (sResult.isEmpty())
    return;

  pItem->m_sFileName = sResult.toUtf8().data();

  SaveToFile(*pItem, pItem->m_sFileName.GetData());
}

void plQtDataWidget::on_ButtonOpen_clicked()
{
  auto pItem = GetCurrentItem();

  if (!pItem)
  {
    QMessageBox::information(this, QLatin1String("plInspector"), QLatin1String("No valid item selected."), QMessageBox::Ok, QMessageBox::Ok);
    return;
  }

  if (pItem->m_sFileName.IsEmpty())
    on_ButtonSave_clicked();

  if (pItem->m_sFileName.IsEmpty())
    return;

  SaveToFile(*pItem, pItem->m_sFileName.GetData());

  if (!QDesktopServices::openUrl(QUrl(pItem->m_sFileName.GetData())))
    QMessageBox::information(this, QLatin1String("plInspector"), QLatin1String("Could not open the file. There is probably no application registered to handle this file type."), QMessageBox::Ok, QMessageBox::Ok);
}
