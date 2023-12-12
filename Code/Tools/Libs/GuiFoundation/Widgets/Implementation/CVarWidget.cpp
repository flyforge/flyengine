#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/CVarWidget.moc.h>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

plQtCVarWidget::plQtCVarWidget(QWidget* parent)
  : QWidget(parent)
{
  setupUi(this);

  m_pItemModel = new plQtCVarModel(this);

  m_pFilterModel = new QSortFilterProxyModel(this);
  m_pFilterModel->setSourceModel(m_pItemModel);

  m_pItemDelegate = new plQtCVarItemDelegate(this);
  m_pItemDelegate->m_pModel = m_pItemModel;

  CVarsView->setModel(m_pFilterModel);
  CVarsView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  CVarsView->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  CVarsView->setHeaderHidden(false);
  CVarsView->setEditTriggers(QAbstractItemView::EditTrigger::CurrentChanged | QAbstractItemView::EditTrigger::SelectedClicked);
  CVarsView->setItemDelegateForColumn(1, m_pItemDelegate);

  connect(SearchWidget, &plQtSearchWidget::textChanged, this, &plQtCVarWidget::SearchTextChanged);
  connect(ConsoleInput, &plQtSearchWidget::enterPressed, this, &plQtCVarWidget::ConsoleEnterPressed);
  connect(ConsoleInput, &plQtSearchWidget::specialKeyPressed, this, &plQtCVarWidget::ConsoleSpecialKeyPressed);

  m_Console.Events().AddEventHandler(plMakeDelegate(&plQtCVarWidget::OnConsoleEvent, this));

  ConsoleInput->setPlaceholderText("> TAB to auto-complete");
}

plQtCVarWidget::~plQtCVarWidget() {}

void plQtCVarWidget::Clear()
{
  QPointer<QWidget> pFocusWidget = QApplication::focusWidget();
  clearFocus();
  m_pItemModel->BeginResetModel();
  m_pItemModel->EndResetModel();

  if (pFocusWidget)
    pFocusWidget->setFocus();
}

void plQtCVarWidget::RebuildCVarUI(const plMap<plString, plCVarWidgetData>& cvars)
{
  // for now update and rebuild are the same
  UpdateCVarUI(cvars);
}

void plQtCVarWidget::UpdateCVarUI(const plMap<plString, plCVarWidgetData>& cvars)
{
  int row = 0;

  m_pItemModel->BeginResetModel();

  for (auto it = cvars.GetIterator(); it.IsValid(); ++it, ++row)
  {
    it.Value().m_bNewEntry = false;

    auto item = m_pItemModel->CreateEntry(it.Key().GetData());
    item->m_sDescription = it.Value().m_sDescription;
    item->m_sPlugin = it.Value().m_sPlugin;

    switch (it.Value().m_uiType)
    {
      case plCVarType::Bool:
        item->m_Value = it.Value().m_bValue;
        break;
      case plCVarType::Float:
        item->m_Value = it.Value().m_fValue;
        break;
      case plCVarType::Int:
        item->m_Value = it.Value().m_iValue;
        break;
      case plCVarType::String:
        item->m_Value = it.Value().m_sValue;
        break;
    }
  }

  m_pItemModel->EndResetModel();

  CVarsView->expandAll();
  CVarsView->resizeColumnToContents(0);
  CVarsView->resizeColumnToContents(1);
}

void plQtCVarWidget::AddConsoleStrings(const plStringBuilder& encoded)
{
  plHybridArray<plStringView, 64> lines;
  encoded.Split(false, lines, ";;");

  plStringBuilder tmp;

  for (auto l : lines)
  {
    l.Shrink(4, 0); // skip the line type number at the front (for now)

    if (l.StartsWith("<"))
    {
      l.Shrink(1, 0);
      ConsoleInput->setText(l.GetData(tmp));
    }
    else
    {
      GetConsole().AddConsoleString(l);
    }
  }
}

