#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/GUI/ExposedParametersTypeRegistry.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/GroupBoxBase.moc.h>

plExposedParameterCommandAccessor::plExposedParameterCommandAccessor(
  plObjectAccessorBase* pSource, const plAbstractProperty* pParameterProp, const plAbstractProperty* pParameterSourceProp)
  : plObjectProxyAccessor(pSource)
  , m_pParameterProp(pParameterProp)
  , m_pParameterSourceProp(pParameterSourceProp)
{
}

plStatus plExposedParameterCommandAccessor::GetValue(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant& out_value, plVariant index /*= plVariant()*/)
{
  if (IsExposedProperty(pObject, pProp))
    pProp = m_pParameterProp;

  plStatus res = plObjectProxyAccessor::GetValue(pObject, pProp, out_value, index);
  if (res.Succeeded() && !index.IsValid() && m_pParameterProp == pProp)
  {
    plVariantDictionary defaultDict;
    if (const plExposedParameters* pParams = GetExposedParams(pObject))
    {
      for (plExposedParameter* pParam : pParams->m_Parameters)
      {
        defaultDict.Insert(pParam->m_sName, pParam->m_DefaultValue);
      }
    }
    const plVariantDictionary& overwrittenDict = out_value.Get<plVariantDictionary>();
    for (auto it : overwrittenDict)
    {
      defaultDict[it.Key()] = it.Value();
    }
    out_value = defaultDict;
  }
  else if (res.Failed() && m_pParameterProp == pProp && index.IsA<plString>())
  {
    // If the actual GetValue fails but the key is an exposed param, return its default value instead.
    if (const plExposedParameter* pParam = GetExposedParam(pObject, index.Get<plString>()))
    {
      out_value = pParam->m_DefaultValue;
      return plStatus(PLASMA_SUCCESS);
    }
  }
  return res;
}

plStatus plExposedParameterCommandAccessor::SetValue(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index /*= plVariant()*/)
{
  if (IsExposedProperty(pObject, pProp))
    pProp = m_pParameterProp;

  plStatus res = plObjectProxyAccessor::SetValue(pObject, pProp, newValue, index);
  // As we pretend the exposed params always exist the actual SetValue will fail if this is not actually true,
  // so we redirect to insert to make it true.
  if (res.Failed() && m_pParameterProp == pProp && index.IsA<plString>())
  {
    return InsertValue(pObject, pProp, newValue, index);
  }
  return res;
}

plStatus plExposedParameterCommandAccessor::RemoveValue(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index /*= plVariant()*/)
{
  plStatus res = plObjectProxyAccessor::RemoveValue(pObject, pProp, index);
  if (res.Failed() && m_pParameterProp == pProp && index.IsA<plString>())
  {
    // It this is one of the exposed params, pretend we removed it successfully to suppress error messages.
    if (const plExposedParameter* pParam = GetExposedParam(pObject, index.Get<plString>()))
    {
      return plStatus(PLASMA_SUCCESS);
    }
  }
  return res;
}

plStatus plExposedParameterCommandAccessor::GetCount(const plDocumentObject* pObject, const plAbstractProperty* pProp, plInt32& out_iCount)
{
  if (m_pParameterProp == pProp)
  {
    plHybridArray<plVariant, 16> keys;
    GetKeys(pObject, pProp, keys).IgnoreResult();
    out_iCount = keys.GetCount();
    return plStatus(PLASMA_SUCCESS);
  }
  return plObjectProxyAccessor::GetCount(pObject, pProp, out_iCount);
}

plStatus plExposedParameterCommandAccessor::GetKeys(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, plDynamicArray<plVariant>& out_keys)
{
  if (m_pParameterProp == pProp)
  {
    if (const plExposedParameters* pParams = GetExposedParams(pObject))
    {
      for (const auto& pParam : pParams->m_Parameters)
      {
        out_keys.PushBack(plVariant(pParam->m_sName));
      }

      plHybridArray<plVariant, 16> realKeys;
      plStatus res = plObjectProxyAccessor::GetKeys(pObject, pProp, realKeys);
      for (const auto& key : realKeys)
      {
        if (!out_keys.Contains(key))
        {
          out_keys.PushBack(key);
        }
      }
      return plStatus(PLASMA_SUCCESS);
    }
  }
  return plObjectProxyAccessor::GetKeys(pObject, pProp, out_keys);
}

