#pragma once

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptNodeRegistry.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class plVisualScriptPin : public plPin
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plVisualScriptPin, plPin);

public:
  plVisualScriptPin(Type type, plStringView sName, const plVisualScriptNodeRegistry::PinDesc& pinDesc, const plDocumentObject* pObject, plUInt32 uiDataPinIndex);
  ~plVisualScriptPin();

  PLASMA_ALWAYS_INLINE bool IsExecutionPin() const { return m_ScriptDataType == plVisualScriptDataType::Invalid; }
  PLASMA_ALWAYS_INLINE bool IsDataPin() const { return m_ScriptDataType != plVisualScriptDataType::Invalid; }

  PLASMA_ALWAYS_INLINE const plRTTI* GetDataType() const { return m_pDataType; }
  PLASMA_ALWAYS_INLINE plVisualScriptDataType::Enum GetScriptDataType() const { return m_ScriptDataType; }
  plVisualScriptDataType::Enum GetResolvedScriptDataType() const;
  plStringView GetDataTypeName() const;
  PLASMA_ALWAYS_INLINE plUInt32 GetDataPinIndex() const { return m_uiDataPinIndex; }
  PLASMA_ALWAYS_INLINE bool IsRequired() const { return m_bRequired; }
  PLASMA_ALWAYS_INLINE bool HasDynamicPinProperty() const { return m_bHasDynamicPinProperty; }
  PLASMA_ALWAYS_INLINE bool SplitExecution() const { return m_bSplitExecution; }
  PLASMA_ALWAYS_INLINE bool NeedsTypeDeduction() const { return m_DeductTypeFunc != nullptr; }

  PLASMA_ALWAYS_INLINE plVisualScriptNodeRegistry::PinDesc::DeductTypeFunc GetDeductTypeFunc() const { return m_DeductTypeFunc; }

  bool CanConvertTo(const plVisualScriptPin& targetPin, bool bUseResolvedDataTypes = true) const;

private:
  const plRTTI* m_pDataType = nullptr;
  plVisualScriptNodeRegistry::PinDesc::DeductTypeFunc m_DeductTypeFunc = nullptr;
  plUInt32 m_uiDataPinIndex = 0;
  plEnum<plVisualScriptDataType> m_ScriptDataType;
  bool m_bRequired = false;
  bool m_bHasDynamicPinProperty = false;
  bool m_bSplitExecution = false;
};

class plVisualScriptNodeManager : public plDocumentNodeManager
{
public:
  plVisualScriptNodeManager();
  ~plVisualScriptNodeManager();

  plHashedString GetScriptBaseClass() const;
  bool IsFilteredByBaseClass(const plRTTI* pNodeType, const plVisualScriptNodeRegistry::NodeDesc& nodeDesc, const plHashedString& sBaseClass, bool bLogWarning = false) const;

  plVisualScriptDataType::Enum GetVariableType(plTempHashedString sName) const;
  plResult GetVariableDefaultValue(plTempHashedString sName, plVariant& out_value) const;

  void GetInputExecutionPins(const plDocumentObject* pObject, plDynamicArray<const plVisualScriptPin*>& out_pins) const;
  void GetOutputExecutionPins(const plDocumentObject* pObject, plDynamicArray<const plVisualScriptPin*>& out_pins) const;

  void GetInputDataPins(const plDocumentObject* pObject, plDynamicArray<const plVisualScriptPin*>& out_pins) const;
  void GetOutputDataPins(const plDocumentObject* pObject, plDynamicArray<const plVisualScriptPin*>& out_pins) const;

  void GetEntryNodes(const plDocumentObject* pObject, plDynamicArray<const plDocumentObject*>& out_entryNodes) const;

  static plStringView GetNiceTypeName(const plDocumentObject* pObject);
  static plStringView GetNiceFunctionName(const plDocumentObject* pObject);

  plVisualScriptDataType::Enum GetDeductedType(const plVisualScriptPin& pin) const;
  plVisualScriptDataType::Enum GetDeductedType(const plDocumentObject* pObject) const;

  bool IsCoroutine(const plDocumentObject* pObject) const;
  bool IsLoop(const plDocumentObject* pObject) const;

  plEvent<const plDocumentObject*> m_NodeChangedEvent;

private:
  virtual bool InternalIsNode(const plDocumentObject* pObject) const override;
  virtual bool InternalIsDynamicPinProperty(const plDocumentObject* pObject, const plAbstractProperty* pProp) const override;
  virtual plStatus InternalCanConnect(const plPin& source, const plPin& target, CanConnectResult& out_Result) const override;

  virtual void InternalCreatePins(const plDocumentObject* pObject, NodeInternal& node) override;

  virtual void GetCreateableTypes(plHybridArray<const plRTTI*, 32>& Types) const override;

  void NodeEventsHandler(const plDocumentNodeManagerEvent& e);
  void PropertyEventsHandler(const plDocumentObjectPropertyEvent& e);

  friend class plVisualScriptPin;
  void RemoveDeductedPinType(const plVisualScriptPin& pin);
  void DeductNodeTypeAndAllPinTypes(const plDocumentObject* pObject, const plPin* pDisconnectedPin = nullptr);
  void UpdateCoroutine(const plDocumentObject* pTargetNode, const plConnection& changedConnection, bool bIsAboutToDisconnect = false);
  bool IsConnectedToCoroutine(const plDocumentObject* pEntryNode, const plConnection& changedConnection, bool bIsAboutToDisconnect = false) const;

  plHashTable<const plDocumentObject*, plEnum<plVisualScriptDataType>> m_ObjectToDeductedType;
  plHashTable<const plVisualScriptPin*, plEnum<plVisualScriptDataType>> m_PinToDeductedType;
  plHashSet<const plDocumentObject*> m_CoroutineObjects;
};
