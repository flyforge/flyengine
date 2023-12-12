#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/InputContexts/SelectionContext.h>
#include <EditorFramework/PropertyGrid/GameObjectReferencePropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plQtGameObjectReferencePropertyWidget::plQtGameObjectReferencePropertyWidget()
  : plQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  m_pLayout->setSpacing(0);
  setLayout(m_pLayout);

  m_pWidget = new QLabel(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

  m_pButton = new QToolButton(this);
  m_pButton->setText(QStringLiteral("..."));
  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);
  m_pButton->setIcon(QIcon(":/GuiFoundation/Icons/Cursor.svg"));
  m_pButton->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  m_pButton->setCursor(Qt::WhatsThisCursor);

  PLASMA_VERIFY(connect(m_pButton, SIGNAL(clicked()), this, SLOT(on_PickObject_clicked())) != nullptr, "signal/slot connection failed");
  PLASMA_VERIFY(
    connect(m_pButton, &QWidget::customContextMenuRequested, this, &plQtGameObjectReferencePropertyWidget::on_customContextMenuRequested) != nullptr,
    "signal/slot connection failed");

  m_pLayout->addWidget(m_pWidget);
  m_pLayout->addWidget(m_pButton);
}


void plQtGameObjectReferencePropertyWidget::OnInit()
{
  PLASMA_ASSERT_DEV(m_pProp->GetAttributeByType<plGameObjectReferenceAttribute>() != nullptr,
    "plQtGameObjectReferencePropertyWidget was created without a plGameObjectReferenceAttribute!");
}

void plQtGameObjectReferencePropertyWidget::InternalSetValue(const plVariant& value)
{
  plQtScopedBlockSignals b(m_pWidget);
  plQtScopedBlockSignals b2(m_pButton);

  if (!value.IsValid())
  {
    m_pWidget->setText(QStringLiteral("<Multiple Values>"));
  }
  else
  {
    SetValue(value.ConvertTo<plString>().GetData());
  }
}

void plQtGameObjectReferencePropertyWidget::FillContextMenu(QMenu& menu)
{
  if (!menu.isEmpty())
    menu.addSeparator();

  menu.setDefaultAction(
    menu.addAction(QIcon(":/GuiFoundation/Icons/Cursor.svg"), QLatin1String("Pick Object"), this, SLOT(on_PickObject_clicked())));
  QAction* pCopyAction =
    menu.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Copy.svg")), QLatin1String("Copy Object Reference"), this, SLOT(OnCopyReference()));
  menu.addAction(QIcon(":/GuiFoundation/Icons/Paste.svg"), QLatin1String("Paste Object Reference"), this, SLOT(OnPasteReference()));
  QAction* pSelectAction =
    menu.addAction(QIcon(":/GuiFoundation/Icons/Go.svg"), QLatin1String("Select Referenced Object"), this, SLOT(OnSelectReferencedObject()));
  QAction* pClearAction =
    menu.addAction(QIcon(":/GuiFoundation/Icons/Delete.svg"), QLatin1String("Clear Reference"), this, SLOT(OnClearReference()));

  pCopyAction->setEnabled(!m_sInternalValue.isEmpty());
  pSelectAction->setEnabled(!m_sInternalValue.isEmpty());
  // pClearAction->setEnabled(!m_sInternalValue.isEmpty()); // this would disable the clear button with multi selection
}

void plQtGameObjectReferencePropertyWidget::PickObjectOverride(const plDocumentObject* pObject)
{
  if (pObject != nullptr)
  {
    if (m_Items[0].m_pObject->GetDocumentObjectManager() != pObject->GetDocumentObjectManager())
    {
      if (plQtDocumentWindow* pWindow = plQtDocumentWindow::FindWindowByDocument(m_pGrid->GetDocument()))
      {
        pWindow->ShowTemporaryStatusBarMsg("Can't reference object in another layer.");
      }
      return;
    }

    plStringBuilder sGuid;
    plConversionUtils::ToString(pObject->GetGuid(), sGuid);

    SetValue(sGuid.GetData());
  }

  ClearPicking();
}

void plQtGameObjectReferencePropertyWidget::ClearPicking()
{
  m_pGrid->GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(
    plMakeDelegate(&plQtGameObjectReferencePropertyWidget::SelectionManagerEventHandler, this));

  for (auto pContext : m_SelectionContextsToUnsubscribe)
  {
    pContext->ResetPickObjectOverride();
  }

  m_SelectionContextsToUnsubscribe.Clear();
}

