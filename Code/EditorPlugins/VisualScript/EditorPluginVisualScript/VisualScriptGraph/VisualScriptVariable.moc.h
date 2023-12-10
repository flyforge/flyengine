#pragma once

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

struct plVisualScriptVariable
{
  plHashedString m_sName;
  plVariant m_DefaultValue;
  bool m_bExpose = false;
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_EDITORPLUGINVISUALSCRIPT_DLL, plVisualScriptVariable);

//////////////////////////////////////////////////////////////////////////

class plVisualScriptVariableAttribute : public plTypeWidgetAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plVisualScriptVariableAttribute, plTypeWidgetAttribute);
};

//////////////////////////////////////////////////////////////////////////

class plQtVisualScriptVariableWidget : public plQtVariantPropertyWidget
{
  Q_OBJECT;

public:
  plQtVisualScriptVariableWidget();
  virtual ~plQtVisualScriptVariableWidget();

protected:
  virtual plResult GetVariantTypeDisplayName(plVariantType::Enum type, plStringBuilder& out_sName) const override;
};
