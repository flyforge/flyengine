#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/ElementGroupButton.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>
#include <GuiFoundation/Widgets/InlinedGroupBox.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

#include <QClipboard>
#include <QDragEnterEvent>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QScrollArea>
#include <QStringBuilder>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plPropertyClipboard, plNoBase, 1, plRTTIDefaultAllocator<plPropertyClipboard>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("m_Type", m_Type),
    PLASMA_MEMBER_PROPERTY("m_Value", m_Value),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

/// *** BASE ***
plQtPropertyWidget::plQtPropertyWidget()
  : QWidget(nullptr)
  , m_pGrid(nullptr)
  , m_pProp(nullptr)
{
  m_bUndead = false;
  m_bIsDefault = true;
  setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

plQtPropertyWidget::~plQtPropertyWidget() {}

void plQtPropertyWidget::Init(
  plQtPropertyGridWidget* pGrid, plObjectAccessorBase* pObjectAccessor, const plRTTI* pType, const plAbstractProperty* pProp)
{
  m_pGrid = pGrid;
  m_pObjectAccessor = pObjectAccessor;
  m_pType = pType;
  m_pProp = pProp;
  PLASMA_ASSERT_DEBUG(m_pGrid && m_pObjectAccessor && m_pType && m_pProp, "");

  if (pProp->GetAttributeByType<plReadOnlyAttribute>() != nullptr || pProp->GetFlags().IsSet(plPropertyFlags::ReadOnly))
    setEnabled(false);

  OnInit();
}

void plQtPropertyWidget::SetSelection(const plHybridArray<plPropertySelection, 8>& items)
{
  m_Items = items;
}

const char* plQtPropertyWidget::GetLabel(plStringBuilder& ref_sTmp) const
{
  ref_sTmp.Set(m_pType->GetTypeName(), "::", m_pProp->GetPropertyName());
  return ref_sTmp;
}

void plQtPropertyWidget::ExtendContextMenu(QMenu& m)
{
  m.setToolTipsVisible(true);
  // revert
  {
    QAction* pRevert = m.addAction("Revert to Default");
    pRevert->setEnabled(!m_bIsDefault);
    connect(pRevert, &QAction::triggered, this, [this]()
      {
      m_pObjectAccessor->StartTransaction("Revert to Default");

      switch (m_pProp->GetCategory())
      {
        case plPropertyCategory::Enum::Array:
        case plPropertyCategory::Enum::Set:
        case plPropertyCategory::Enum::Map:
        {

          plStatus res = plStatus(PLASMA_SUCCESS);
          if (!m_Items[0].m_Index.IsValid())
          {
            // Revert container
            plDefaultContainerState defaultState(m_pObjectAccessor, m_Items, m_pProp->GetPropertyName());
            res = defaultState.RevertContainer();
          }
          else
          {
            const bool bIsValueType = plReflectionUtils::IsValueType(m_pProp) || m_pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags);
            if (bIsValueType)
            {
              // Revert container value type element
              plDefaultContainerState defaultState(m_pObjectAccessor, m_Items, m_pProp->GetPropertyName());
              res = defaultState.RevertElement({});
            }
            else
            {
              // Revert objects pointed to by the object type element
              plHybridArray<plPropertySelection, 8> ResolvedObjects;
              for (const auto& item : m_Items)
              {
                plUuid ObjectGuid = m_pObjectAccessor->Get<plUuid>(item.m_pObject, m_pProp, item.m_Index);
                if (ObjectGuid.IsValid())
                {
                  ResolvedObjects.PushBack({m_pObjectAccessor->GetObject(ObjectGuid), plVariant()});
                }
              }
              plDefaultObjectState defaultState(m_pObjectAccessor, ResolvedObjects);
              res = defaultState.RevertObject();
            }
          }
          if (res.Failed())
          {
            res.LogFailure();
            m_pObjectAccessor->CancelTransaction();
            return;
          }
        }
        break;
        default:
        {
          // Revert object member property
          plDefaultObjectState defaultState(m_pObjectAccessor, m_Items);
          plStatus res = defaultState.RevertProperty(m_pProp);
          if (res.Failed())
          {
            res.LogFailure();
            m_pObjectAccessor->CancelTransaction();
            return;
          }
        }
        break;
      }
      m_pObjectAccessor->FinishTransaction(); });
  }

  const char* szMimeType = "application/PlasmaEditor.Property";
  bool bValueType = plReflectionUtils::IsValueType(m_pProp) || m_pProp->GetFlags().IsAnySet(plPropertyFlags::Bitflags | plPropertyFlags::IsEnum);
  // Copy
  {
    plVariant commonValue = GetCommonValue(m_Items, m_pProp);
    QAction* pCopy = m.addAction("Copy Value");
    if (!bValueType)
    {
      pCopy->setEnabled(false);
      pCopy->setToolTip("Not a value type");
    }
    else if (!commonValue.IsValid())
    {
      pCopy->setEnabled(false);
      pCopy->setToolTip("No common value in selection");
    }

    connect(pCopy, &QAction::triggered, this, [this, szMimeType, commonValue]()
      {
      plPropertyClipboard content;
      content.m_Type = m_pProp->GetSpecificType()->GetTypeName();
      content.m_Value = commonValue;

      // Serialize
      plContiguousMemoryStreamStorage streamStorage;
      plMemoryStreamWriter memoryWriter(&streamStorage);
      plReflectionSerializer::WriteObjectToDDL(memoryWriter, plGetStaticRTTI<plPropertyClipboard>(), &content);
      memoryWriter.WriteBytes("\0", 1).IgnoreResult(); // null terminate

      // Write to clipboard
      QClipboard* clipboard = QApplication::clipboard();
      QMimeData* mimeData = new QMimeData();
      QByteArray encodedData((const char*)streamStorage.GetData(), streamStorage.GetStorageSize32());

      mimeData->setData(szMimeType, encodedData);
      mimeData->setText(QString::fromUtf8((const char*)streamStorage.GetData()));
      clipboard->setMimeData(mimeData); });
  }

  // Paste
  {
    QAction* pPaste = m.addAction("Paste Value");

    QClipboard* clipboard = QApplication::clipboard();
    auto mimedata = clipboard->mimeData();

    if (!bValueType)
    {
      pPaste->setEnabled(false);
      pPaste->setToolTip("Not a value type");
    }
    else if (!isEnabled())
    {
      pPaste->setEnabled(false);
      pPaste->setToolTip("Property is read only");
    }
    else if (!mimedata->hasFormat(szMimeType))
    {
      pPaste->setEnabled(false);
      pPaste->setToolTip("No property in clipboard");
    }
    else
    {
      QByteArray ba = mimedata->data(szMimeType);
      plRawMemoryStreamReader memoryReader(ba.data(), ba.count());

      plPropertyClipboard content;
      plReflectionSerializer::ReadObjectPropertiesFromDDL(memoryReader, *plGetStaticRTTI<plPropertyClipboard>(), &content);

      const bool bIsArray = m_pProp->GetCategory() == plPropertyCategory::Array || m_pProp->GetCategory() == plPropertyCategory::Set;
      const plRTTI* pClipboardType = plRTTI::FindTypeByName(content.m_Type);
      const bool bIsEnumeration = pClipboardType && (pClipboardType->IsDerivedFrom<plEnumBase>() || pClipboardType->IsDerivedFrom<plBitflagsBase>() || m_pProp->GetSpecificType()->IsDerivedFrom<plEnumBase>() || m_pProp->GetSpecificType()->IsDerivedFrom<plBitflagsBase>());
      const bool bEnumerationMissmatch = bIsEnumeration ? pClipboardType != m_pProp->GetSpecificType() : false;
      const plResult clamped = plReflectionUtils::ClampValue(content.m_Value, m_pProp->GetAttributeByType<plClampValueAttribute>());

      if (content.m_Value.IsA<plVariantArray>() != bIsArray)
      {
        pPaste->setEnabled(false);
        plStringBuilder sTemp;
        sTemp.Format("Cannot convert clipboard and property content between arrays and members.");
        pPaste->setToolTip(sTemp.GetData());
      }
      else if (bEnumerationMissmatch || !content.m_Value.CanConvertTo(m_pProp->GetSpecificType()->GetVariantType()) && content.m_Type != m_pProp->GetSpecificType()->GetTypeName())
      {
        pPaste->setEnabled(false);
        plStringBuilder sTemp;
        sTemp.Format("Cannot convert clipboard of type '{}' to property of type '{}'", content.m_Type, m_pProp->GetSpecificType()->GetTypeName());
        pPaste->setToolTip(sTemp.GetData());
      }
      else if (clamped.Failed())
      {
        pPaste->setEnabled(false);
        plStringBuilder sTemp;
        sTemp.Format("The member property '{}' has an plClampValueAttribute but plReflectionUtils::ClampValue failed.", m_pProp->GetPropertyName());
      }

      connect(pPaste, &QAction::triggered, this, [this, content]()
        {
        m_pObjectAccessor->StartTransaction("Paste Value");
        if (content.m_Value.IsA<plVariantArray>())
        {
          const plVariantArray& values = content.m_Value.Get<plVariantArray>();
          for (const plPropertySelection& sel : m_Items)
          {
            if (m_pObjectAccessor->Clear(sel.m_pObject, m_pProp->GetPropertyName()).Failed())
            {
              m_pObjectAccessor->CancelTransaction();
              return;
            }
            for (const plVariant& val : values)
            {
              if (m_pObjectAccessor->InsertValue(sel.m_pObject, m_pProp, val, -1).Failed())
              {
                m_pObjectAccessor->CancelTransaction();
                return;
              }
            }
          }
        }
        else
        {
          for (const plPropertySelection& sel : m_Items)
          {
            if (m_pObjectAccessor->SetValue(sel.m_pObject, m_pProp, content.m_Value, sel.m_Index).Failed())
            {
              m_pObjectAccessor->CancelTransaction();
              return;
            }
          }
        }

        m_pObjectAccessor->FinishTransaction(); });
    }
  }

  // copy internal name
  {
    auto lambda = [this]()
    {
      QClipboard* clipboard = QApplication::clipboard();
      QMimeData* mimeData = new QMimeData();
      mimeData->setText(m_pProp->GetPropertyName());
      clipboard->setMimeData(mimeData);

      plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(
        plFmt("Copied Property Name: {}", m_pProp->GetPropertyName()), plTime::Seconds(5));
    };

    QAction* pAction = m.addAction("Copy Internal Property Name:");
    connect(pAction, &QAction::triggered, this, lambda);

    QAction* pAction2 = m.addAction(m_pProp->GetPropertyName());
    connect(pAction2, &QAction::triggered, this, lambda);
  }
}

