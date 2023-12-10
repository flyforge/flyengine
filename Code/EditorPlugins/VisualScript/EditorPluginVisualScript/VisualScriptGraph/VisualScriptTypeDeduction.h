#pragma once

#include <VisualScriptPlugin/Runtime/VisualScriptDataType.h>

class plVisualScriptPin;

class plVisualScriptTypeDeduction
{
public:
  static plVisualScriptDataType::Enum DeductFromNodeDataType(const plVisualScriptPin& pin);
  static plVisualScriptDataType::Enum DeductFromTypeProperty(const plVisualScriptPin& pin);

  static plVisualScriptDataType::Enum DeductFromAllInputPins(const plDocumentObject* pObject, const plVisualScriptPin* pDisconnectedPin);
  static plVisualScriptDataType::Enum DeductFromVariableNameProperty(const plDocumentObject* pObject, const plVisualScriptPin* pDisconnectedPin);
  static plVisualScriptDataType::Enum DeductFromScriptDataTypeProperty(const plDocumentObject* pObject, const plVisualScriptPin* pDisconnectedPin);
  static plVisualScriptDataType::Enum DeductFromPropertyProperty(const plDocumentObject* pObject, const plVisualScriptPin* pDisconnectedPin);

  static const plRTTI* GetReflectedType(const plDocumentObject* pObject);
  static const plAbstractProperty* GetReflectedProperty(const plDocumentObject* pObject);
};
