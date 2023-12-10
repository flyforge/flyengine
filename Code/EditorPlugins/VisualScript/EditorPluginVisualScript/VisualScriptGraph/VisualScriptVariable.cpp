#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptVariable.moc.h>

#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plVisualScriptVariable, plNoBase, 1, plRTTIDefaultAllocator<plVisualScriptVariable>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Name", m_sName),
    PLASMA_MEMBER_PROPERTY("DefaultValue", m_DefaultValue)->AddAttributes(new plDefaultValueAttribute(0), new plVisualScriptVariableAttribute()),
    PLASMA_MEMBER_PROPERTY("Expose", m_bExpose),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plVisualScriptVariableAttribute, 1, plRTTIDefaultAllocator<plVisualScriptVariableAttribute>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

static plQtPropertyWidget* VisualScriptVariableTypeCreator(const plRTTI* pRtti)
{
  return new plQtVisualScriptVariableWidget();
}

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginVisualScript, VisualScriptVariable)

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

PLASMA_END_SUBSYSTEM_DECLARATION;
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
    return PLASMA_FAILURE;

  plVisualScriptDataType::Enum dataType = plVisualScriptDataType::FromVariantType(type);
  if (type != plVariantType::Invalid && dataType == plVisualScriptDataType::Invalid)
    return PLASMA_FAILURE;

  const plRTTI* pVisualScriptDataType = plGetStaticRTTI<plVisualScriptDataType>();
  if (plReflectionUtils::EnumerationToString(pVisualScriptDataType, dataType, out_sName) == false)
    return PLASMA_FAILURE;

  return PLASMA_SUCCESS;
}