const plRTTI* plQtPropertyWidget::GetCommonBaseType(const plHybridArray<plPropertySelection, 8>& items)
{
  const plRTTI* pSubtype = nullptr;

  for (const auto& item : items)
  {
    const auto& accessor = item.m_pObject->GetTypeAccessor();

    if (pSubtype == nullptr)
      pSubtype = accessor.GetType();
    else
    {
      pSubtype = plReflectionUtils::GetCommonBaseType(pSubtype, accessor.GetType());
    }
  }

  return pSubtype;
}

QColor plQtPropertyWidget::SetPaletteBackgroundColor(plColorGammaUB inputColor, QPalette& ref_palette)
{
  QColor qColor = qApp->palette().color(QPalette::Window);
  if (inputColor.a != 0)
  {
    const plColor paletteColorLinear = qtToPlColor(qColor);
    const plColor inputColorLinear = inputColor;

    plColor blendedColor = plMath::Lerp(paletteColorLinear, inputColorLinear, inputColorLinear.a);
    blendedColor.a = 1.0f;
    qColor = plToQtColor(blendedColor);
  }

  ref_palette.setBrush(QPalette::Window, QBrush(qColor, Qt::SolidPattern));
  return qColor;
}

bool plQtPropertyWidget::GetCommonVariantSubType(
  const plHybridArray<plPropertySelection, 8>& items, const plAbstractProperty* pProperty, plVariantType::Enum& out_type)
{
  bool bFirst = true;
  // check if we have multiple values
  for (const auto& item : items)
  {
    if (bFirst)
    {
      bFirst = false;
      plVariant value;
      m_pObjectAccessor->GetValue(item.m_pObject, pProperty, value, item.m_Index).IgnoreResult();
      out_type = value.GetType();
    }
    else
    {
      plVariant valueNext;
      m_pObjectAccessor->GetValue(item.m_pObject, pProperty, valueNext, item.m_Index).IgnoreResult();
      if (valueNext.GetType() != out_type)
      {
        out_type = plVariantType::Invalid;
        return false;
      }
    }
  }
  return true;
}

plVariant plQtPropertyWidget::GetCommonValue(const plHybridArray<plPropertySelection, 8>& items, const plAbstractProperty* pProperty)
{
  if (!items[0].m_Index.IsValid() && (m_pProp->GetCategory() == plPropertyCategory::Array || m_pProp->GetCategory() == plPropertyCategory::Set))
  {
    plVariantArray values;
    // check if we have multiple values
    for (plUInt32 i = 0; i < items.GetCount(); i++)
    {
      const auto& item = items[i];
      if (i == 0)
      {
        m_pObjectAccessor->GetValues(item.m_pObject, pProperty, values).IgnoreResult();
      }
      else
      {
        plVariantArray valuesNext;
        m_pObjectAccessor->GetValues(item.m_pObject, pProperty, valuesNext).IgnoreResult();
        if (values != valuesNext)
        {
          return plVariant();
        }
      }
    }
    return values;
  }
  else
  {
    plVariant value;
    // check if we have multiple values
    for (const auto& item : items)
    {
      if (!value.IsValid())
      {
        m_pObjectAccessor->GetValue(item.m_pObject, pProperty, value, item.m_Index).IgnoreResult();
      }
      else
      {
        plVariant valueNext;
        m_pObjectAccessor->GetValue(item.m_pObject, pProperty, valueNext, item.m_Index).IgnoreResult();
        if (value != valueNext)
        {
          value = plVariant();
          break;
        }
      }
    }
    return value;
  }
}

void plQtPropertyWidget::PrepareToDie()
{
  PLASMA_ASSERT_DEBUG(!m_bUndead, "Object has already been marked for cleanup");

  m_bUndead = true;

  DoPrepareToDie();
}


void plQtPropertyWidget::OnCustomContextMenu(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);

  ExtendContextMenu(m);
  m_pGrid->ExtendContextMenu(m, m_Items, m_pProp);

  m.exec(pt); // pt is already in global space, because we fixed that
}

void plQtPropertyWidget::Broadcast(plPropertyEvent::Type type)
{
  plPropertyEvent ed;
  ed.m_Type = type;
  ed.m_pProperty = m_pProp;
  PropertyChangedHandler(ed);
}

void plQtPropertyWidget::PropertyChangedHandler(const plPropertyEvent& ed)
{
  if (m_bUndead)
    return;


  switch (ed.m_Type)
  {
    case plPropertyEvent::Type::SingleValueChanged:
    {
      plStringBuilder sTemp;
      sTemp.Format("Change Property '{0}'", plTranslate(ed.m_pProperty->GetPropertyName()));
      m_pObjectAccessor->StartTransaction(sTemp);

      plStatus res;
      for (const auto& sel : *ed.m_pItems)
      {
        res = m_pObjectAccessor->SetValue(sel.m_pObject, ed.m_pProperty, ed.m_Value, sel.m_Index);
        if (res.m_Result.Failed())
          break;
      }

      if (res.m_Result.Failed())
        m_pObjectAccessor->CancelTransaction();
      else
        m_pObjectAccessor->FinishTransaction();

      plQtUiServices::GetSingleton()->MessageBoxStatus(res, "Changing the property failed.");
    }
    break;

    case plPropertyEvent::Type::BeginTemporary:
    {
      plStringBuilder sTemp;
      sTemp.Format("Change Property '{0}'", plTranslate(ed.m_pProperty->GetPropertyName()));
      m_pObjectAccessor->BeginTemporaryCommands(sTemp);
    }
    break;

    case plPropertyEvent::Type::EndTemporary:
    {
      m_pObjectAccessor->FinishTemporaryCommands();
    }
    break;

    case plPropertyEvent::Type::CancelTemporary:
    {
      m_pObjectAccessor->CancelTemporaryCommands();
    }
    break;
  }
}

