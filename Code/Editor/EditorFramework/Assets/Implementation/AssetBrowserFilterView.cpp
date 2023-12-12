#include "EditorFramework/Assets/AssetBrowserFilterView.moc.h"

plQtAssetBrowserFilterView::plQtAssetBrowserFilterView(QWidget* parent)
{
  setupUi(this);
}

plQtAssetBrowserFilterView::~plQtAssetBrowserFilterView()
{
}

void plQtAssetBrowserFilterView::AddPart(QTreeWidgetItem *node)
{
  plStringBuilder namestr = node->text(0).toUtf8().data();
  plStringBuilder pathTo = namestr;
  QTreeWidgetItem* parnode = node->parent();
  while (parnode && parnode->parent())  //we dont prepend the name of root
  {
    pathTo.PrependFormat("{}/", parnode->text(0).toUtf8().data());
    parnode = parnode->parent();
  }

  QMenu* menu = new QMenu();
  for (int i = 0; i < node->childCount(); i++)
  {
    
    QTreeWidgetItem* childnode = node->child(i);
    QAction* act = new QAction();
    plStringBuilder pathToChild = pathTo;
    pathToChild.AppendFormat("/{}", childnode->text(0).toUtf8().data());
    act->setData(QVariant(childnode->data(0, plQtAssetBrowserModel::UserRoles::AbsolutePath))); // stores rel path data as it might be different from displayed name i.e :root
    act->setText(childnode->text(0).toUtf8().data());
    PLASMA_VERIFY(connect(act, &QAction::triggered, this, [this, pathToChild]
                    { m_pFilter->SetPathFilter(pathToChild); }) != nullptr, "failed to connect signal/slot");
    menu->addAction(act);
  }

  QAction* act = new QAction();
  act->setData(QVariant(node->data(0, plQtAssetBrowserModel::UserRoles::AbsolutePath))); // stores rel path data as it might be differrent from displayed name i.e :root
  act->setText(namestr.GetData());
  connect(act, &QAction::triggered, this, [this, pathTo]
    { m_pFilter->SetPathFilter(pathTo); });

  QToolButton* toolButton = new QToolButton(this);
  toolButton->setObjectName(node->data(0, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
  if (node->childCount() > 0)
  {
    toolButton->setMenu(menu);
    toolButton->setPopupMode(QToolButton::MenuButtonPopup);
  }
  else  //if no menu we dont display the arrow
  {
    toolButton->setPopupMode(QToolButton::InstantPopup);
  }

  toolButton->setDefaultAction(act);
  toolButton->setAutoRaise(true);

  horizontalLayout->insertWidget(m_pParts.GetCount(), toolButton);
  m_pParts.PushBack(toolButton);
}

void plQtAssetBrowserFilterView::AddRoot()
{
  QTreeWidgetItem* root = TreeFolderFilter->topLevelItem(0);

  if (!root)
  {
    PLASMA_ASSERT_DEV(false, "Not sure how we got here");
    return;
  }

  //build menu, actions going to children folders
  QMenu* menu = new QMenu();
  for (int i = 0; i < root->childCount(); i++)
  {
    QTreeWidgetItem* childnode = root->child(i);
    QAction* act = new QAction();
    act->setData(QVariant(childnode->data(0, plQtAssetBrowserModel::UserRoles::AbsolutePath))); // stores rel path data as it might be different from displayed name i.e :root
    act->setText(childnode->text(0).toUtf8().data());
    connect(act, &QAction::triggered, this, [this, act]
      { m_pFilter->SetPathFilter(act->text().toUtf8().data()); });
    menu->addAction(act);
  }

  //build action, going back to the folder
  QAction* act = new QAction();
  act->setData(QVariant(root->data(0, plQtAssetBrowserModel::UserRoles::AbsolutePath))); // stores rel path data as it might be differrent from displayed name i.e :root
  act->setText(root->text(0).toUtf8().data());
  connect(act, &QAction::triggered, this, [this]
    { m_pFilter->SetPathFilter(""); });


  //builds the button itself
  QToolButton* toolButton = new QToolButton(this);
  toolButton->setObjectName(root->data(0, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString());
  if (root->childCount() > 0)
  {
    toolButton->setMenu(menu);
    toolButton->setPopupMode(QToolButton::MenuButtonPopup);
  }
  else // if no menu we dont display the arrow
  {
    toolButton->setPopupMode(QToolButton::InstantPopup);
  }

  toolButton->setDefaultAction(act);
  toolButton->setAutoRaise(true); //this option is by true by default for toolbuttons in toolbar but those are in a widget

  horizontalLayout->insertWidget(m_pParts.GetCount(), toolButton);
  m_pParts.PushBack(toolButton);
}


void plQtAssetBrowserFilterView::SetFilter(plQtAssetBrowserFilter* filter)
{
  m_pFilter = filter;
  PLASMA_VERIFY(connect(m_pFilter, SIGNAL(PathFilterChanged()), this, SLOT(Reset())) != nullptr, "signal/slot connection failed");
}

void plQtAssetBrowserFilterView::Reset()
{

  while (!m_pParts.IsEmpty())
  {
    QWidget* widget = m_pParts.PeekBack();
    horizontalLayout->removeWidget(widget);
    m_pParts.PopBack();
    delete widget;  //otherwise they can still display
  }

  AddRoot();

  //split the filter path
  plString filterPath = m_pFilter->GetPathFilter();
  plDynamicArray<plStringView> compTypes;
  filterPath.Split(false, compTypes, "/");

  QTreeWidgetItem* parNode = TreeFolderFilter->topLevelItem(0);

  for (int i = 0; i < compTypes.GetCount(); i++)
  {
    plStringBuilder str;
    compTypes[i].GetData(str);

    if (parNode->childCount() > 0)
    {
      bool bFound = false;
      for (int j = 0; !bFound && j < parNode->childCount(); j++)
      {
        QTreeWidgetItem* node = parNode->child(j);
        if (node->text(0) == str)
        {
          bFound = true;
          AddPart(node);
          parNode = node;
        }
      }
      if (!bFound)
      {
        plLog::Error("Failed to find child node '{}' for parent '{}'", str, parNode->text(0).toUtf8());
        return;
      }
    }
    else
    {
      plLog::Error("Filter depth not matching folder tree depth");
    }
  }
}