void plQtGameObjectReferencePropertyWidget::SelectionManagerEventHandler(const plSelectionManagerEvent& e)
{
  // if the selection changes while we wait for a picking result, clear the picking override
  ClearPicking();
}

void plQtGameObjectReferencePropertyWidget::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set palette.
  m_pWidget->setPalette(m_Pal);
  plQtStandardPropertyWidget::showEvent(event);
}

void plQtGameObjectReferencePropertyWidget::SetValue(const QString& sValue)
{
  // don't early out if the value is equal, otherwise that breaks clearing the reference with a multi-selection

  m_sInternalValue = sValue;

  const plDocumentObject* pObject = nullptr;
  plStringBuilder sDisplayName = m_sInternalValue.toUtf8().data();

  if (plConversionUtils::IsStringUuid(m_sInternalValue.toUtf8().data()))
  {
    const plUuid guid = plConversionUtils::ConvertStringToUuid(m_sInternalValue.toUtf8().data());

    pObject = m_pObjectAccessor->GetObject(guid);
  }

  if (pObject != nullptr)
  {
    m_Pal.setColor(QPalette::WindowText, QColor::fromRgb(182, 255, 0));
    m_pWidget->setToolTip(QStringLiteral("The reference is a known game object."));

    if (auto* pGoDoc = plDynamicCast<const plGameObjectDocument*>(m_pGrid->GetDocument()))
    {
      pGoDoc->QueryCachedNodeName(pObject, sDisplayName, nullptr, nullptr);
    }
  }
  else
  {
    m_Pal.setColor(QPalette::WindowText, Qt::red);
    m_pWidget->setToolTip(QStringLiteral("The reference is invalid."));
  }

  m_pWidget->setPalette(m_Pal);

  m_pWidget->setText(sDisplayName.GetData());
  BroadcastValueChanged(m_sInternalValue.toUtf8().data());
}

void plQtGameObjectReferencePropertyWidget::on_PickObject_clicked()
{
  auto dele = plMakeDelegate(&plQtGameObjectReferencePropertyWidget::SelectionManagerEventHandler, this);

  if (m_pGrid->GetDocument()->GetSelectionManager()->m_Events.HasEventHandler(dele))
  {
    // this happens when clicking the 'pick' button twice
    ClearPicking();
  }

  plQtDocumentWindow* pWindow = plQtDocumentWindow::FindWindowByDocument(m_pGrid->GetDocument());

  plQtGameObjectDocumentWindow* pGoWindow = qobject_cast<plQtGameObjectDocumentWindow*>(pWindow);

  if (pGoWindow == nullptr)
    return;

  for (auto pView : pGoWindow->GetViewWidgets())
  {
    if (auto pGoView = qobject_cast<plQtGameObjectViewWidget*>(pView))
    {
      pGoView->m_pSelectionContext->SetPickObjectOverride(plMakeDelegate(&plQtGameObjectReferencePropertyWidget::PickObjectOverride, this));

      m_SelectionContextsToUnsubscribe.PushBack(pGoView->m_pSelectionContext);
    }
  }


  m_pGrid->GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(dele);
}

void plQtGameObjectReferencePropertyWidget::on_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);
  FillContextMenu(m);

  m.exec(m_pButton->mapToGlobal(pt));
}

void plQtGameObjectReferencePropertyWidget::OnSelectReferencedObject()
{
  plStringBuilder sGuid = m_sInternalValue.toUtf8().data();

  if (!plConversionUtils::IsStringUuid(sGuid))
    return;

  const plUuid guid = plConversionUtils::ConvertStringToUuid(sGuid);

  if (const plDocumentObject* pObject = m_pObjectAccessor->GetObject(guid))
  {
    m_pGrid->GetDocument()->GetSelectionManager()->SetSelection(pObject);
  }
}

void plQtGameObjectReferencePropertyWidget::OnCopyReference()
{
  QClipboard* clipboard = QApplication::clipboard();
  clipboard->setText(m_sInternalValue);

  plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(
    plFmt("Copied Object Reference: {}", m_sInternalValue.toUtf8().data()), plTime::Seconds(5));
}


void plQtGameObjectReferencePropertyWidget::OnClearReference()
{
  SetValue("");
}

void plQtGameObjectReferencePropertyWidget::OnPasteReference()
{
  QClipboard* clipboard = QApplication::clipboard();
  QString sReference = clipboard->text();

  if (plConversionUtils::IsStringUuid(sReference.toUtf8().data()))
  {
    SetValue(sReference);
  }
}