bool plQtPropertyWidget::eventFilter(QObject* pWatched, QEvent* pEvent)
{
  // if (pEvent->type() == QEvent::Wheel)
  // {
  //   if (pWatched->parent())
  //   {
  //     pWatched->parent()->event(pEvent);
  //   }

  //   return true;
  // }

  return false;
}

/// *** plQtUnsupportedPropertyWidget ***

plQtUnsupportedPropertyWidget::plQtUnsupportedPropertyWidget(const char* szMessage)
  : plQtPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QLabel(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pWidget);
  m_sMessage = szMessage;
}

void plQtUnsupportedPropertyWidget::OnInit()
{
  plQtScopedBlockSignals bs(m_pWidget);

  plStringBuilder tmp;
  QString sMessage = QStringLiteral("Unsupported Type: ") % QString::fromUtf8(m_pProp->GetSpecificType()->GetTypeName().GetData(tmp));
  if (!m_sMessage.IsEmpty())
    sMessage += QStringLiteral(" (") % QString::fromUtf8(m_sMessage, m_sMessage.GetElementCount()) % QStringLiteral(")");
  m_pWidget->setText(sMessage);
  m_pWidget->setToolTip(sMessage);
}


/// *** plQtStandardPropertyWidget ***

plQtStandardPropertyWidget::plQtStandardPropertyWidget()
  : plQtPropertyWidget()
{
}

void plQtStandardPropertyWidget::SetSelection(const plHybridArray<plPropertySelection, 8>& items)
{
  plQtPropertyWidget::SetSelection(items);

  m_OldValue = GetCommonValue(items, m_pProp);
  InternalSetValue(m_OldValue);
}

void plQtStandardPropertyWidget::BroadcastValueChanged(const plVariant& NewValue)
{
  if (NewValue == m_OldValue)
    return;

  m_OldValue = NewValue;

  plPropertyEvent ed;
  ed.m_Type = plPropertyEvent::Type::SingleValueChanged;
  ed.m_pProperty = m_pProp;
  ed.m_Value = NewValue;
  ed.m_pItems = &m_Items;
  PropertyChangedHandler(ed);
}


/// *** plQtPropertyPointerWidget ***

plQtPropertyPointerWidget::plQtPropertyPointerWidget()
  : plQtPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pGroup = new plQtCollapsibleGroupBox(this);
  m_pGroupLayout = new QHBoxLayout(nullptr);
  m_pGroupLayout->setSpacing(1);
  m_pGroupLayout->setContentsMargins(5, 0, 0, 0);
  m_pGroup->GetContent()->setLayout(m_pGroupLayout);

  m_pLayout->addWidget(m_pGroup);

  m_pAddButton = new plQtAddSubElementButton();
  m_pGroup->GetHeader()->layout()->addWidget(m_pAddButton);

  m_pDeleteButton = new plQtElementGroupButton(m_pGroup->GetHeader(), plQtElementGroupButton::ElementAction::DeleteElement, this);
  m_pGroup->GetHeader()->layout()->addWidget(m_pDeleteButton);
  connect(m_pDeleteButton, &QToolButton::clicked, this, &plQtPropertyPointerWidget::OnDeleteButtonClicked);

  m_pTypeWidget = nullptr;
}

plQtPropertyPointerWidget::~plQtPropertyPointerWidget()
{
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(
    plMakeDelegate(&plQtPropertyPointerWidget::StructureEventHandler, this));
}

void plQtPropertyPointerWidget::OnInit()
{
  UpdateTitle();
  m_pGrid->SetCollapseState(m_pGroup);
  connect(m_pGroup, &plQtGroupBoxBase::CollapseStateChanged, m_pGrid, &plQtPropertyGridWidget::OnCollapseStateChanged);

  // Add Buttons
  auto pAttr = m_pProp->GetAttributeByType<plContainerAttribute>();
  m_pAddButton->setVisible(!pAttr || pAttr->CanAdd());
  m_pDeleteButton->setVisible(!pAttr || pAttr->CanDelete());

  m_pAddButton->Init(m_pGrid, m_pObjectAccessor, m_pType, m_pProp);
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(
    plMakeDelegate(&plQtPropertyPointerWidget::StructureEventHandler, this));
}

void plQtPropertyPointerWidget::SetSelection(const plHybridArray<plPropertySelection, 8>& items)
{
  plQtScopedUpdatesDisabled _(this);

  plQtPropertyWidget::SetSelection(items);

  if (m_pTypeWidget)
  {
    m_pGroupLayout->removeWidget(m_pTypeWidget);
    delete m_pTypeWidget;
    m_pTypeWidget = nullptr;
  }


  plHybridArray<plPropertySelection, 8> emptyItems;
  plHybridArray<plPropertySelection, 8> subItems;
  for (const auto& item : m_Items)
  {
    plUuid ObjectGuid = m_pObjectAccessor->Get<plUuid>(item.m_pObject, m_pProp, item.m_Index);
    if (!ObjectGuid.IsValid())
    {
      emptyItems.PushBack(item);
    }
    else
    {
      plPropertySelection sel;
      sel.m_pObject = m_pObjectAccessor->GetObject(ObjectGuid);

      subItems.PushBack(sel);
    }
  }

  auto pAttr = m_pProp->GetAttributeByType<plContainerAttribute>();
  if (!pAttr || pAttr->CanAdd())
    m_pAddButton->setVisible(!emptyItems.IsEmpty());
  if (!pAttr || pAttr->CanDelete())
    m_pDeleteButton->setVisible(!subItems.IsEmpty());

  if (!emptyItems.IsEmpty())
  {
    m_pAddButton->SetSelection(emptyItems);
  }

  const plRTTI* pCommonType = nullptr;
  if (!subItems.IsEmpty())
  {
    pCommonType = plQtPropertyWidget::GetCommonBaseType(subItems);

    m_pTypeWidget = new plQtTypeWidget(m_pGroup->GetContent(), m_pGrid, m_pObjectAccessor, pCommonType, nullptr, nullptr);
    m_pTypeWidget->SetSelection(subItems);

    m_pGroupLayout->addWidget(m_pTypeWidget);
  }

  UpdateTitle(pCommonType);
}


void plQtPropertyPointerWidget::DoPrepareToDie()
{
  if (m_pTypeWidget)
  {
    m_pTypeWidget->PrepareToDie();
  }
}

void plQtPropertyPointerWidget::UpdateTitle(const plRTTI* pType /*= nullptr*/)
{
  plStringBuilder sb = plTranslate(m_pProp->GetPropertyName());
  if (pType != nullptr)
  {
    plStringBuilder tmp;
    sb.Append(": ", plTranslate(pType->GetTypeName().GetData(tmp)));
  }
  m_pGroup->SetTitle(sb);
}

void plQtPropertyPointerWidget::OnDeleteButtonClicked()
{
  m_pObjectAccessor->StartTransaction("Delete Object");

  plStatus res;
  const plHybridArray<plPropertySelection, 8> selection = m_pTypeWidget->GetSelection();
  for (auto& item : selection)
  {
    res = m_pObjectAccessor->RemoveObject(item.m_pObject);
    if (res.m_Result.Failed())
      break;
  }

  if (res.m_Result.Failed())
    m_pObjectAccessor->CancelTransaction();
  else
    m_pObjectAccessor->FinishTransaction();

  plQtUiServices::GetSingleton()->MessageBoxStatus(res, "Removing sub-element from the property failed.");
}

void plQtPropertyPointerWidget::StructureEventHandler(const plDocumentObjectStructureEvent& e)
{
  if (IsUndead())
    return;

  switch (e.m_EventType)
  {
    case plDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case plDocumentObjectStructureEvent::Type::AfterObjectMoved:
    case plDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      if (!e.m_sParentProperty.IsEqual(m_pProp->GetPropertyName()))
        return;

      if (std::none_of(cbegin(m_Items), cend(m_Items),
            [&](const plPropertySelection& sel)
            { return e.m_pNewParent == sel.m_pObject || e.m_pPreviousParent == sel.m_pObject; }))
        return;

      SetSelection(m_Items);
    }
    break;
    default:
      break;
  }
}

