#pragma once

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

struct plVisualScriptVariable
{
  plHashedString m_sName;
  plVariant m_DefaultValue;
  bool m_bExpose = false;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_EDITORPLUGINVISUALSCRIPT_DLL, plVisualScriptVariable);

//////////////////////////////////////////////////////////////////////////

class plVisualScriptVariableAttribute : public plTypeWidgetAttribute
{
  PL_ADD_DYNAMIC_REFLECTION(plVisualScriptVariableAttribute, plTypeWidgetAttribute);
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

//////////////////////////////////////////////////////////////////////////

struct plVisualScriptExpressionDataType
{
  using StorageType = plUInt8;

  enum Enum
  {
    Int = static_cast<plUInt8>(plProcessingStream::DataType::Int),
    Float = static_cast<plUInt8>(plProcessingStream::DataType::Float),
    Vector3 = static_cast<plUInt8>(plProcessingStream::DataType::Float3),
    Color = static_cast<plUInt8>(plProcessingStream::DataType::Float4),

    Default = Float
  };

  static plVisualScriptDataType::Enum GetVisualScriptDataType(Enum dataType);
};

PL_DECLARE_REFLECTABLE_TYPE(PL_EDITORPLUGINVISUALSCRIPT_DLL, plVisualScriptExpressionDataType);

struct plVisualScriptExpressionVariable
{
  plHashedString m_sName;
  plEnum<plVisualScriptExpressionDataType> m_Type;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_EDITORPLUGINVISUALSCRIPT_DLL, plVisualScriptExpressionVariable);
