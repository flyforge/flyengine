#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/UIServices/QtProgressbar.h>
#include <QApplication>
#include <QProgressDialog>

plQtProgressbar::plQtProgressbar() = default;

plQtProgressbar::~plQtProgressbar()
{
  SetProgressbar(nullptr);
  EnsureDestroyed();
}

void plQtProgressbar::SetProgressbar(plProgress* pProgress)
{
  if (m_pProgress)
  {
    m_pProgress->m_Events.RemoveEventHandler(plMakeDelegate(&plQtProgressbar::ProgressbarEventHandler, this));
    m_pProgress = nullptr;
  }

  if (pProgress)
  {
    m_pProgress = pProgress;
    m_pProgress->m_Events.AddEventHandler(plMakeDelegate(&plQtProgressbar::ProgressbarEventHandler, this));
  }
}

void plQtProgressbar::ProgressbarEventHandler(const plProgressEvent& e)
{
  switch (e.m_Type)
  {
    case plProgressEvent::Type::ProgressStarted:
    {
      ++m_iNestedProcessEvents;
      EnsureCreated();
    }
    break;

    case plProgressEvent::Type::ProgressEnded:
    {
      EnsureDestroyed();
      --m_iNestedProcessEvents;
    }
    break;

    case plProgressEvent::Type::ProgressChanged:
    {
      ++m_iNestedProcessEvents;

      // make sure to fire all queued events before EnsureCreated()
      // because this might delete the progress dialog and then crash
      QCoreApplication::processEvents();

      EnsureCreated();

      plStringBuilder sText(e.m_pProgressbar->GetMainDisplayText(), "\n", e.m_pProgressbar->GetStepDisplayText());

      m_pDialog->setLabelText(QString::fromUtf8(sText.GetData()));
      PLASMA_ASSERT_DEV(m_pDialog != nullptr, "Progress dialog was destroyed while being in use");

      const plUInt32 uiProMille = plMath::Clamp<plUInt32>((plUInt32)(e.m_pProgressbar->GetCompletion() * 1000.0), 0, 1000);
      m_pDialog->setValue(uiProMille);

      if (m_pDialog->wasCanceled())
      {
        m_pProgress->UserClickedCancel();
      }

      QCoreApplication::processEvents();
      --m_iNestedProcessEvents;
    }
    break;

    case plProgressEvent::Type::CancelClicked:
      break;
  }
}

void plQtProgressbar::EnsureCreated()
{
  if (m_pDialog)
    return;

  m_pDialog = new QProgressDialog("                                                                                ", "Cancel", 0, 1000, QApplication::activeWindow());

  m_pDialog->setWindowModality(Qt::WindowModal);
  m_pDialog->setMinimumDuration((int)500);
  m_pDialog->setAutoReset(false);
  m_pDialog->setAutoClose(false);
  m_pDialog->show();

  if (!m_pProgress->AllowUserCancel())
    m_pDialog->setCancelButton(nullptr);

  auto ClearDialog = [this]() {
    // this can happen during tests
    m_pDialog = nullptr;
  };

  m_OnDialogDestroyed = QObject::connect(m_pDialog, &QObject::destroyed, ClearDialog);
}

void plQtProgressbar::EnsureDestroyed()
{
  if (m_pDialog)
  {
    delete m_pDialog;
    m_pDialog = nullptr;
  }
}