/// *** plQtEmbeddedClassPropertyWidget ***

plQtEmbeddedClassPropertyWidget::plQtEmbeddedClassPropertyWidget()
  : plQtPropertyWidget()
  , m_bTemporaryCommand(false)
  , m_pResolvedType(nullptr)
{
}


plQtEmbeddedClassPropertyWidget::~plQtEmbeddedClassPropertyWidget()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plQtEmbeddedClassPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtEmbeddedClassPropertyWidget::CommandHistoryEventHandler, this));
}

void plQtEmbeddedClassPropertyWidget::SetSelection(const plHybridArray<plPropertySelection, 8>& items)
{
  plQtScopedUpdatesDisabled _(this);

  plQtPropertyWidget::SetSelection(items);

  // Retrieve the objects the property points to. This could be an embedded class or
  // an element of an array, be it pointer or embedded class.
  m_ResolvedObjects.Clear();
  for (const auto& item : m_Items)
  {
    plUuid ObjectGuid = m_pObjectAccessor->Get<plUuid>(item.m_pObject, m_pProp, item.m_Index);
    plPropertySelection sel;
    sel.m_pObject = m_pObjectAccessor->GetObject(ObjectGuid);
    // sel.m_Index; intentionally invalid as we just retrieved the value so it is a pointer to an object

    m_ResolvedObjects.PushBack(sel);
  }

  m_pResolvedType = m_pProp->GetSpecificType();
  if (m_pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
  {
    m_pResolvedType = plQtPropertyWidget::GetCommonBaseType(m_ResolvedObjects);
  }
}

void plQtEmbeddedClassPropertyWidget::SetPropertyValue(const plAbstractProperty* pProperty, const plVariant& NewValue)
{
  plStatus res;
  for (const auto& sel : m_ResolvedObjects)
  {
    res = m_pObjectAccessor->SetValue(sel.m_pObject, pProperty, NewValue, sel.m_Index);
    if (res.m_Result.Failed())
      break;
  }
  // plPropertyEvent ed;
  // ed.m_Type = plPropertyEvent::Type::SingleValueChanged;
  // ed.m_pProperty = pProperty;
  // ed.m_Value = NewValue;
  // ed.m_pItems = &m_ResolvedObjects;

  // m_Events.Broadcast(ed);
}

void plQtEmbeddedClassPropertyWidget::OnInit()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plQtEmbeddedClassPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.AddEventHandler(plMakeDelegate(&plQtEmbeddedClassPropertyWidget::CommandHistoryEventHandler, this));
}


void plQtEmbeddedClassPropertyWidget::DoPrepareToDie() {}

void plQtEmbeddedClassPropertyWidget::PropertyEventHandler(const plDocumentObjectPropertyEvent& e)
{
  if (IsUndead())
    return;

  if (std::none_of(cbegin(m_ResolvedObjects), cend(m_ResolvedObjects), [=](const plPropertySelection& sel)
        { return e.m_pObject == sel.m_pObject; }))
    return;

  if (!m_QueuedChanges.Contains(e.m_sProperty))
  {
    m_QueuedChanges.PushBack(e.m_sProperty);
  }
}


void plQtEmbeddedClassPropertyWidget::CommandHistoryEventHandler(const plCommandHistoryEvent& e)
{
  if (IsUndead())
    return;

  switch (e.m_Type)
  {
    case plCommandHistoryEvent::Type::UndoEnded:
    case plCommandHistoryEvent::Type::RedoEnded:
    case plCommandHistoryEvent::Type::TransactionEnded:
    case plCommandHistoryEvent::Type::TransactionCanceled:
    {
      FlushQueuedChanges();
    }
    break;

    default:
      break;
  }
}

void plQtEmbeddedClassPropertyWidget::FlushQueuedChanges()
{
  for (const plString& sProperty : m_QueuedChanges)
  {
    OnPropertyChanged(sProperty);
  }

  m_QueuedChanges.Clear();
}

/// *** plQtPropertyTypeWidget ***

plQtPropertyTypeWidget::plQtPropertyTypeWidget(bool bAddCollapsibleGroup)
  : plQtPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);
  m_pGroup = nullptr;
  m_pGroupLayout = nullptr;

  if (bAddCollapsibleGroup)
  {
    m_pGroup = new plQtCollapsibleGroupBox(this);
    m_pGroupLayout = new QHBoxLayout(nullptr);
    m_pGroupLayout->setSpacing(1);
    m_pGroupLayout->setContentsMargins(5, 0, 0, 0);
    m_pGroup->GetContent()->setLayout(m_pGroupLayout);

    m_pLayout->addWidget(m_pGroup);
  }
  m_pTypeWidget = nullptr;
}

plQtPropertyTypeWidget::~plQtPropertyTypeWidget() {}

void plQtPropertyTypeWidget::OnInit()
{
  if (m_pGroup)
  {
    m_pGroup->SetTitle(plTranslate(m_pProp->GetPropertyName()));
    m_pGrid->SetCollapseState(m_pGroup);
    connect(m_pGroup, &plQtGroupBoxBase::CollapseStateChanged, m_pGrid, &plQtPropertyGridWidget::OnCollapseStateChanged);
  }
}

void plQtPropertyTypeWidget::SetSelection(const plHybridArray<plPropertySelection, 8>& items)
{
  plQtScopedUpdatesDisabled _(this);

  plQtPropertyWidget::SetSelection(items);

  QHBoxLayout* pLayout = m_pGroup != nullptr ? m_pGroupLayout : m_pLayout;
  QWidget* pOwner = m_pGroup != nullptr ? m_pGroup->GetContent() : this;
  if (m_pTypeWidget)
  {
    pLayout->removeWidget(m_pTypeWidget);
    delete m_pTypeWidget;
    m_pTypeWidget = nullptr;
  }

  // Retrieve the objects the property points to. This could be an embedded class or
  // an element of an array, be it pointer or embedded class.
  plHybridArray<plPropertySelection, 8> ResolvedObjects;
  for (const auto& item : m_Items)
  {
    plUuid ObjectGuid = m_pObjectAccessor->Get<plUuid>(item.m_pObject, m_pProp, item.m_Index);
    plPropertySelection sel;
    sel.m_pObject = m_pObjectAccessor->GetObject(ObjectGuid);
    // sel.m_Index; intentionally invalid as we just retrieved the value so it is a pointer to an object

    ResolvedObjects.PushBack(sel);
  }

  const plRTTI* pCommonType = nullptr;
  if (m_pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
  {
    pCommonType = plQtPropertyWidget::GetCommonBaseType(ResolvedObjects);
  }
  else
  {
    // If we create a widget for a member class we already determined the common base type at the parent type widget.
    // As we are not dealing with a pointer in this case the type must match the property exactly.
    pCommonType = m_pProp->GetSpecificType();
  }
  m_pTypeWidget = new plQtTypeWidget(pOwner, m_pGrid, m_pObjectAccessor, pCommonType, nullptr, nullptr);
  pLayout->addWidget(m_pTypeWidget);
  m_pTypeWidget->SetSelection(ResolvedObjects);
}


void plQtPropertyTypeWidget::SetIsDefault(bool bIsDefault)
{
  // The default state set by the parent object / container only refers to the element's correct position in the container but the entire state of the object. As recursively checking an entire object if is has any non-default values is quite costly, we just pretend the object is never in its default state the the user can click revert to default on any object at any time.
  m_bIsDefault = false;
}

void plQtPropertyTypeWidget::DoPrepareToDie()
{
  if (m_pTypeWidget)
  {
    m_pTypeWidget->PrepareToDie();
  }
}

/// *** plQtPropertyContainerWidget ***

plQtPropertyContainerWidget::plQtPropertyContainerWidget()
  : plQtPropertyWidget()
  , m_pAddButton(nullptr)
{
  m_Pal = palette();
  setAutoFillBackground(true);

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pGroup = new plQtCollapsibleGroupBox(this);
  m_pGroupLayout = new QVBoxLayout(nullptr);
  m_pGroupLayout->setSpacing(1);
  m_pGroupLayout->setContentsMargins(5, 0, 0, 0);
  m_pGroup->GetContent()->setLayout(m_pGroupLayout);
  m_pGroup->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  connect(m_pGroup, &QWidget::customContextMenuRequested, this, &plQtPropertyContainerWidget::OnContainerContextMenu);

  setAcceptDrops(true);
  m_pLayout->addWidget(m_pGroup);
}

plQtPropertyContainerWidget::~plQtPropertyContainerWidget()
{
  Clear();
}

void plQtPropertyContainerWidget::SetSelection(const plHybridArray<plPropertySelection, 8>& items)
{
  plQtPropertyWidget::SetSelection(items);

  UpdateElements();

  if (m_pAddButton)
  {
    m_pAddButton->SetSelection(m_Items);
  }
}

void plQtPropertyContainerWidget::SetIsDefault(bool bIsDefault)
{
  // This is called from the type widget which we ignore as we have a tighter scoped default value provider for containers.
}

void plQtPropertyContainerWidget::DoPrepareToDie()
{
  for (const auto& e : m_Elements)
  {
    e.m_pWidget->PrepareToDie();
  }
}

void plQtPropertyContainerWidget::dragEnterEvent(QDragEnterEvent* event)
{
  updateDropIndex(event);
}

void plQtPropertyContainerWidget::dragMoveEvent(QDragMoveEvent* event)
{
  updateDropIndex(event);
}

void plQtPropertyContainerWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
  m_iDropSource = -1;
  m_iDropTarget = -1;
  update();
}

