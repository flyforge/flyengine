#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/VarianceTypes.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/TagSetPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/Implementation/VarianceWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>
#include <ToolsFoundation/Document/Document.h>

#include <Foundation/Profiling/Profiling.h>
#include <GuiFoundation/Widgets/CurveEditData.h>
#include <QLayout>
#include <QScrollArea>

plRttiMappedObjectFactory<plQtPropertyWidget> plQtPropertyGridWidget::s_Factory;

static plQtPropertyWidget* StandardTypeCreator(const plRTTI* pRtti)
{
  PLASMA_ASSERT_DEV(pRtti->GetTypeFlags().IsSet(plTypeFlags::StandardType), "This function is only valid for StandardType properties, regardless of category");

  if (pRtti == plGetStaticRTTI<plVariant>())
  {
    return new plQtVariantPropertyWidget();
  }

  switch (pRtti->GetVariantType())
  {
    case plVariant::Type::Bool:
      return new plQtPropertyEditorCheckboxWidget();

    case plVariant::Type::Time:
      return new plQtPropertyEditorTimeWidget();

    case plVariant::Type::Float:
    case plVariant::Type::Double:
      return new plQtPropertyEditorDoubleSpinboxWidget(1);

    case plVariant::Type::Vector2:
      return new plQtPropertyEditorDoubleSpinboxWidget(2);

    case plVariant::Type::Vector3:
      return new plQtPropertyEditorDoubleSpinboxWidget(3);

    case plVariant::Type::Vector4:
      return new plQtPropertyEditorDoubleSpinboxWidget(4);

    case plVariant::Type::Vector2I:
      return new plQtPropertyEditorIntSpinboxWidget(2, -2147483645, 2147483645);

    case plVariant::Type::Vector3I:
      return new plQtPropertyEditorIntSpinboxWidget(3, -2147483645, 2147483645);

    case plVariant::Type::Vector4I:
      return new plQtPropertyEditorIntSpinboxWidget(4, -2147483645, 2147483645);

    case plVariant::Type::Vector2U:
      return new plQtPropertyEditorIntSpinboxWidget(2, 0, 2147483645);

    case plVariant::Type::Vector3U:
      return new plQtPropertyEditorIntSpinboxWidget(3, 0, 2147483645);

    case plVariant::Type::Vector4U:
      return new plQtPropertyEditorIntSpinboxWidget(4, 0, 2147483645);

    case plVariant::Type::Quaternion:
      return new plQtPropertyEditorQuaternionWidget();

    case plVariant::Type::Int8:
      return new plQtPropertyEditorIntSpinboxWidget(1, -127, 127);

    case plVariant::Type::UInt8:
      return new plQtPropertyEditorIntSpinboxWidget(1, 0, 255);

    case plVariant::Type::Int16:
      return new plQtPropertyEditorIntSpinboxWidget(1, -32767, 32767);

    case plVariant::Type::UInt16:
      return new plQtPropertyEditorIntSpinboxWidget(1, 0, 65535);

    case plVariant::Type::Int32:
    case plVariant::Type::Int64:
      return new plQtPropertyEditorIntSpinboxWidget(1, -2147483645, 2147483645);

    case plVariant::Type::UInt32:
    case plVariant::Type::UInt64:
      return new plQtPropertyEditorIntSpinboxWidget(1, 0, 2147483645);

    case plVariant::Type::String:
      return new plQtPropertyEditorLineEditWidget();

    case plVariant::Type::StringView:
      return new plQtPropertyEditorLineEditWidget();

    case plVariant::Type::Color:
    case plVariant::Type::ColorGamma:
      return new plQtPropertyEditorColorWidget();

    case plVariant::Type::Angle:
      return new plQtPropertyEditorAngleWidget();

    case plVariant::Type::HashedString:
      return new plQtPropertyEditorLineEditWidget();

    default:
      PLASMA_REPORT_FAILURE("No default property widget available for type: {0}", pRtti->GetTypeName());
      return nullptr;
  }
}

static plQtPropertyWidget* EnumCreator(const plRTTI* pRtti)
{
  return new plQtPropertyEditorEnumWidget();
}

static plQtPropertyWidget* BitflagsCreator(const plRTTI* pRtti)
{
  return new plQtPropertyEditorBitflagsWidget();
}

static plQtPropertyWidget* TagSetCreator(const plRTTI* pRtti)
{
  return new plQtPropertyEditorTagSetWidget();
}

static plQtPropertyWidget* VarianceTypeCreator(const plRTTI* pRtti)
{
  return new plQtVarianceTypeWidget();
}