void plQtCVarWidget::SearchTextChanged(const QString& text)
{
  m_pFilterModel->setRecursiveFilteringEnabled(true);
  m_pFilterModel->setFilterRole(Qt::UserRole);
  m_pFilterModel->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);

  QRegularExpression e;

  QString st = "(?=.*" + text + ".*)";
  st.replace(" ", ".*)(?=.*");

  e.setPattern(st);
  e.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
  m_pFilterModel->setFilterRegularExpression(e);
  CVarsView->expandAll();
}

void plQtCVarWidget::ConsoleEnterPressed()
{
  m_Console.AddToInputHistory(ConsoleInput->text().toUtf8().data());
  m_Console.ExecuteCommand(ConsoleInput->text().toUtf8().data());
  ConsoleInput->setText("");
}

void plQtCVarWidget::ConsoleSpecialKeyPressed(Qt::Key key)
{
  if (key == Qt::Key_Tab)
  {
    plStringBuilder input = ConsoleInput->text().toUtf8().data();

    if (m_Console.AutoComplete(input))
    {
      ConsoleInput->setText(input.GetData());
    }
  }
  if (key == Qt::Key_Up)
  {
    plStringBuilder input = ConsoleInput->text().toUtf8().data();
    m_Console.RetrieveInputHistory(1, input);
    ConsoleInput->setText(input.GetData());
  }
  if (key == Qt::Key_Down)
  {
    plStringBuilder input = ConsoleInput->text().toUtf8().data();
    m_Console.RetrieveInputHistory(-1, input);
    ConsoleInput->setText(input.GetData());
  }
  if (key == Qt::Key_F2)
  {
    if (m_Console.GetInputHistory().GetCount() >= 1)
    {
      m_Console.ExecuteCommand(m_Console.GetInputHistory()[0]);
    }
  }
  if (key == Qt::Key_F3)
  {
    if (m_Console.GetInputHistory().GetCount() >= 2)
    {
      m_Console.ExecuteCommand(m_Console.GetInputHistory()[1]);
    }
  }
}

void plQtCVarWidget::OnConsoleEvent(const plConsoleEvent& e)
{
  if (e.m_Type == plConsoleEvent::Type::OutputLineAdded)
  {
    QString t = ConsoleOutput->toPlainText();
    t += e.m_AddedpConsoleString->m_sText;
    t += "\n";
    ConsoleOutput->setPlainText(t);
    ConsoleOutput->verticalScrollBar()->setValue(ConsoleOutput->verticalScrollBar()->maximum());
  }
}

plQtCVarModel::plQtCVarModel(plQtCVarWidget* owner)
  : QAbstractItemModel(owner)
{
  m_pOwner = owner;
}

plQtCVarModel::~plQtCVarModel() = default;

void plQtCVarModel::BeginResetModel()
{
  beginResetModel();
  m_RootEntries.Clear();
  m_AllEntries.Clear();
}

void plQtCVarModel::EndResetModel()
{
  endResetModel();
}

QVariant plQtCVarModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
  if (role == Qt::DisplayRole)
  {
    switch (section)
    {
      case 0:
        return "Name";

      case 1:
        return "Value";

      case 2:
        return "Description";

      case 3:
        return "Plugin";
    }
  }

  return QAbstractItemModel::headerData(section, orientation, role);
}

bool plQtCVarModel::setData(const QModelIndex& index, const QVariant& value, int role /*= Qt::EditRole*/)
{
  if (index.column() == 1 && role == Qt::EditRole)
  {
    plQtCVarModel::Entry* e = reinterpret_cast<plQtCVarModel::Entry*>(index.internalId());

    switch (e->m_Value.GetType())
    {
      case plVariantType::Bool:
        e->m_Value = value.toBool();
        m_pOwner->onBoolChanged(e->m_sFullName, value.toBool());
        break;
      case plVariantType::Int32:
        e->m_Value = value.toInt();
        m_pOwner->onIntChanged(e->m_sFullName, value.toInt());
        break;
      case plVariantType::Float:
        e->m_Value = value.toFloat();
        m_pOwner->onFloatChanged(e->m_sFullName, value.toFloat());
        break;
      case plVariantType::String:
        e->m_Value = value.toString().toUtf8().data();
        m_pOwner->onStringChanged(e->m_sFullName, value.toString().toUtf8().data());
        break;
      default:
        break;
    }
  }

  return QAbstractItemModel::setData(index, value, role);
}