void plQtPropertyContainerWidget::dropEvent(QDropEvent* event)
{
  if (updateDropIndex(event))
  {
    plQtGroupBoxBase* pGroup = qobject_cast<plQtGroupBoxBase*>(event->source());
    Element* pDragElement =
      std::find_if(begin(m_Elements), end(m_Elements), [pGroup](const Element& elem) -> bool
        { return elem.m_pSubGroup == pGroup; });
    if (pDragElement)
    {
      const plAbstractProperty* pProp = pDragElement->m_pWidget->GetProperty();
      plHybridArray<plPropertySelection, 8> items = pDragElement->m_pWidget->GetSelection();
      if (m_iDropSource != m_iDropTarget && (m_iDropSource + 1) != m_iDropTarget)
      {
        MoveItems(items, m_iDropTarget - m_iDropSource);
      }
    }
  }
  m_iDropSource = -1;
  m_iDropTarget = -1;
  update();
}

void plQtPropertyContainerWidget::paintEvent(QPaintEvent* event)
{
  plQtPropertyWidget::paintEvent(event);
  if (m_iDropSource != -1 && m_iDropTarget != -1)
  {
    plInt32 iYPos = 0;
    if (m_iDropTarget < (plInt32)m_Elements.GetCount())
    {
      const QPoint globalPos = m_Elements[m_iDropTarget].m_pSubGroup->mapToGlobal(QPoint(0, 0));
      iYPos = mapFromGlobal(globalPos).y();
    }
    else
    {
      const QPoint globalPos = m_Elements[m_Elements.GetCount() - 1].m_pSubGroup->mapToGlobal(QPoint(0, 0));
      iYPos = mapFromGlobal(globalPos).y() + m_Elements[m_Elements.GetCount() - 1].m_pSubGroup->height();
    }

    QPainter painter(this);
    painter.setPen(QPen(Qt::PenStyle::NoPen));
    painter.setBrush(palette().brush(QPalette::Highlight));
    painter.drawRect(0, iYPos - 3, width(), 4);
  }
}

void plQtPropertyContainerWidget::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set palette.
  setPalette(m_Pal);
  plQtPropertyWidget::showEvent(event);
}

bool plQtPropertyContainerWidget::updateDropIndex(QDropEvent* pEvent)
{
  if (pEvent->source() && pEvent->mimeData()->hasFormat("application/x-groupBoxDragProperty"))
  {
    // Is the drop source part of this widget?
    for (plUInt32 i = 0; i < m_Elements.GetCount(); i++)
    {
      if (m_Elements[i].m_pSubGroup == pEvent->source())
      {
        pEvent->setDropAction(Qt::MoveAction);
        pEvent->accept();
        plInt32 iNewDropTarget = -1;
        // Find closest drop target.
        const plInt32 iGlobalYPos = mapToGlobal(pEvent->pos()).y();
        for (plUInt32 j = 0; j < m_Elements.GetCount(); j++)
        {
          const QRect rect(m_Elements[j].m_pSubGroup->mapToGlobal(QPoint(0, 0)), m_Elements[j].m_pSubGroup->size());
          if (iGlobalYPos > rect.center().y())
          {
            iNewDropTarget = (plInt32)j + 1;
          }
          else if (iGlobalYPos < rect.center().y())
          {
            iNewDropTarget = (plInt32)j;
            break;
          }
        }
        if (m_iDropSource != (plInt32)i || m_iDropTarget != iNewDropTarget)
        {
          m_iDropSource = (plInt32)i;
          m_iDropTarget = iNewDropTarget;
          update();
        }
        return true;
      }
    }
  }

  if (m_iDropSource != -1 || m_iDropTarget != -1)
  {
    m_iDropSource = -1;
    m_iDropTarget = -1;
    update();
  }
  pEvent->ignore();
  return false;
}

void plQtPropertyContainerWidget::OnElementButtonClicked()
{
  plQtElementGroupButton* pButton = qobject_cast<plQtElementGroupButton*>(sender());
  const plAbstractProperty* pProp = pButton->GetGroupWidget()->GetProperty();
  plHybridArray<plPropertySelection, 8> items = pButton->GetGroupWidget()->GetSelection();

  switch (pButton->GetAction())
  {
    case plQtElementGroupButton::ElementAction::MoveElementUp:
    {
      MoveItems(items, -1);
    }
    break;
    case plQtElementGroupButton::ElementAction::MoveElementDown:
    {
      MoveItems(items, 2);
    }
    break;
    case plQtElementGroupButton::ElementAction::DeleteElement:
    {
      DeleteItems(items);
    }
    break;

    case plQtElementGroupButton::ElementAction::Help:
      // handled by custom lambda
      break;
  }
}

void plQtPropertyContainerWidget::OnDragStarted(QMimeData& ref_mimeData)
{
  plQtGroupBoxBase* pGroup = qobject_cast<plQtGroupBoxBase*>(sender());
  Element* pDragElement =
    std::find_if(begin(m_Elements), end(m_Elements), [pGroup](const Element& elem) -> bool
      { return elem.m_pSubGroup == pGroup; });
  if (pDragElement)
  {
    ref_mimeData.setData("application/x-groupBoxDragProperty", QByteArray());
  }
}

void plQtPropertyContainerWidget::OnContainerContextMenu(const QPoint& pt)
{
  plQtGroupBoxBase* pGroup = qobject_cast<plQtGroupBoxBase*>(sender());

  QMenu m;
  m.setToolTipsVisible(true);
  ExtendContextMenu(m);

  if (!m.isEmpty())
  {
    m.exec(pGroup->mapToGlobal(pt));
  }
}

void plQtPropertyContainerWidget::OnCustomElementContextMenu(const QPoint& pt)
{
  plQtGroupBoxBase* pGroup = qobject_cast<plQtGroupBoxBase*>(sender());
  Element* pElement = std::find_if(begin(m_Elements), end(m_Elements), [pGroup](const Element& elem) -> bool
    { return elem.m_pSubGroup == pGroup; });

  if (pElement)
  {
    QMenu m;
    m.setToolTipsVisible(true);
    pElement->m_pWidget->ExtendContextMenu(m);

    m_pGrid->ExtendContextMenu(m, pElement->m_pWidget->GetSelection(), pElement->m_pWidget->GetProperty());

    if (!m.isEmpty())
    {
      m.exec(pGroup->mapToGlobal(pt));
    }
  }
}

plQtGroupBoxBase* plQtPropertyContainerWidget::CreateElement(QWidget* pParent)
{
  auto pBox = new plQtCollapsibleGroupBox(pParent);
  //pBox->SetFillColor(palette().window().color());
  return pBox;
}

plQtPropertyWidget* plQtPropertyContainerWidget::CreateWidget(plUInt32 index)
{
  return new plQtPropertyTypeWidget();
}