static plQtPropertyWidget* Curve1DTypeCreator(const plRTTI* pRtti)
{
  return new plQtPropertyEditorCurve1DWidget();
}

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, PropertyGrid)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation", "PropertyMetaState"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<bool>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<float>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<double>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plVec2>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plVec3>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plVec4>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plVec2I32>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plVec3I32>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plVec4I32>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plVec2U32>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plVec3U32>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plVec4U32>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plQuat>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plInt8>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plUInt8>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plInt16>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plUInt16>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plInt32>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plUInt32>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plInt64>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plUInt64>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plConstCharPtr>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plString>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plStringView>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plTime>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plColor>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plColorGammaUB>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plAngle>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plVariant>(), StandardTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plHashedString>(), StandardTypeCreator);

    // TODO: plMat3, plMat4, plTransform, plUuid, plVariant
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plEnumBase>(), EnumCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plBitflagsBase>(), BitflagsCreator);

    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plTagSetWidgetAttribute>(), TagSetCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plVarianceTypeBase>(), VarianceTypeCreator);
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plSingleCurveData>(), Curve1DTypeCreator);


  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<bool>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<float>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<double>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plVec2>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plVec3>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plVec4>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plVec2I32>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plVec3I32>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plVec4I32>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plVec2U32>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plVec3U32>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plVec4U32>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plQuat>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plInt8>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plUInt8>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plInt16>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plUInt16>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plInt32>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plUInt32>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plInt64>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plUInt64>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plConstCharPtr>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plString>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plTime>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plColor>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plColorGammaUB>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plAngle>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plVariant>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plEnumBase>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plBitflagsBase>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plTagSetWidgetAttribute>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plVarianceTypeBase>());
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plSingleCurveData>());
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

plRttiMappedObjectFactory<plQtPropertyWidget>& plQtPropertyGridWidget::GetFactory()
{
  return s_Factory;
}

plQtPropertyGridWidget::plQtPropertyGridWidget(QWidget* pParent, plDocument* pDocument, bool bBindToSelectionManager)
  : QWidget(pParent)
{
  m_pDocument = nullptr;

  m_pScroll = new QScrollArea(this);
  m_pScroll->setContentsMargins(0, 0, 0, 0);

  m_pLayout = new QVBoxLayout(this);
  m_pLayout->setSpacing(0);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);
  m_pLayout->addWidget(m_pScroll);

  m_pContent = new QWidget(this);
  m_pScroll->setWidget(m_pContent);
  m_pScroll->setWidgetResizable(true);
  m_pContent->setBackgroundRole(QPalette::ColorRole::Window);
  m_pContent->setAutoFillBackground(true);

  m_pContentLayout = new QVBoxLayout(m_pContent);
  m_pContentLayout->setSpacing(1);
  m_pContentLayout->setContentsMargins(0, 0, 0, 0);
  m_pContent->setLayout(m_pContentLayout);

  m_pSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
  m_pContentLayout->addSpacerItem(m_pSpacer);

  m_pTypeWidget = nullptr;

  s_Factory.m_Events.AddEventHandler(plMakeDelegate(&plQtPropertyGridWidget::FactoryEventHandler, this));
  plPhantomRttiManager::s_Events.AddEventHandler(plMakeDelegate(&plQtPropertyGridWidget::TypeEventHandler, this));

  SetDocument(pDocument, bBindToSelectionManager);
}

plQtPropertyGridWidget::~plQtPropertyGridWidget()
{
  s_Factory.m_Events.RemoveEventHandler(plMakeDelegate(&plQtPropertyGridWidget::FactoryEventHandler, this));
  plPhantomRttiManager::s_Events.RemoveEventHandler(plMakeDelegate(&plQtPropertyGridWidget::TypeEventHandler, this));

  if (m_pDocument)
  {
    m_pDocument->m_ObjectAccessorChangeEvents.RemoveEventHandler(plMakeDelegate(&plQtPropertyGridWidget::ObjectAccessorChangeEventHandler, this));
    m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtPropertyGridWidget::SelectionEventHandler, this));
  }
}


