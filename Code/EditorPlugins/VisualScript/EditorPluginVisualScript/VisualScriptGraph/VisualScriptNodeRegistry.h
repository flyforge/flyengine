#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

struct plNodePropertyValue;
class plVisualScriptPin;

class plVisualScriptNodeRegistry
{
  PL_DECLARE_SINGLETON(plVisualScriptNodeRegistry);

public:
  struct PinDesc
  {
    plHashedString m_sName;
    plHashedString m_sDynamicPinProperty;
    const plRTTI* m_pDataType = nullptr;

    using DeductTypeFunc = plVisualScriptDataType::Enum (*)(const plVisualScriptPin& pin);
    DeductTypeFunc m_DeductTypeFunc = nullptr;

    plEnum<plVisualScriptDataType> m_ScriptDataType;
    bool m_bRequired = false;
    bool m_bSplitExecution = false;
    bool m_bReplaceWithArray = false;

    PL_ALWAYS_INLINE bool IsExecutionPin() const { return m_ScriptDataType == plVisualScriptDataType::Invalid; }
    PL_ALWAYS_INLINE bool IsDataPin() const { return m_ScriptDataType != plVisualScriptDataType::Invalid; }

    static plColor GetColorForScriptDataType(plVisualScriptDataType::Enum dataType);
    plColor GetColor() const;
  };

  struct NodeDesc
  {
    plSmallArray<PinDesc, 4> m_InputPins;
    plSmallArray<PinDesc, 4> m_OutputPins;
    plHashedString m_sFilterByBaseClass;
    const plRTTI* m_pTargetType = nullptr;
    plSmallArray<const plAbstractProperty*, 1> m_TargetProperties;

    using DeductTypeFunc = plVisualScriptDataType::Enum (*)(const plDocumentObject* pObject, const plVisualScriptPin* pDisconnectedPin);
    DeductTypeFunc m_DeductTypeFunc = nullptr;

    plEnum<plVisualScriptNodeDescription::Type> m_Type;
    bool m_bImplicitExecution = true;
    bool m_bHasDynamicPins = false;

    void AddInputExecutionPin(plStringView sName, const plHashedString& sDynamicPinProperty = plHashedString());
    void AddOutputExecutionPin(plStringView sName, const plHashedString& sDynamicPinProperty = plHashedString(), bool bSplitExecution = false);

    void AddInputDataPin(plStringView sName, const plRTTI* pDataType, plVisualScriptDataType::Enum scriptDataType, bool bRequired, const plHashedString& sDynamicPinProperty = plHashedString(), PinDesc::DeductTypeFunc deductTypeFunc = nullptr, bool bReplaceWithArray = false);
    void AddOutputDataPin(plStringView sName, const plRTTI* pDataType, plVisualScriptDataType::Enum scriptDataType, const plHashedString& sDynamicPinProperty = plHashedString(), PinDesc::DeductTypeFunc deductTypeFunc = nullptr);

    PL_ALWAYS_INLINE bool NeedsTypeDeduction() const { return m_DeductTypeFunc != nullptr; }
  };

  plVisualScriptNodeRegistry();
  ~plVisualScriptNodeRegistry();

  const plRTTI* GetNodeBaseType() const { return m_pBaseType; }
  const plRTTI* GetVariableSetterType() const { return m_pSetVariableType; }
  const plRTTI* GetVariableGetterType() const { return m_pGetVariableType; }
  const NodeDesc* GetNodeDescForType(const plRTTI* pRtti) const { return m_TypeToNodeDescs.GetValue(pRtti); }

  struct NodeCreationTemplate
  {
    const plRTTI* m_pType = nullptr;
    plStringView m_sTypeName;
    plHashedString m_sCategory;
    plUInt32 m_uiPropertyValuesStart;
    plUInt32 m_uiPropertyValuesCount;
  };

  const plArrayPtr<const NodeCreationTemplate> GetNodeCreationTemplates() const { return m_NodeCreationTemplates; }
  const plArrayPtr<const plNodePropertyValue> GetPropertyValues() const { return m_PropertyValues; }

  static constexpr const char* s_szTypeNamePrefix = "VisualScriptNode_";
  static constexpr plUInt32 s_uiTypeNamePrefixLength = plStringUtils::GetStringElementCount(s_szTypeNamePrefix);

private:
  void PhantomTypeRegistryEventHandler(const plPhantomRttiManagerEvent& e);
  void UpdateNodeTypes();
  void UpdateNodeType(const plRTTI* pRtti);

  plResult GetScriptDataType(const plRTTI* pRtti, plVisualScriptDataType::Enum& out_scriptDataType, plStringView sFunctionName = plStringView(), plStringView sArgName = plStringView());
  plVisualScriptDataType::Enum GetScriptDataType(const plAbstractProperty* pProp);

  template <typename T>
  void AddInputDataPin(plReflectedTypeDescriptor& ref_typeDesc, NodeDesc& ref_nodeDesc, plStringView sName);
  void AddInputDataPin_Any(plReflectedTypeDescriptor& ref_typeDesc, NodeDesc& ref_nodeDesc, plStringView sName, bool bRequired, bool bAddVariantProperty = false, PinDesc::DeductTypeFunc deductTypeFunc = nullptr);

  template <typename T>
  void AddOutputDataPin(NodeDesc& ref_nodeDesc, plStringView sName);

  void CreateBuiltinTypes();
  void CreateGetOwnerNodeType(const plRTTI* pRtti);
  void CreateFunctionCallNodeType(const plRTTI* pRtti, const plHashedString& sCategory, const plAbstractFunctionProperty* pFunction, const plScriptableFunctionAttribute* pScriptableFunctionAttribute, bool bIsEntryFunction);
  void CreateCoroutineNodeType(const plRTTI* pRtti);
  void CreateMessageNodeTypes(const plRTTI* pRtti);
  void CreateEnumNodeTypes(const plRTTI* pRtti);

  void FillDesc(plReflectedTypeDescriptor& desc, const plRTTI* pRtti, const plColorGammaUB* pColorOverride = nullptr);
  void FillDesc(plReflectedTypeDescriptor& desc, plStringView sTypeName, const plColorGammaUB& color);

  const plRTTI* RegisterNodeType(plReflectedTypeDescriptor& typeDesc, NodeDesc&& nodeDesc, const plHashedString& sCategory);

  const plRTTI* m_pBaseType = nullptr;
  const plRTTI* m_pSetPropertyType = nullptr;
  const plRTTI* m_pGetPropertyType = nullptr;
  const plRTTI* m_pSetVariableType = nullptr;
  const plRTTI* m_pGetVariableType = nullptr;
  bool m_bBuiltinTypesCreated = false;
  plMap<const plRTTI*, NodeDesc> m_TypeToNodeDescs;
  plHashSet<const plRTTI*> m_EnumTypes;

  plDynamicArray<NodeCreationTemplate> m_NodeCreationTemplates;
  plDynamicArray<plNodePropertyValue> m_PropertyValues;
  plDeque<plString> m_PropertyNodeTypeNames;
};