plQtPropertyContainerWidget::Element& plQtPropertyContainerWidget::AddElement(plUInt32 index)
{
  plQtGroupBoxBase* pSubGroup = CreateElement(m_pGroup);
  pSubGroup->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  connect(pSubGroup, &plQtGroupBoxBase::CollapseStateChanged, m_pGrid, &plQtPropertyGridWidget::OnCollapseStateChanged);
  connect(pSubGroup, &QWidget::customContextMenuRequested, this, &plQtPropertyContainerWidget::OnCustomElementContextMenu);

  QVBoxLayout* pSubLayout = new QVBoxLayout(nullptr);
  pSubLayout->setContentsMargins(5, 0, 5, 0);
  pSubLayout->setSpacing(1);
  pSubGroup->GetContent()->setLayout(pSubLayout);

  m_pGroupLayout->insertWidget((int)index, pSubGroup);

  plQtPropertyWidget* pNewWidget = CreateWidget(index);

  pNewWidget->setParent(pSubGroup);
  pSubLayout->addWidget(pNewWidget);

  pNewWidget->Init(m_pGrid, m_pObjectAccessor, m_pType, m_pProp);

  // Add Buttons
  auto pAttr = m_pProp->GetAttributeByType<plContainerAttribute>();
  if ((!pAttr || pAttr->CanMove()) && m_pProp->GetCategory() != plPropertyCategory::Map)
  {
    pSubGroup->SetDraggable(true);
    connect(pSubGroup, &plQtGroupBoxBase::DragStarted, this, &plQtPropertyContainerWidget::OnDragStarted);
  }

  plQtElementGroupButton* pHelpButton = new plQtElementGroupButton(pSubGroup->GetHeader(), plQtElementGroupButton::ElementAction::Help, pNewWidget);
  pSubGroup->GetHeader()->layout()->addWidget(pHelpButton);
  pHelpButton->setVisible(false); // added now, and shown later when we know the URL

  if (!pAttr || pAttr->CanDelete())
  {
    plQtElementGroupButton* pDeleteButton =
      new plQtElementGroupButton(pSubGroup->GetHeader(), plQtElementGroupButton::ElementAction::DeleteElement, pNewWidget);
    pSubGroup->GetHeader()->layout()->addWidget(pDeleteButton);
    connect(pDeleteButton, &QToolButton::clicked, this, &plQtPropertyContainerWidget::OnElementButtonClicked);
  }

  m_Elements.Insert(Element(pSubGroup, pNewWidget, pHelpButton), index);
  return m_Elements[index];
}

void plQtPropertyContainerWidget::RemoveElement(plUInt32 index)
{
  Element& elem = m_Elements[index];

  m_pGroupLayout->removeWidget(elem.m_pSubGroup);
  delete elem.m_pSubGroup;
  m_Elements.RemoveAtAndCopy(index);
}

void plQtPropertyContainerWidget::UpdateElements()
{
  plQtScopedUpdatesDisabled _(this);

  plUInt32 iElements = GetRequiredElementCount();

  while (m_Elements.GetCount() > iElements)
  {
    RemoveElement(m_Elements.GetCount() - 1);
  }
  while (m_Elements.GetCount() < iElements)
  {
    AddElement(m_Elements.GetCount());
  }

  for (plUInt32 i = 0; i < iElements; ++i)
  {
    UpdateElement(i);
  }

  UpdatePropertyMetaState();

  // Force re-layout of parent hierarchy to prevent flicker.
  QWidget* pCur = m_pGroup;
  while (pCur != nullptr && qobject_cast<QScrollArea*>(pCur) == nullptr)
  {
    pCur->updateGeometry();
    pCur = pCur->parentWidget();
  }
}

plUInt32 plQtPropertyContainerWidget::GetRequiredElementCount() const
{
  if (m_pProp->GetCategory() == plPropertyCategory::Map)
  {
    m_Keys.Clear();
    PLASMA_VERIFY(m_pObjectAccessor->GetKeys(m_Items[0].m_pObject, m_pProp, m_Keys).m_Result.Succeeded(), "GetKeys should always succeed.");
    plHybridArray<plVariant, 16> keys;
    for (plUInt32 i = 1; i < m_Items.GetCount(); i++)
    {
      keys.Clear();
      PLASMA_VERIFY(m_pObjectAccessor->GetKeys(m_Items[i].m_pObject, m_pProp, keys).m_Result.Succeeded(), "GetKeys should always succeed.");
      for (plInt32 k = (plInt32)m_Keys.GetCount() - 1; k >= 0; --k)
      {
        if (!keys.Contains(m_Keys[k]))
        {
          m_Keys.RemoveAtAndSwap(k);
        }
      }
    }
    m_Keys.Sort([](const plVariant& a, const plVariant& b)
      { return a.Get<plString>().Compare(b.Get<plString>()) < 0; });
    return m_Keys.GetCount();
  }
  else
  {
    plInt32 iElements = 0x7FFFFFFF;
    for (const auto& item : m_Items)
    {
      plInt32 iCount = 0;
      PLASMA_VERIFY(m_pObjectAccessor->GetCount(item.m_pObject, m_pProp, iCount).m_Result.Succeeded(), "GetCount should always succeed.");
      iElements = plMath::Min(iElements, iCount);
    }
    PLASMA_ASSERT_DEV(iElements >= 0, "Mismatch between storage and RTTI ({0})", iElements);
    m_Keys.Clear();
    for (plUInt32 i = 0; i < (plUInt32)iElements; i++)
    {
      m_Keys.PushBack(i);
    }

    return plUInt32(iElements);
  }
}

void plQtPropertyContainerWidget::UpdatePropertyMetaState()
{
  plPropertyMetaState* pMeta = plPropertyMetaState::GetSingleton();
  plHashTable<plVariant, plPropertyUiState> ElementStates;
  pMeta->GetContainerElementsState(m_Items, m_pProp->GetPropertyName(), ElementStates);

  plDefaultContainerState defaultState(m_pObjectAccessor, m_Items, m_pProp->GetPropertyName());
  m_bIsDefault = defaultState.IsDefaultContainer();
  m_pGroup->SetBoldTitle(!m_bIsDefault);

  QColor qColor = plQtPropertyWidget::SetPaletteBackgroundColor(defaultState.GetBackgroundColor(), m_Pal);
  setPalette(m_Pal);

  const bool bReadOnly = m_pProp->GetFlags().IsSet(plPropertyFlags::ReadOnly) ||
                         (m_pProp->GetAttributeByType<plReadOnlyAttribute>() != nullptr);
  for (plUInt32 i = 0; i < m_Elements.GetCount(); i++)
  {
    Element& element = m_Elements[i];
    plVariant& key = m_Keys[i];
    const bool bIsDefault = defaultState.IsDefaultElement(key);
    auto itData = ElementStates.Find(key);
    plPropertyUiState::Visibility state = plPropertyUiState::Default;
    if (itData.IsValid())
    {
      state = itData.Value().m_Visibility;
    }

    if (element.m_pSubGroup)
    {
      element.m_pSubGroup->setVisible(state != plPropertyUiState::Invisible);
      element.m_pSubGroup->setEnabled(!bReadOnly && state != plPropertyUiState::Disabled);
      element.m_pSubGroup->SetBoldTitle(!bIsDefault);

      // // If the fill color is invalid that means no border is drawn and we don't want to change the color then.
      if (!element.m_pSubGroup->GetFillColor().isValid())
      {
        element.m_pSubGroup->SetFillColor(Qt::transparent);
      }
    }
    if (element.m_pWidget)
    {
      element.m_pWidget->setVisible(state != plPropertyUiState::Invisible);
      element.m_pSubGroup->setEnabled(!bReadOnly && state != plPropertyUiState::Disabled);
      element.m_pWidget->SetIsDefault(bIsDefault);
    }
  }
}

void plQtPropertyContainerWidget::Clear()
{
  while (m_Elements.GetCount() > 0)
  {
    RemoveElement(m_Elements.GetCount() - 1);
  }

  m_Elements.Clear();
}