plStatus plExposedParameterCommandAccessor::GetValues(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, plDynamicArray<plVariant>& out_values)
{
  if (m_pParameterProp == pProp)
  {
    plHybridArray<plVariant, 16> keys;
    GetKeys(pObject, pProp, keys).IgnoreResult();
    for (const auto& key : keys)
    {
      auto& var = out_values.ExpandAndGetRef();
      PLASMA_VERIFY(GetValue(pObject, pProp, var, key).Succeeded(), "GetValue to valid a key should be not fail.");
    }
    return plStatus(PLASMA_SUCCESS);
  }
  return plObjectProxyAccessor::GetValues(pObject, pProp, out_values);
}


const plExposedParameters* plExposedParameterCommandAccessor::GetExposedParams(const plDocumentObject* pObject)
{
  plVariant value;
  if (plObjectProxyAccessor::GetValue(pObject, m_pParameterSourceProp, value).Succeeded())
  {
    if (value.IsA<plString>())
    {
      const auto& sValue = value.Get<plString>();
      if (const auto asset = plAssetCurator::GetSingleton()->FindSubAsset(sValue.GetData()))
      {
        return asset->m_pAssetInfo->m_Info->GetMetaInfo<plExposedParameters>();
      }
    }
  }
  return nullptr;
}


const plExposedParameter* plExposedParameterCommandAccessor::GetExposedParam(const plDocumentObject* pObject, const char* szParamName)
{
  if (const plExposedParameters* pParams = GetExposedParams(pObject))
  {
    return pParams->Find(szParamName);
  }
  return nullptr;
}


const plRTTI* plExposedParameterCommandAccessor::GetExposedParamsType(const plDocumentObject* pObject)
{
  plVariant value;
  if (plObjectProxyAccessor::GetValue(pObject, m_pParameterSourceProp, value).Succeeded())
  {
    if (value.IsA<plString>())
    {
      const auto& sValue = value.Get<plString>();
      if (const auto asset = plAssetCurator::GetSingleton()->FindSubAsset(sValue.GetData()))
      {
        return plExposedParametersTypeRegistry::GetSingleton()->GetExposedParametersType(sValue);
      }
    }
  }
  return nullptr;
}

const plRTTI* plExposedParameterCommandAccessor::GetCommonExposedParamsType(const plHybridArray<plPropertySelection, 8>& items)
{
  const plRTTI* type = nullptr;
  bool bFirst = true;
  // check if we have multiple values
  for (const auto& item : items)
  {
    if (bFirst)
    {
      type = GetExposedParamsType(item.m_pObject);
    }
    else
    {
      auto type2 = GetExposedParamsType(item.m_pObject);
      if (type != type2)
      {
        return nullptr;
      }
    }
  }
  return type;
}

bool plExposedParameterCommandAccessor::IsExposedProperty(const plDocumentObject* pObject, const plAbstractProperty* pProp)
{
  if (auto type = GetExposedParamsType(pObject))
  {
    auto props = type->GetProperties();
    return std::any_of(cbegin(props), cend(props), [pProp](const plAbstractProperty* prop) { return prop == pProp; });
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////

void plQtExposedParameterPropertyWidget::InternalSetValue(const plVariant& value)
{
  plVariantType::Enum commonType = plVariantType::Invalid;
  const bool sameType = GetCommonVariantSubType(m_Items, m_pProp, commonType);
  const plRTTI* pNewtSubType = commonType != plVariantType::Invalid ? plReflectionUtils::GetTypeFromVariant(commonType) : nullptr;

  plExposedParameterCommandAccessor* proxy = static_cast<plExposedParameterCommandAccessor*>(m_pObjectAccessor);
  if (auto type = proxy->GetCommonExposedParamsType(m_Items))
  {
    if (auto prop = type->FindPropertyByName(m_Items[0].m_Index.Get<plString>()))
    {
      auto paramDefault = plToolsReflectionUtils::GetStorageDefault(prop);
      if (paramDefault.GetType() == commonType)
      {
        if (prop->GetSpecificType() != m_pCurrentSubType || m_pWidget == nullptr)
        {
          if (m_pWidget)
          {
            m_pWidget->PrepareToDie();
            m_pWidget->deleteLater();
            m_pWidget = nullptr;
          }
          m_pCurrentSubType = pNewtSubType;
          m_pWidget = plQtPropertyGridWidget::CreateMemberPropertyWidget(prop);
          if (!m_pWidget)
            m_pWidget = new plQtUnsupportedPropertyWidget("Unsupported type");

          m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
          m_pWidget->setParent(this);
          m_pLayout->addWidget(m_pWidget);
          m_pWidget->Init(m_pGrid, m_pObjectAccessor, type, prop);
          UpdateTypeListSelection(commonType);
        }
        m_pWidget->SetSelection(m_Items);
        return;
      }
    }
  }
  plQtVariantPropertyWidget::InternalSetValue(value);
}

//////////////////////////////////////////////////////////////////////////

plQtExposedParametersPropertyWidget::plQtExposedParametersPropertyWidget() {}

plQtExposedParametersPropertyWidget::~plQtExposedParametersPropertyWidget()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plQtExposedParametersPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtExposedParametersPropertyWidget::CommandHistoryEventHandler, this));
}