void plQtPropertyGridWidget::SetDocument(plDocument* pDocument, bool bBindToSelectionManager)
{
  m_bBindToSelectionManager = bBindToSelectionManager;
  if (m_pDocument)
  {
    m_pDocument->m_ObjectAccessorChangeEvents.RemoveEventHandler(plMakeDelegate(&plQtPropertyGridWidget::ObjectAccessorChangeEventHandler, this));
    m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtPropertyGridWidget::SelectionEventHandler, this));
  }

  m_pDocument = pDocument;

  if (m_pDocument)
  {
    m_pDocument->m_ObjectAccessorChangeEvents.AddEventHandler(plMakeDelegate(&plQtPropertyGridWidget::ObjectAccessorChangeEventHandler, this));
    m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(plMakeDelegate(&plQtPropertyGridWidget::SelectionEventHandler, this));
  }
}

void plQtPropertyGridWidget::ClearSelection()
{
  if (m_pTypeWidget)
  {
    m_pContentLayout->removeWidget(m_pTypeWidget);
    m_pTypeWidget->hide();

    m_pTypeWidget->PrepareToDie();

    m_pTypeWidget->deleteLater();
    m_pTypeWidget = nullptr;
  }

  m_Selection.Clear();
}

void plQtPropertyGridWidget::SetSelectionIncludeExcludeProperties(const char* szIncludeProperties /*= nullptr*/, const char* szExcludeProperties /*= nullptr*/)
{
  m_sSelectionIncludeProperties = szIncludeProperties;
  m_sSelectionExcludeProperties = szExcludeProperties;
}

void plQtPropertyGridWidget::SetSelection(const plDeque<const plDocumentObject*>& selection)
{
  plQtScopedUpdatesDisabled _(this);

  ClearSelection();

  m_Selection = selection;

  if (m_Selection.IsEmpty())
    return;

  {
    plHybridArray<plPropertySelection, 8> Items;
    Items.Reserve(m_Selection.GetCount());

    for (const auto* sel : m_Selection)
    {
      plPropertySelection s;
      s.m_pObject = sel;

      Items.PushBack(s);
    }

    const plRTTI* pCommonType = plQtPropertyWidget::GetCommonBaseType(Items);
    m_pTypeWidget = new plQtTypeWidget(m_pContent, this, GetObjectAccessor(), pCommonType, m_sSelectionIncludeProperties, m_sSelectionExcludeProperties);
    m_pTypeWidget->SetSelection(Items);

    m_pContentLayout->insertWidget(0, m_pTypeWidget, 0);
  }
}

const plDocument* plQtPropertyGridWidget::GetDocument() const
{
  return m_pDocument;
}

const plDocumentObjectManager* plQtPropertyGridWidget::GetObjectManager() const
{
  return m_pDocument->GetObjectManager();
}

plCommandHistory* plQtPropertyGridWidget::GetCommandHistory() const
{
  return m_pDocument->GetCommandHistory();
}


plObjectAccessorBase* plQtPropertyGridWidget::GetObjectAccessor() const
{
  return m_pDocument->GetObjectAccessor();
}

plQtPropertyWidget* plQtPropertyGridWidget::CreateMemberPropertyWidget(const plAbstractProperty* pProp)
{
  // Try to create a registered widget for an existing plTypeWidgetAttribute.
  const plTypeWidgetAttribute* pAttrib = pProp->GetAttributeByType<plTypeWidgetAttribute>();
  if (pAttrib != nullptr)
  {
    plQtPropertyWidget* pWidget = plQtPropertyGridWidget::GetFactory().CreateObject(pAttrib->GetDynamicRTTI());
    if (pWidget != nullptr)
      return pWidget;
  }

  // Try to create a registered widget for the given property type.
  plQtPropertyWidget* pWidget = plQtPropertyGridWidget::GetFactory().CreateObject(pProp->GetSpecificType());
  if (pWidget != nullptr)
    return pWidget;

  return new plQtUnsupportedPropertyWidget("No property grid widget registered");
}