void plQtPropertyContainerWidget::OnInit()
{
  plStringBuilder fullname(m_pType->GetTypeName(), "::", m_pProp->GetPropertyName());

  m_pGroup->SetTitle(plTranslate(fullname));

  const plContainerAttribute* pArrayAttr = m_pProp->GetAttributeByType<plContainerAttribute>();
  if (!pArrayAttr || pArrayAttr->CanAdd())
  {
    m_pAddButton = new plQtAddSubElementButton();
    m_pAddButton->Init(m_pGrid, m_pObjectAccessor, m_pType, m_pProp);
    m_pGroup->GetHeader()->layout()->addWidget(m_pAddButton);
  }

  m_pGrid->SetCollapseState(m_pGroup);
  connect(m_pGroup, &plQtGroupBoxBase::CollapseStateChanged, m_pGrid, &plQtPropertyGridWidget::OnCollapseStateChanged);
}

void plQtPropertyContainerWidget::DeleteItems(plHybridArray<plPropertySelection, 8>& items)
{
  m_pObjectAccessor->StartTransaction("Delete Object");

  plStatus res(PLASMA_SUCCESS);
  const bool bIsValueType = plReflectionUtils::IsValueType(m_pProp);

  if (bIsValueType)
  {
    for (auto& item : items)
    {
      res = m_pObjectAccessor->RemoveValue(item.m_pObject, m_pProp, item.m_Index);
      if (res.m_Result.Failed())
        break;
    }
  }
  else
  {
    plRemoveObjectCommand cmd;

    for (auto& item : items)
    {
      plUuid value = m_pObjectAccessor->Get<plUuid>(item.m_pObject, m_pProp, item.m_Index);
      const plDocumentObject* pObject = m_pObjectAccessor->GetObject(value);
      res = m_pObjectAccessor->RemoveObject(pObject);
      if (res.m_Result.Failed())
        break;
    }
  }

  if (res.m_Result.Failed())
    m_pObjectAccessor->CancelTransaction();
  else
    m_pObjectAccessor->FinishTransaction();

  plQtUiServices::GetSingleton()->MessageBoxStatus(res, "Removing sub-element from the property failed.");
}

void plQtPropertyContainerWidget::MoveItems(plHybridArray<plPropertySelection, 8>& items, plInt32 iMove)
{
  PLASMA_ASSERT_DEV(m_pProp->GetCategory() != plPropertyCategory::Map, "Map entries can't be moved.");

  m_pObjectAccessor->StartTransaction("Reparent Object");

  plStatus res(PLASMA_SUCCESS);
  const bool bIsValueType = plReflectionUtils::IsValueType(m_pProp);
  if (bIsValueType)
  {
    for (auto& item : items)
    {
      plInt32 iCurIndex = item.m_Index.ConvertTo<plInt32>() + iMove;
      if (iCurIndex < 0 || iCurIndex > m_pObjectAccessor->GetCount(item.m_pObject, m_pProp))
        continue;

      res = m_pObjectAccessor->MoveValue(item.m_pObject, m_pProp, item.m_Index, iCurIndex);
      if (res.m_Result.Failed())
        break;
    }
  }
  else
  {
    plMoveObjectCommand cmd;

    for (auto& item : items)
    {
      plInt32 iCurIndex = item.m_Index.ConvertTo<plInt32>() + iMove;
      if (iCurIndex < 0 || iCurIndex > m_pObjectAccessor->GetCount(item.m_pObject, m_pProp))
        continue;

      plUuid value = m_pObjectAccessor->Get<plUuid>(item.m_pObject, m_pProp, item.m_Index);
      const plDocumentObject* pObject = m_pObjectAccessor->GetObject(value);

      res = m_pObjectAccessor->MoveObject(pObject, item.m_pObject, m_pProp, iCurIndex);
      if (res.m_Result.Failed())
        break;
    }
  }

  if (res.m_Result.Failed())
    m_pObjectAccessor->CancelTransaction();
  else
    m_pObjectAccessor->FinishTransaction();

  plQtUiServices::GetSingleton()->MessageBoxStatus(res, "Moving sub-element failed.");
}


/// *** plQtPropertyStandardTypeContainerWidget ***

plQtPropertyStandardTypeContainerWidget::plQtPropertyStandardTypeContainerWidget()
  : plQtPropertyContainerWidget()
{
}

plQtPropertyStandardTypeContainerWidget::~plQtPropertyStandardTypeContainerWidget() {}

plQtGroupBoxBase* plQtPropertyStandardTypeContainerWidget::CreateElement(QWidget* pParent)
{
  auto* pBox = new plQtInlinedGroupBox(pParent);
  pBox->SetFillColor(QColor::Invalid);
  return pBox;
}


plQtPropertyWidget* plQtPropertyStandardTypeContainerWidget::CreateWidget(plUInt32 index)
{
  return plQtPropertyGridWidget::CreateMemberPropertyWidget(m_pProp);
}

plQtPropertyContainerWidget::Element& plQtPropertyStandardTypeContainerWidget::AddElement(plUInt32 index)
{
  plQtPropertyContainerWidget::Element& elem = plQtPropertyContainerWidget::AddElement(index);
  return elem;
}

void plQtPropertyStandardTypeContainerWidget::RemoveElement(plUInt32 index)
{
  plQtPropertyContainerWidget::RemoveElement(index);
}

void plQtPropertyStandardTypeContainerWidget::UpdateElement(plUInt32 index)
{
  Element& elem = m_Elements[index];

  plHybridArray<plPropertySelection, 8> SubItems;

  for (const auto& item : m_Items)
  {
    plPropertySelection sel;
    sel.m_pObject = item.m_pObject;
    sel.m_Index = m_Keys[index];

    SubItems.PushBack(sel);
  }

  plStringBuilder sTitle;
  if (m_pProp->GetCategory() == plPropertyCategory::Map)
    sTitle.Format("{0}", m_Keys[index].ConvertTo<plString>());
  else
    sTitle.Format("[{0}]", m_Keys[index].ConvertTo<plString>());

  elem.m_pSubGroup->SetTitle(sTitle);
  m_pGrid->SetCollapseState(elem.m_pSubGroup);
  elem.m_pWidget->SetSelection(SubItems);
}

/// *** plQtPropertyTypeContainerWidget ***

plQtPropertyTypeContainerWidget::plQtPropertyTypeContainerWidget()
  : m_bNeedsUpdate(false)
{
}

plQtPropertyTypeContainerWidget::~plQtPropertyTypeContainerWidget()
{
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(
    plMakeDelegate(&plQtPropertyTypeContainerWidget::StructureEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtPropertyTypeContainerWidget::CommandHistoryEventHandler, this));
}