void plQtExposedParametersPropertyWidget::SetSelection(const plHybridArray<plPropertySelection, 8>& items)
{
  plQtPropertyStandardTypeContainerWidget::SetSelection(items);
  UpdateActionState();
}

void plQtExposedParametersPropertyWidget::OnInit()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plQtExposedParametersPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.AddEventHandler(plMakeDelegate(&plQtExposedParametersPropertyWidget::CommandHistoryEventHandler, this));

  const auto* pAttrib = m_pProp->GetAttributeByType<plExposedParametersAttribute>();
  PLASMA_ASSERT_DEV(pAttrib, "plQtExposedParametersPropertyWidget was created for a property that does not have the plExposedParametersAttribute.");
  m_sExposedParamProperty = pAttrib->GetParametersSource();
  const plAbstractProperty* pParameterSourceProp = m_pType->FindPropertyByName(m_sExposedParamProperty);
  PLASMA_ASSERT_DEV(
    pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", m_sExposedParamProperty, m_pType->GetTypeName());
  m_pSourceObjectAccessor = m_pObjectAccessor;
  m_pProxy = PLASMA_DEFAULT_NEW(plExposedParameterCommandAccessor, m_pSourceObjectAccessor, m_pProp, pParameterSourceProp);
  m_pObjectAccessor = m_pProxy.Borrow();

  plQtPropertyStandardTypeContainerWidget::OnInit();

  {
    // Help button to indicate exposed parameter mismatches.
    m_pFixMeButton = new QToolButton();
    m_pFixMeButton->setAutoRaise(true);
    m_pFixMeButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
    m_pFixMeButton->setIcon(plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Attention.svg"));
    auto sp = m_pFixMeButton->sizePolicy();
    sp.setVerticalPolicy(QSizePolicy::Ignored);
    m_pFixMeButton->setSizePolicy(sp);
    QMenu* pFixMeMenu = new QMenu(m_pFixMeButton);
    {
      m_pRemoveUnusedAction = pFixMeMenu->addAction(QStringLiteral("Remove unused keys"));
      m_pRemoveUnusedAction->setToolTip(
        QStringLiteral("The map contains keys that are no longer used by the asset's exposed parameters and thus can be removed."));
      connect(m_pRemoveUnusedAction, &QAction::triggered, this, [this](bool checked) { RemoveUnusedKeys(false); });
    }
    {
      m_pFixTypesAction = pFixMeMenu->addAction(QStringLiteral("Fix keys with wrong types"));
      connect(m_pFixTypesAction, &QAction::triggered, this, [this](bool checked) { FixKeyTypes(false); });
    }
    m_pFixMeButton->setMenu(pFixMeMenu);

    auto layout = qobject_cast<QHBoxLayout*>(m_pGroup->GetHeader()->layout());
    layout->insertWidget(layout->count() - 1, m_pFixMeButton);
  }
}

plQtPropertyWidget* plQtExposedParametersPropertyWidget::CreateWidget(plUInt32 index)
{
  return new plQtExposedParameterPropertyWidget();
}


void plQtExposedParametersPropertyWidget::UpdateElement(plUInt32 index)
{
  plQtPropertyStandardTypeContainerWidget::UpdateElement(index);
}

void plQtExposedParametersPropertyWidget::UpdatePropertyMetaState()
{
  plQtPropertyStandardTypeContainerWidget::UpdatePropertyMetaState();
  return;

  for (plUInt32 i = 0; i < m_Elements.GetCount(); i++)
  {
    Element& elem = m_Elements[i];
    const auto& selection = elem.m_pWidget->GetSelection();
    bool isDefault = true;
    for (const auto& item : selection)
    {
      plVariant value;
      plStatus res = m_pSourceObjectAccessor->GetValue(item.m_pObject, m_pProp, value, item.m_Index);
      if (res.Succeeded())
      {
        // In case we successfully read the value from the source accessor (not the proxy that pretends all exposed params exist)
        // we now the value is overwritten as in the default case the map index would not exist.
        isDefault = false;
        break;
      }
    }
    elem.m_pWidget->SetIsDefault(isDefault);
    elem.m_pSubGroup->SetBoldTitle(!isDefault);
  }
}

void plQtExposedParametersPropertyWidget::PropertyEventHandler(const plDocumentObjectPropertyEvent& e)
{
  if (IsUndead())
    return;

  if (std::none_of(cbegin(m_Items), cend(m_Items), [=](const plPropertySelection& sel) { return e.m_pObject == sel.m_pObject; }))
    return;

  if (!m_bNeedsUpdate && m_sExposedParamProperty == e.m_sProperty)
  {
    m_bNeedsUpdate = true;
    // In case the change happened outside the command history we have to update at once.
    if (!m_pGrid->GetCommandHistory()->IsInTransaction() && !m_pGrid->GetCommandHistory()->IsInUndoRedo())
      FlushQueuedChanges();
  }
  if (!m_bNeedsMetaDataUpdate && m_pProp->GetPropertyName() == e.m_sProperty)
  {
    m_bNeedsMetaDataUpdate = true;
    if (!m_pGrid->GetCommandHistory()->IsInTransaction() && !m_pGrid->GetCommandHistory()->IsInUndoRedo())
      FlushQueuedChanges();
  }
}

void plQtExposedParametersPropertyWidget::CommandHistoryEventHandler(const plCommandHistoryEvent& e)
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

void plQtExposedParametersPropertyWidget::FlushQueuedChanges()
{
  if (m_bNeedsUpdate)
  {
    m_bNeedsUpdate = false;
    SetSelection(m_Items);
  }
  if (m_bNeedsMetaDataUpdate)
  {
    UpdateActionState();
  }
}

bool plQtExposedParametersPropertyWidget::RemoveUnusedKeys(bool bTestOnly)
{
  bool bStuffDone = false;
  if (!bTestOnly)
    m_pSourceObjectAccessor->StartTransaction("Remove unused keys");
  for (const auto& item : m_Items)
  {
    if (const plExposedParameters* pParams = m_pProxy->GetExposedParams(item.m_pObject))
    {
      plHybridArray<plVariant, 16> keys;
      PLASMA_VERIFY(m_pSourceObjectAccessor->GetKeys(item.m_pObject, m_pProp, keys).Succeeded(), "");
      for (auto& key : keys)
      {
        if (!pParams->Find(key.Get<plString>()))
        {
          if (!bTestOnly)
          {
            bStuffDone = true;
            m_pSourceObjectAccessor->RemoveValue(item.m_pObject, m_pProp, key).LogFailure();
          }
          else
          {
            return true;
          }
        }
      }
    }
  }
  if (!bTestOnly)
    m_pSourceObjectAccessor->FinishTransaction();
  return bStuffDone;
}

bool plQtExposedParametersPropertyWidget::FixKeyTypes(bool bTestOnly)
{
  bool bStuffDone = false;
  if (!bTestOnly)
    m_pSourceObjectAccessor->StartTransaction("Remove unused keys");
  for (const auto& item : m_Items)
  {
    if (const plExposedParameters* pParams = m_pProxy->GetExposedParams(item.m_pObject))
    {
      plHybridArray<plVariant, 16> keys;
      PLASMA_VERIFY(m_pSourceObjectAccessor->GetKeys(item.m_pObject, m_pProp, keys).Succeeded(), "");
      for (auto& key : keys)
      {
        if (const auto* pParam = pParams->Find(key.Get<plString>()))
        {
          plVariant value;
          const plRTTI* pType = pParam->m_DefaultValue.GetReflectedType();
          PLASMA_VERIFY(m_pSourceObjectAccessor->GetValue(item.m_pObject, m_pProp, value, key).Succeeded(), "");
          if (value.GetReflectedType() != pType)
          {
            if (!bTestOnly)
            {
              bStuffDone = true;
              plVariantType::Enum type = pParam->m_DefaultValue.GetType();
              if (value.CanConvertTo(type))
              {
                m_pObjectAccessor->SetValue(item.m_pObject, m_pProp, value.ConvertTo(type), key).LogFailure();
              }
              else
              {
                m_pObjectAccessor->SetValue(item.m_pObject, m_pProp, pParam->m_DefaultValue, key).LogFailure();
              }
            }
            else
            {
              return true;
            }
          }
        }
      }
    }
  }
  if (!bTestOnly)
    m_pSourceObjectAccessor->FinishTransaction();
  return bStuffDone;
}

void plQtExposedParametersPropertyWidget::UpdateActionState()
{
  m_bNeedsMetaDataUpdate = false;
  m_pRemoveUnusedAction->setEnabled(RemoveUnusedKeys(true));
  m_pFixTypesAction->setEnabled(FixKeyTypes(true));
  m_pFixMeButton->setVisible(m_pRemoveUnusedAction->isEnabled() || m_pFixTypesAction->isEnabled());
}
