#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>

#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plVisualScriptVariable, plNoBase, 1, plRTTIDefaultAllocator<plVisualScriptVariable>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Name", m_sName),
    PL_MEMBER_PROPERTY("DefaultValue", m_DefaultValue)->AddAttributes(new plDefaultValueAttribute(0), new plVisualScriptVariableAttribute()),
    PL_MEMBER_PROPERTY("Expose", m_bExpose),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plVisualScriptVariableAttribute, 1, plRTTIDefaultAllocator<plVisualScriptVariableAttribute>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

static plQtPropertyWidget* VisualScriptVariableTypeCreator(const plRTTI* pRtti)
{
  return new plQtVisualScriptVariableWidget();
}

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginVisualScript, VisualScriptVariable)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation", "PropertyMetaState"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plQtPropertyGridWidget::GetFactory().RegisterCreator(plGetStaticRTTI<plVisualScriptVariableAttribute>(), VisualScriptVariableTypeCreator);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plQtPropertyGridWidget::GetFactory().UnregisterCreator(plGetStaticRTTI<plVisualScriptVariableAttribute>());
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

plQtVisualScriptVariableWidget::plQtVisualScriptVariableWidget() = default;
plQtVisualScriptVariableWidget::~plQtVisualScriptVariableWidget() = default;

plResult plQtVisualScriptVariableWidget::GetVariantTypeDisplayName(plVariantType::Enum type, plStringBuilder& out_sName) const
{
  if (type == plVariantType::Int8 ||
      type == plVariantType::Int16 ||
      type == plVariantType::UInt16 ||
      type == plVariantType::UInt32 ||
      type == plVariantType::UInt64 ||
      type == plVariantType::StringView ||
      type == plVariantType::TempHashedString)
    return PL_FAILURE;

  plVisualScriptDataType::Enum dataType = plVisualScriptDataType::FromVariantType(type);
  if (type != plVariantType::Invalid && dataType == plVisualScriptDataType::Invalid)
    return PL_FAILURE;

  const plRTTI* pVisualScriptDataType = plGetStaticRTTI<plVisualScriptDataType>();
  if (plReflectionUtils::EnumerationToString(pVisualScriptDataType, dataType, out_sName) == false)
    return PL_FAILURE;

  return PL_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plVisualScriptExpressionDataType, 1)
  PL_ENUM_CONSTANT(plVisualScriptExpressionDataType::Int),
  PL_ENUM_CONSTANT(plVisualScriptExpressionDataType::Float),
  PL_ENUM_CONSTANT(plVisualScriptExpressionDataType::Vector3),
  PL_ENUM_CONSTANT(plVisualScriptExpressionDataType::Color),
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_TYPE(plVisualScriptExpressionVariable, plNoBase, 1, plRTTIDefaultAllocator<plVisualScriptExpressionVariable>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Name", m_sName),
    PL_ENUM_MEMBER_PROPERTY("Type", plVisualScriptExpressionDataType, m_Type),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

plVisualScriptDataType::Enum plVisualScriptExpressionDataType::GetVisualScriptDataType(Enum dataType)
{
  switch (dataType)
  {
    case Int:
      return plVisualScriptDataType::Int;
    case Float:
      return plVisualScriptDataType::Float;
    case Vector3:
      return plVisualScriptDataType::Vector3;
    case Color:
      return plVisualScriptDataType::Color;
    default:
      PL_ASSERT_NOT_IMPLEMENTED;
  }

  return plVisualScriptDataType::Invalid;
}