void plQtPropertyTypeContainerWidget::OnInit()
{
  plQtPropertyContainerWidget::OnInit();
  m_pGrid->GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(
    plMakeDelegate(&plQtPropertyTypeContainerWidget::StructureEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.AddEventHandler(plMakeDelegate(&plQtPropertyTypeContainerWidget::CommandHistoryEventHandler, this));
}

void plQtPropertyTypeContainerWidget::UpdateElement(plUInt32 index)
{
  Element& elem = m_Elements[index];
  plHybridArray<plPropertySelection, 8> SubItems;

  // To be in line with all other plQtPropertyWidget the container element will
  // be given a selection in the form of this is the parent object, this is the property and in this
  // specific case this is the index you are working on. So SubItems only decorates the items with the correct index.
  for (const auto& item : m_Items)
  {
    plPropertySelection sel;
    sel.m_pObject = item.m_pObject;
    sel.m_Index = m_Keys[index];

    SubItems.PushBack(sel);
  }

  {
    // To get the correct name we actually need to resolve the selection to the actual objects
    // they are pointing to.
    plHybridArray<plPropertySelection, 8> ResolvedObjects;
    for (const auto& item : SubItems)
    {
      plUuid ObjectGuid = m_pObjectAccessor->Get<plUuid>(item.m_pObject, m_pProp, item.m_Index);
      plPropertySelection sel;
      sel.m_pObject = m_pObjectAccessor->GetObject(ObjectGuid);
      ResolvedObjects.PushBack(sel);
    }

    const plRTTI* pCommonType = plQtPropertyWidget::GetCommonBaseType(ResolvedObjects);

    // Label
    {
      plStringBuilder sTitle, tmp;
      sTitle.Format("{0}", plTranslate(pCommonType->GetTypeName().GetData(tmp)));

      if (auto pInDev = pCommonType->GetAttributeByType<plInDevelopmentAttribute>())
      {
        sTitle.AppendFormat(" [ {} ]", pInDev->GetString());
      }

      elem.m_pSubGroup->SetTitle(sTitle);
    }

    plColor borderIconColor = plColor::ZeroColor();

    if (const plColorAttribute* pColorAttrib = pCommonType->GetAttributeByType<plColorAttribute>())
    {
      borderIconColor = pColorAttrib->GetColor();
      elem.m_pSubGroup->SetFillColor(plToQtColor(pColorAttrib->GetColor()));
    }
    else if (const plCategoryAttribute* pCatAttrib = pCommonType->GetAttributeByType<plCategoryAttribute>())
    {
      borderIconColor = plColorScheme::GetCategoryColor(pCatAttrib->GetCategory(), plColorScheme::CategoryColorUsage::BorderIconColor);
      elem.m_pSubGroup->SetFillColor(plToQtColor(plColorScheme::GetCategoryColor(pCatAttrib->GetCategory(), plColorScheme::CategoryColorUsage::BorderColor)));
    }
    else
    {
      const QPalette& pal = palette();
      elem.m_pSubGroup->SetFillColor(pal.mid().color());
    }

    // Icon
    {
      plStringBuilder sIconName;
      sIconName.Set(":/TypeIcons/", pCommonType->GetTypeName(), ".svg");
      elem.m_pSubGroup->SetIcon(plQtUiServices::GetCachedIconResource(sIconName.GetData()));
    }


    // help URL
    {
      plStringBuilder tmp;
      QString url = plTranslateHelpURL(pCommonType->GetTypeName().GetData(tmp));


      if (!url.isEmpty())
      {
        elem.m_pHelpButton->setVisible(true);
        connect(elem.m_pHelpButton, &QToolButton::clicked, this, [=]()
          { QDesktopServices::openUrl(QUrl(url)); });
      }
      else
      {
        elem.m_pHelpButton->setVisible(false);
      }
    }
  }


  m_pGrid->SetCollapseState(elem.m_pSubGroup);
  elem.m_pWidget->SetSelection(SubItems);
}

void plQtPropertyTypeContainerWidget::StructureEventHandler(const plDocumentObjectStructureEvent& e)
{
  if (IsUndead())
    return;

  switch (e.m_EventType)
  {
    case plDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case plDocumentObjectStructureEvent::Type::AfterObjectMoved:
    case plDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      if (!e.m_sParentProperty.IsEqual(m_pProp->GetPropertyName()))
        return;

      if (std::none_of(cbegin(m_Items), cend(m_Items),
            [&](const plPropertySelection& sel)
            { return e.m_pNewParent == sel.m_pObject || e.m_pPreviousParent == sel.m_pObject; }))
        return;

      m_bNeedsUpdate = true;
    }
    break;
    default:
      break;
  }
}

void plQtPropertyTypeContainerWidget::CommandHistoryEventHandler(const plCommandHistoryEvent& e)
{
  if (IsUndead())
    return;

  switch (e.m_Type)
  {
    case plCommandHistoryEvent::Type::UndoEnded:
    case plCommandHistoryEvent::Type::RedoEnded:
    case plCommandHistoryEvent::Type::TransactionEnded:
    case plCommandHistoryEvent::Type::TransactionCanceled:
    {
      if (m_bNeedsUpdate)
      {
        m_bNeedsUpdate = false;
        UpdateElements();
      }
    }
    break;

    default:
      break;
  }
}

/// *** plQtVariantPropertyWidget ***

plQtVariantPropertyWidget::plQtVariantPropertyWidget()
{
  m_pLayout = new QVBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 4);
  m_pLayout->setSpacing(1);
  setLayout(m_pLayout);

  m_pTypeList = new QComboBox(this);
  m_pTypeList->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pLayout->addWidget(m_pTypeList);
}

plQtVariantPropertyWidget::~plQtVariantPropertyWidget() = default;

void plQtVariantPropertyWidget::OnInit()
{
  plStringBuilder sName;
  for (int i = plVariantType::Invalid; i < plVariantType::LastExtendedType; ++i)
  {
    auto type = static_cast<plVariantType::Enum>(i);
    if (GetVariantTypeDisplayName(type, sName).Succeeded())
    {
      m_pTypeList->addItem(plTranslate(sName), i);
    }
  }

  connect(m_pTypeList, &QComboBox::currentIndexChanged,
    [this](int index)
    {
      ChangeVariantType(static_cast<plVariantType::Enum>(m_pTypeList->itemData(index).toInt()));
    });
}

void plQtVariantPropertyWidget::InternalSetValue(const plVariant& value)
{
  plVariantType::Enum commonType = plVariantType::Invalid;
  const bool sameType = GetCommonVariantSubType(m_Items, m_pProp, commonType);
  const plRTTI* pNewtSubType = commonType != plVariantType::Invalid ? plReflectionUtils::GetTypeFromVariant(commonType) : nullptr;
  if (pNewtSubType != m_pCurrentSubType || m_pWidget == nullptr)
  {
    if (m_pWidget)
    {
      m_pWidget->PrepareToDie();
      m_pWidget->deleteLater();
      m_pWidget = nullptr;
    }
    m_pCurrentSubType = pNewtSubType;
    if (pNewtSubType)
    {
      m_pWidget = plQtPropertyGridWidget::GetFactory().CreateObject(pNewtSubType);
      if (!m_pWidget)
        m_pWidget = new plQtUnsupportedPropertyWidget("Unsupported type");
    }
    else if (!sameType)
    {
      m_pWidget = new plQtUnsupportedPropertyWidget("Multi-selection has varying types");
    }
    else
    {
      m_pWidget = new plQtUnsupportedPropertyWidget("<Invalid>");
    }
    m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_pWidget->setParent(this);
    m_pLayout->addWidget(m_pWidget);
    m_pWidget->Init(m_pGrid, m_pObjectAccessor, m_pType, m_pProp);

    UpdateTypeListSelection(commonType);
  }
  m_pWidget->SetSelection(m_Items);
}

void plQtVariantPropertyWidget::DoPrepareToDie()
{
  if (m_pWidget)
    m_pWidget->PrepareToDie();
}

void plQtVariantPropertyWidget::UpdateTypeListSelection(plVariantType::Enum type)
{
  plQtScopedBlockSignals bs(m_pTypeList);
  for (int i = 0; i < m_pTypeList->count(); ++i)
  {
    if (m_pTypeList->itemData(i).toInt() == type)
    {
      m_pTypeList->setCurrentIndex(i);
      break;
    }
  }
}

void plQtVariantPropertyWidget::ChangeVariantType(plVariantType::Enum type)
{
  m_pObjectAccessor->StartTransaction("Change variant type");
  // check if we have multiple values
  for (const auto& item : m_Items)
  {
    plVariant value;
    PLASMA_VERIFY(m_pObjectAccessor->GetValue(item.m_pObject, m_pProp, value, item.m_Index).Succeeded(), "");
    if (value.CanConvertTo(type))
    {
      PLASMA_VERIFY(m_pObjectAccessor->SetValue(item.m_pObject, m_pProp, value.ConvertTo(type), item.m_Index).Succeeded(), "");
    }
    else
    {
      PLASMA_VERIFY(
        m_pObjectAccessor->SetValue(item.m_pObject, m_pProp, plReflectionUtils::GetDefaultVariantFromType(type), item.m_Index).Succeeded(), "");
    }
  }
  m_pObjectAccessor->FinishTransaction();
}

plResult plQtVariantPropertyWidget::GetVariantTypeDisplayName(plVariantType::Enum type, plStringBuilder& out_sName) const
{
  if (type == plVariantType::FirstStandardType || type >= plVariantType::LastStandardType ||
      type == plVariantType::StringView || type == plVariantType::DataBuffer || type == plVariantType::TempHashedString)
    return PLASMA_FAILURE;

  const plRTTI* pVariantEnum = plGetStaticRTTI<plVariantType>();
  if (plReflectionUtils::EnumerationToString(pVariantEnum, type, out_sName) == false)
    return PLASMA_FAILURE;

  return PLASMA_SUCCESS;
}
