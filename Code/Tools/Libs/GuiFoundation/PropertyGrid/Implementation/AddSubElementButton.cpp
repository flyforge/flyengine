#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenu>
#include <QPushButton>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plString plQtAddSubElementButton::s_sLastMenuSearch;
bool plQtAddSubElementButton::s_bShowInDevelopmentFeatures = false;

plQtAddSubElementButton::plQtAddSubElementButton()
  : plQtPropertyWidget()
{
  // Reset base class size policy as we are put in a layout that would cause us to vanish instead.
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pButton = new QPushButton(this);
  m_pButton->setText("Add Item");
  m_pButton->setObjectName("Button");

  QSizePolicy policy = m_pButton->sizePolicy();
  policy.setHorizontalStretch(0);
  m_pButton->setSizePolicy(policy);

  m_pLayout->addSpacerItem(new QSpacerItem(0, 0));
  m_pLayout->setStretch(0, 1);
  m_pLayout->addWidget(m_pButton);
  m_pLayout->addSpacerItem(new QSpacerItem(0, 0));
  m_pLayout->setStretch(2, 1);

  m_pMenu = nullptr;
}

void plQtAddSubElementButton::OnInit()
{
  if (m_pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
  {
    m_pMenu = new QMenu(m_pButton);
    m_pMenu->setToolTipsVisible(true);
    connect(m_pMenu, &QMenu::aboutToShow, this, &plQtAddSubElementButton::onMenuAboutToShow);
    m_pButton->setMenu(m_pMenu);
    m_pButton->setObjectName("Button");
  }

  if (const plMaxArraySizeAttribute* pAttr = m_pProp->GetAttributeByType<plMaxArraySizeAttribute>())
  {
    m_uiMaxElements = pAttr->GetMaxSize();
  }

  if (const plPreventDuplicatesAttribute* pAttr = m_pProp->GetAttributeByType<plPreventDuplicatesAttribute>())
  {
    m_bPreventDuplicates = true;
  }

  QMetaObject::connectSlotsByName(this);
}

struct TypeComparer
{
  PLASMA_FORCE_INLINE bool Less(const plRTTI* a, const plRTTI* b) const
  {
    const plCategoryAttribute* pCatA = a->GetAttributeByType<plCategoryAttribute>();
    const plCategoryAttribute* pCatB = b->GetAttributeByType<plCategoryAttribute>();
    if (pCatA != nullptr && pCatB == nullptr)
    {
      return true;
    }
    else if (pCatA == nullptr && pCatB != nullptr)
    {
      return false;
    }
    else if (pCatA != nullptr && pCatB != nullptr)
    {
      plInt32 iRes = plStringUtils::Compare(pCatA->GetCategory(), pCatB->GetCategory());
      if (iRes != 0)
      {
        return iRes < 0;
      }
    }

    return a->GetTypeName().Compare(b->GetTypeName()) < 0;
  }
};

QMenu* plQtAddSubElementButton::CreateCategoryMenu(const char* szCategory, plMap<plString, QMenu*>& existingMenus)
{
  if (plStringUtils::IsNullOrEmpty(szCategory))
    return m_pMenu;


  auto it = existingMenus.Find(szCategory);
  if (it.IsValid())
    return it.Value();

  plStringBuilder sPath = szCategory;
  sPath.PathParentDirectory();
  sPath.Trim("/");

  QMenu* pParentMenu = m_pMenu;

  if (!sPath.IsEmpty())
  {
    pParentMenu = CreateCategoryMenu(sPath, existingMenus);
  }

  sPath = szCategory;
  sPath = sPath.GetFileName();

  QMenu* pNewMenu = pParentMenu->addMenu(plTranslate(sPath.GetData()));
  existingMenus[szCategory] = pNewMenu;

  return pNewMenu;
}

void plQtAddSubElementButton::onMenuAboutToShow()
{
  if (m_Items.IsEmpty())
    return;

  if (m_pMenu->isEmpty())
  {
    auto pProp = GetProperty();

    if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
    {
      m_SupportedTypes.Clear();
      plReflectionUtils::GatherTypesDerivedFromClass(pProp->GetSpecificType(), m_SupportedTypes);
    }
    m_SupportedTypes.Insert(pProp->GetSpecificType());

    // remove all types that are marked as hidden
    for (auto it = m_SupportedTypes.GetIterator(); it.IsValid();)
    {
      if (it.Key()->GetAttributeByType<plHiddenAttribute>() != nullptr)
      {
        it = m_SupportedTypes.Remove(it);
        continue;
      }

      if (!s_bShowInDevelopmentFeatures)
      {
        if (auto pInDev = it.Key()->GetAttributeByType<plInDevelopmentAttribute>())
        {
          it = m_SupportedTypes.Remove(it);
          continue;
        }
      }

      ++it;
    }

    // Make category-sorted array of types
    plDynamicArray<const plRTTI*> supportedTypes;
    for (const plRTTI* pRtti : m_SupportedTypes)
    {
      if (pRtti->GetTypeFlags().IsAnySet(plTypeFlags::Abstract))
        continue;

      supportedTypes.PushBack(pRtti);
    }
    supportedTypes.Sort(TypeComparer());

    if (!m_bPreventDuplicates && supportedTypes.GetCount() > 10)
    {
      // only show a searchable menu when it makes some sense
      // also deactivating entries to prevent duplicates is currently not supported by the searchable menu
      m_pSearchableMenu = new plQtSearchableMenu(m_pMenu);
    }

    plStringBuilder sIconName;
    plStringBuilder sCategory = "";

    plMap<plString, QMenu*> existingMenus;

    if (m_pSearchableMenu == nullptr)
    {
      // first round: create all sub menus
      for (const plRTTI* pRtti : supportedTypes)
      {
        // Determine current menu
        const plCategoryAttribute* pCatA = pRtti->GetAttributeByType<plCategoryAttribute>();

        if (pCatA)
        {
          CreateCategoryMenu(pCatA->GetCategory(), existingMenus);
        }
      }
    }

    plStringBuilder tmp;

    // second round: create the actions
    for (const plRTTI* pRtti : supportedTypes)
    {
      sIconName.Set(":/TypeIcons/", pRtti->GetTypeName(), ".svg");

      // Determine current menu
      const plCategoryAttribute* pCatA = pRtti->GetAttributeByType<plCategoryAttribute>();
      const plInDevelopmentAttribute* pInDev = pRtti->GetAttributeByType<plInDevelopmentAttribute>();
      const plColorAttribute* pColA = pRtti->GetAttributeByType<plColorAttribute>();

      plColor iconColor = plColor::ZeroColor();

      if (pColA)
      {
        iconColor = pColA->GetColor();
      }
      else if (pCatA && iconColor == plColor::ZeroColor())
      {
        iconColor = plColorScheme::GetCategoryColor(pCatA->GetCategory(), plColorScheme::CategoryColorUsage::MenuEntryIcon);
      }

      const QIcon actionIcon = plQtUiServices::GetCachedIconResource(sIconName.GetData(), iconColor);

      if (m_pSearchableMenu != nullptr)
      {
        plStringBuilder fullName;
        fullName = pCatA ? pCatA->GetCategory() : "";
        fullName.AppendPath(plTranslate(pRtti->GetTypeName().GetData(tmp)));

        if (pInDev)
        {
          fullName.AppendFormat(" [ {} ]", pInDev->GetString());
        }

        m_pSearchableMenu->AddItem(fullName, QVariant::fromValue((void*)pRtti), actionIcon);
      }
      else
      {
        QMenu* pCat = CreateCategoryMenu(pCatA ? pCatA->GetCategory() : nullptr, existingMenus);

        plStringBuilder fullName = plTranslate(pRtti->GetTypeName().GetData(tmp));

        if (pInDev)
        {
          fullName.AppendFormat(" [ {} ]", pInDev->GetString());
        }

        // Add type action to current menu
        QAction* pAction = new QAction(fullName.GetData(), m_pMenu);
        pAction->setProperty("type", QVariant::fromValue((void*)pRtti));
        PLASMA_VERIFY(connect(pAction, SIGNAL(triggered()), this, SLOT(OnMenuAction())) != nullptr, "connection failed");

        pAction->setIcon(actionIcon);

        pCat->addAction(pAction);
      }
    }

    if (m_pSearchableMenu != nullptr)
    {
      connect(m_pSearchableMenu, &plQtSearchableMenu::MenuItemTriggered, m_pMenu, [this](const QString& sName, const QVariant& variant) {
        const plRTTI* pRtti = static_cast<const plRTTI*>(variant.value<void*>());

        OnAction(pRtti);
        m_pMenu->close(); });

      connect(m_pSearchableMenu, &plQtSearchableMenu::SearchTextChanged, m_pMenu,
        [this](const QString& text) { s_sLastMenuSearch = text.toUtf8().data(); });

      m_pMenu->addAction(m_pSearchableMenu);

      // important to do this last to make sure the search bar gets focus
      m_pSearchableMenu->Finalize(s_sLastMenuSearch.GetData());
    }
  }

  if (m_uiMaxElements > 0) // 0 means unlimited
  {
    QList<QAction*> actions = m_pMenu->actions();

    for (auto& item : m_Items)
    {
      plInt32 iCount = 0;
      m_pObjectAccessor->GetCount(item.m_pObject, m_pProp, iCount).IgnoreResult();

      if (iCount >= (plInt32)m_uiMaxElements)
      {
        if (!m_bNoMoreElementsAllowed)
        {
          m_bNoMoreElementsAllowed = true;

          QAction* pAction = new QAction(QString("Maximum allowed elements in array is %1").arg(m_uiMaxElements));
          m_pMenu->insertAction(actions.isEmpty() ? nullptr : actions[0], pAction);

          for (auto pAct : actions)
          {
            pAct->setEnabled(false);
          }
        }

        return;
      }
    }

    if (m_bNoMoreElementsAllowed)
    {
      for (auto pAct : actions)
      {
        pAct->setEnabled(true);
      }

      m_bNoMoreElementsAllowed = false;
      delete m_pMenu->actions()[0]; // remove the dummy action
    }
  }

  if (m_bPreventDuplicates)
  {
    plSet<const plRTTI*> UsedTypes;

    for (auto& item : m_Items)
    {
      plInt32 iCount = 0;
      m_pObjectAccessor->GetCount(item.m_pObject, m_pProp, iCount).IgnoreResult();

      for (plInt32 i = 0; i < iCount; ++i)
      {
        plUuid guid = m_pObjectAccessor->Get<plUuid>(item.m_pObject, m_pProp, i);

        if (guid.IsValid())
        {
          UsedTypes.Insert(m_pObjectAccessor->GetObject(guid)->GetType());
        }
      }

      QList<QAction*> actions = m_pMenu->actions();
      for (auto pAct : actions)
      {
        const plRTTI* pRtti = static_cast<const plRTTI*>(pAct->property("type").value<void*>());

        pAct->setEnabled(!UsedTypes.Contains(pRtti));
      }
    }
  }
}

void plQtAddSubElementButton::on_Button_clicked()
{
  auto pProp = GetProperty();

  if (!pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
  {
    OnAction(pProp->GetSpecificType());
  }
}

void plQtAddSubElementButton::OnMenuAction()
{
  const plRTTI* pRtti = static_cast<const plRTTI*>(sender()->property("type").value<void*>());

  OnAction(pRtti);
}

void plQtAddSubElementButton::OnAction(const plRTTI* pRtti)
{
  PLASMA_ASSERT_DEV(pRtti != nullptr, "user data retrieval failed");
  plVariant index = (plInt32)-1;

  if (m_pProp->GetCategory() == plPropertyCategory::Map)
  {
    QString text;
    bool bOk = false;
    while (!bOk)
    {
      text = QInputDialog::getText(this, "Set map key for new element", "Key:", QLineEdit::Normal, text, &bOk);
      if (!bOk)
        return;

      index = text.toUtf8().data();
      for (auto& item : m_Items)
      {
        plVariant value;
        plStatus res = m_pObjectAccessor->GetValue(item.m_pObject, m_pProp, value, index);
        if (res.m_Result.Succeeded())
        {
          bOk = false;
          break;
        }
      }
      if (!bOk)
      {
        plQtUiServices::GetSingleton()->MessageBoxInformation("The selected key is already used in the selection.");
      }
    }
  }

  m_pObjectAccessor->StartTransaction("Add Element");

  plStatus res;
  const bool bIsValueType = plReflectionUtils::IsValueType(m_pProp);
  if (bIsValueType)
  {
    for (auto& item : m_Items)
    {
      res = m_pObjectAccessor->InsertValue(item.m_pObject, m_pProp, plReflectionUtils::GetDefaultValue(GetProperty(), index), index);
      if (res.m_Result.Failed())
        break;
    }
  }
  else if (GetProperty()->GetFlags().IsSet(plPropertyFlags::Class))
  {
    for (auto& item : m_Items)
    {
      plUuid guid;
      res = m_pObjectAccessor->AddObject(item.m_pObject, m_pProp, index, pRtti, guid);
      if (res.m_Result.Failed())
        break;

      plHybridArray<plPropertySelection, 1> selection;
      selection.PushBack({m_pObjectAccessor->GetObject(guid), plVariant()});
      plDefaultObjectState defaultState(m_pObjectAccessor, selection);
      defaultState.RevertObject().IgnoreResult();
    }
  }

  if (res.m_Result.Failed())
    m_pObjectAccessor->CancelTransaction();
  else
    m_pObjectAccessor->FinishTransaction();

  plQtUiServices::GetSingleton()->MessageBoxStatus(res, "Adding sub-element to the property failed.");
}