QVariant plQtCVarModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid())
    return QVariant();

  plQtCVarModel::Entry* e = reinterpret_cast<plQtCVarModel::Entry*>(index.internalId());

  if (role == Qt::UserRole)
  {
    return e->m_sFullName.GetData();
  }

  if (role == Qt::DisplayRole)
  {
    switch (index.column())
    {
      case 0:
        return e->m_sDisplayString;

      case 1:
        return e->m_Value.ConvertTo<plString>().GetData();

      case 2:
        return e->m_sDescription;
    }
  }

  if (role == Qt::DecorationRole && index.column() == 0)
  {
    if (e->m_Value.IsValid())
    {
      return plQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/CVar.svg");
    }
  }

  if (role == Qt::ToolTipRole)
  {
    if (e->m_Value.IsValid())
    {
      if (index.column() == 0)
      {
        return QString(e->m_sFullName) + " | " + e->m_sPlugin;
      }

      if (index.column() == 2)
      {
        return e->m_sDescription;
      }
    }
  }

  if (role == Qt::EditRole && index.column() == 1)
  {
    switch (e->m_Value.GetType())
    {
      case plVariantType::Bool:
        return e->m_Value.Get<bool>();
      case plVariantType::Int32:
        return e->m_Value.Get<plInt32>();
      case plVariantType::Float:
        return e->m_Value.ConvertTo<double>();
      case plVariantType::String:
        return e->m_Value.Get<plString>().GetData();
      default:
        break;
    }
  }
  return QVariant();
}

Qt::ItemFlags plQtCVarModel::flags(const QModelIndex& index) const
{
  if (index.column() == 1)
  {
    plQtCVarModel::Entry* e = reinterpret_cast<plQtCVarModel::Entry*>(index.internalId());

    if (e->m_Value.IsValid())
    {
      return Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsEditable;
    }
  }

  return Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsEnabled;
}

QModelIndex plQtCVarModel::index(int row, int column, const QModelIndex& parent /*= QModelIndex()*/) const
{
  if (parent.isValid())
  {
    plQtCVarModel::Entry* e = reinterpret_cast<plQtCVarModel::Entry*>(parent.internalId());
    return createIndex(row, column, const_cast<plQtCVarModel::Entry*>(e->m_ChildEntries[row]));
  }
  else
  {
    return createIndex(row, column, const_cast<plQtCVarModel::Entry*>(m_RootEntries[row]));
  }
}

QModelIndex plQtCVarModel::parent(const QModelIndex& index) const
{
  if (!index.isValid())
    return QModelIndex();

  plQtCVarModel::Entry* e = reinterpret_cast<plQtCVarModel::Entry*>(index.internalId());

  if (e->m_pParentEntry == nullptr)
    return QModelIndex();

  plQtCVarModel::Entry* p = e->m_pParentEntry;

  // find the parent entry's row index
  if (p->m_pParentEntry == nullptr)
  {
    // if the parent has no parent itself, it is a root entry and we need to search that array
    for (plUInt32 row = 0; row < m_RootEntries.GetCount(); ++row)
    {
      if (m_RootEntries[row] == p)
      {
        return createIndex(row, index.column(), p);
      }
    }
  }
  else
  {
    // if the parent has a parent itself, search that array for the row index
    for (plUInt32 row = 0; row < p->m_pParentEntry->m_ChildEntries.GetCount(); ++row)
    {
      if (p->m_pParentEntry->m_ChildEntries[row] == e)
      {
        return createIndex(row, index.column(), p);
      }
    }
  }

  return QModelIndex();
}

int plQtCVarModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  if (parent.isValid())
  {
    plQtCVarModel::Entry* e = reinterpret_cast<plQtCVarModel::Entry*>(parent.internalId());

    return (int)e->m_ChildEntries.GetCount();
  }
  else
  {
    return (int)m_RootEntries.GetCount();
  }
}

int plQtCVarModel::columnCount(const QModelIndex& index /*= QModelIndex()*/) const
{
  return 3;
}

plQtCVarModel::Entry* plQtCVarModel::CreateEntry(const char* name)
{
  plStringBuilder tmp = name;
  plStringBuilder tmp2;

  plHybridArray<plStringView, 8> pieces;
  tmp.Split(false, pieces, ".", "_");

  plDynamicArray<Entry*>* vals = &m_RootEntries;
  Entry* parentEntry = nullptr;

  for (plUInt32 p = 0; p < pieces.GetCount(); ++p)
  {
    QString piece = pieces[p].GetData(tmp2);
    for (plUInt32 v = 0; v < vals->GetCount(); ++v)
    {
      if ((*vals)[v]->m_sDisplayString == piece)
      {
        parentEntry = (*vals)[v];
        vals = &((*vals)[v]->m_ChildEntries);
        goto found;
      }
    }

    {
      auto& newItem = m_AllEntries.ExpandAndGetRef();
      newItem.m_sFullName = name;
      newItem.m_sDisplayString = piece;
      newItem.m_pParentEntry = parentEntry;

      vals->PushBack(&newItem);

      parentEntry = &newItem;
      vals = &newItem.m_ChildEntries;
    }
  found:;
  }

  return parentEntry;
}

QWidget* plQtCVarItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& idx) const
{
  m_Index = static_cast<const QSortFilterProxyModel*>(idx.model())->mapToSource(idx);
  plQtCVarModel::Entry* e = reinterpret_cast<plQtCVarModel::Entry*>(m_Index.internalPointer());

  if (!e->m_Value.IsValid())
    return nullptr;

  if (e->m_Value.IsA<bool>())
  {
    QComboBox* ret = new QComboBox(parent);
    ret->addItem("true");
    ret->addItem("false");

    connect(ret, SIGNAL(currentIndexChanged(int)), this, SLOT(onComboChanged(int)));
    return ret;
  }

  if (e->m_Value.IsA<plInt32>())
  {
    QLineEdit* ret = new QLineEdit(parent);
    ret->setValidator(new QIntValidator(ret));
    return ret;
  }

  if (e->m_Value.IsA<float>())
  {
    QLineEdit* ret = new QLineEdit(parent);
    auto val = new QDoubleValidator(ret);
    val->setDecimals(3);
    ret->setValidator(val);
    return ret;
  }

  if (e->m_Value.IsA<plString>())
  {
    QLineEdit* ret = new QLineEdit(parent);
    return ret;
  }

  return nullptr;
}

void plQtCVarItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
  QVariant value = index.model()->data(index, Qt::EditRole);

  if (QLineEdit* pLine = qobject_cast<QLineEdit*>(editor))
  {
    if (value.type() == QVariant::Type::Double)
    {
      double f = value.toDouble();

      pLine->setText(QString("%1").arg(f, 0, (char)103, 3));
    }
    else
    {
      pLine->setText(value.toString());
    }

    pLine->selectAll();
  }

  if (QComboBox* pLine = qobject_cast<QComboBox*>(editor))
  {
    pLine->setCurrentIndex(value.toBool() ? 0 : 1);
    pLine->showPopup();
  }
}

void plQtCVarItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
  if (QLineEdit* pLine = qobject_cast<QLineEdit*>(editor))
  {
    model->setData(index, pLine->text(), Qt::EditRole);
  }

  if (QComboBox* pLine = qobject_cast<QComboBox*>(editor))
  {
    model->setData(index, pLine->currentText(), Qt::EditRole);
  }
}

void plQtCVarItemDelegate::onComboChanged(int)
{
  setModelData(qobject_cast<QWidget*>(sender()), m_pModel, m_Index);
}
