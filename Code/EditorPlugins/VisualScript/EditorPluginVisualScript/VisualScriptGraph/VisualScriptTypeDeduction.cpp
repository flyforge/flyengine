#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptTypeDeduction.h>

// static
plVisualScriptDataType::Enum plVisualScriptTypeDeduction::DeductFromNodeDataType(const plVisualScriptPin& pin)
{
  auto pObject = pin.GetParent();
  auto pManager = static_cast<const plVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());

  return pManager->GetDeductedType(pObject);
}

// static
plVisualScriptDataType::Enum plVisualScriptTypeDeduction::DeductFromTypeProperty(const plVisualScriptPin& pin)
{
  if (auto pType = GetReflectedType(pin.GetParent()))
  {
    return plVisualScriptDataType::FromRtti(pType);
  }

  return plVisualScriptDataType::Invalid;
}

// static
plVisualScriptDataType::Enum plVisualScriptTypeDeduction::DeductFromAllInputPins(const plDocumentObject* pObject, const plVisualScriptPin* pDisconnectedPin)
{
  auto pManager = static_cast<const plVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());

  plVisualScriptDataType::Enum deductedType = plVisualScriptDataType::Invalid;

  plHybridArray<const plVisualScriptPin*, 16> pins;
  pManager->GetInputDataPins(pObject, pins);
  for (auto pPin : pins)
  {
    if (pPin->GetScriptDataType() != plVisualScriptDataType::Any)
      continue;

    // the pin is about to be disconnected so we ignore it here
    if (pPin == pDisconnectedPin)
      continue;

    plVisualScriptDataType::Enum pinDataType = plVisualScriptDataType::Invalid;
    auto connections = pManager->GetConnections(*pPin);
    if (connections.IsEmpty() == false)
    {
      pinDataType = static_cast<const plVisualScriptPin&>(connections[0]->GetSourcePin()).GetResolvedScriptDataType();
    }
    else
    {
      plVariant var = pObject->GetTypeAccessor().GetValue(pPin->GetName());
      pinDataType = plVisualScriptDataType::FromVariantType(var.GetType());
    }

    deductedType = plMath::Max(deductedType, pinDataType);
  }

  return deductedType;
}

// static
plVisualScriptDataType::Enum plVisualScriptTypeDeduction::DeductFromVariableNameProperty(const plDocumentObject* pObject, const plVisualScriptPin* pDisconnectedPin)
{
  auto nameVar = pObject->GetTypeAccessor().GetValue("Name");
  if (nameVar.IsA<plString>())
  {
    auto pManager = static_cast<const plVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());
    return pManager->GetVariableType(plTempHashedString(nameVar.Get<plString>()));
  }

  return plVisualScriptDataType::Invalid;
}

// static
plVisualScriptDataType::Enum plVisualScriptTypeDeduction::DeductFromScriptDataTypeProperty(const plDocumentObject* pObject, const plVisualScriptPin* pDisconnectedPin)
{
  auto typeVar = pObject->GetTypeAccessor().GetValue("Type");
  if (typeVar.IsA<plInt64>())
  {
    return static_cast<plVisualScriptDataType::Enum>(typeVar.Get<plInt64>());
  }

  return plVisualScriptDataType::Invalid;
}

// static
plVisualScriptDataType::Enum plVisualScriptTypeDeduction::DeductFromPropertyProperty(const plDocumentObject* pObject, const plVisualScriptPin* pDisconnectedPin)
{
  if (auto pProperty = GetReflectedProperty(pObject))
  {
    return plVisualScriptDataType::FromRtti(pProperty->GetSpecificType());
  }

  return plVisualScriptDataType::Invalid;
}

// static
const plRTTI* plVisualScriptTypeDeduction::GetReflectedType(const plDocumentObject* pObject)
{
  auto typeVar = pObject->GetTypeAccessor().GetValue("Type");
  if (typeVar.IsA<plString>() == false)
    return nullptr;

  const plString& sTypeName = typeVar.Get<plString>();
  if (sTypeName.IsEmpty())
    return nullptr;

  const plRTTI* pType = plRTTI::FindTypeByName(sTypeName);
  if (pType == nullptr && sTypeName.StartsWith("pl") == false)
  {
    plStringBuilder sFullTypeName;
    sFullTypeName.Set("pl", typeVar.Get<plString>());
    pType = plRTTI::FindTypeByName(sFullTypeName);
  }

  if (pType == nullptr)
  {
    plLog::Error("'{}' is not a valid type", typeVar.Get<plString>());
    return nullptr;
  }

  return pType;
}

// static
const plAbstractProperty* plVisualScriptTypeDeduction::GetReflectedProperty(const plDocumentObject* pObject)
{
  auto pType = GetReflectedType(pObject);
  if (pType == nullptr)
    return nullptr;

  auto propertyVar = pObject->GetTypeAccessor().GetValue("Property");
  if (propertyVar.IsA<plString>() == false)
    return nullptr;

  const plString& sPropertyName = propertyVar.Get<plString>();
  if (sPropertyName.IsEmpty())
    return nullptr;

  const plAbstractProperty* pProperty = pType->FindPropertyByName(propertyVar.Get<plString>());

  if (pProperty == nullptr)
  {
    plLog::Error("'{}' is not a valid property of '{}'", propertyVar.Get<plString>(), pType->GetTypeName());
    return nullptr;
  }

  return pProperty;
}