plQtPropertyWidget* plQtPropertyGridWidget::CreatePropertyWidget(const plAbstractProperty* pProp)
{
  switch (pProp->GetCategory())
  {
    case plPropertyCategory::Member:
    {
      // Try to create a registered widget for an existing plTypeWidgetAttribute.
      const plTypeWidgetAttribute* pAttrib = pProp->GetAttributeByType<plTypeWidgetAttribute>();
      if (pAttrib != nullptr)
      {
        plQtPropertyWidget* pWidget = plQtPropertyGridWidget::GetFactory().CreateObject(pAttrib->GetDynamicRTTI());
        if (pWidget != nullptr)
          return pWidget;
      }

      if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
      {
        if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
          return new plQtPropertyPointerWidget();
        else
          return new plQtUnsupportedPropertyWidget("Pointer: Use plPropertyFlags::PointerOwner or provide derived plTypeWidgetAttribute");
      }
      else
      {
        plQtPropertyWidget* pWidget = plQtPropertyGridWidget::GetFactory().CreateObject(pProp->GetSpecificType());
        if (pWidget != nullptr)
          return pWidget;

        if (pProp->GetFlags().IsSet(plPropertyFlags::Class))
        {
          // Member struct / class
          return new plQtPropertyTypeWidget(true);
        }
      }
    }
    break;
    case plPropertyCategory::Set:
    case plPropertyCategory::Array:
    case plPropertyCategory::Map:
    {
      // Try to create a registered container widget for an existing plContainerWidgetAttribute.
      const plContainerWidgetAttribute* pAttrib = pProp->GetAttributeByType<plContainerWidgetAttribute>();
      if (pAttrib != nullptr)
      {
        plQtPropertyWidget* pWidget = plQtPropertyGridWidget::GetFactory().CreateObject(pAttrib->GetDynamicRTTI());
        if (pWidget != nullptr)
          return pWidget;
      }

      // Fallback to default container widgets.
      const bool bIsValueType = plReflectionUtils::IsValueType(pProp);
      if (bIsValueType)
      {
        return new plQtPropertyStandardTypeContainerWidget();
      }
      else
      {
        if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
        {
          return new plQtUnsupportedPropertyWidget("Pointer: Use plPropertyFlags::PointerOwner or provide derived plContainerWidgetAttribute");
        }

        return new plQtPropertyTypeContainerWidget();
      }
    }
    break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return new plQtUnsupportedPropertyWidget();
}

void plQtPropertyGridWidget::SetCollapseState(plQtGroupBoxBase* pBox)
{
  plUInt32 uiHash = GetGroupBoxHash(pBox);
  bool bCollapsed = false;
  auto it = m_CollapseState.Find(uiHash);
  if (it.IsValid())
    bCollapsed = it.Value();

  pBox->SetCollapseState(bCollapsed);
}

void plQtPropertyGridWidget::OnCollapseStateChanged(bool bCollapsed)
{
  plQtGroupBoxBase* pBox = qobject_cast<plQtGroupBoxBase*>(sender());
  plUInt32 uiHash = GetGroupBoxHash(pBox);
  m_CollapseState[uiHash] = pBox->GetCollapseState();
}

void plQtPropertyGridWidget::ObjectAccessorChangeEventHandler(const plObjectAccessorChangeEvent& e)
{
  SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
}

void plQtPropertyGridWidget::SelectionEventHandler(const plSelectionManagerEvent& e)
{
  // TODO: even when not binding to the selection manager we need to test whether our selection is still valid.
  if (!m_bBindToSelectionManager)
    return;

  switch (e.m_Type)
  {
    case plSelectionManagerEvent::Type::SelectionCleared:
    {
      ClearSelection();
    }
    break;
    case plSelectionManagerEvent::Type::SelectionSet:
    case plSelectionManagerEvent::Type::ObjectAdded:
    case plSelectionManagerEvent::Type::ObjectRemoved:
    {
      SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
    }
    break;
  }
}

void plQtPropertyGridWidget::FactoryEventHandler(const plRttiMappedObjectFactory<plQtPropertyWidget>::Event& e)
{
  if (m_bBindToSelectionManager)
    SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
  else
  {
    plDeque<const plDocumentObject*> selection = m_Selection;
    SetSelection(selection);
  }
}

void plQtPropertyGridWidget::TypeEventHandler(const plPhantomRttiManagerEvent& e)
{
  // Adding types cannot affect the property grid content.
  if (e.m_Type == plPhantomRttiManagerEvent::Type::TypeAdded)
    return;

  PLASMA_PROFILE_SCOPE("TypeEventHandler");
  if (m_bBindToSelectionManager)
    SetSelection(m_pDocument->GetSelectionManager()->GetSelection());
  else
  {
    plDeque<const plDocumentObject*> selection = m_Selection;
    SetSelection(selection);
  }
}

plUInt32 plQtPropertyGridWidget::GetGroupBoxHash(plQtGroupBoxBase* pBox) const
{
  plUInt32 uiHash = 0;

  QWidget* pCur = pBox;
  while (pCur != nullptr && pCur != this)
  {
    plQtGroupBoxBase* pCurBox = qobject_cast<plQtGroupBoxBase*>(pCur);
    if (pCurBox != nullptr)
    {
      const QByteArray name = pCurBox->GetTitle().toUtf8().data();
      uiHash += plHashingUtils::xxHash32(name, name.length());
    }
    pCur = pCur->parentWidget();
  }
  return uiHash;
}
