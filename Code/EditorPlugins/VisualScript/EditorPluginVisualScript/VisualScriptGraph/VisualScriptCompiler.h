#pragma once

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>

class plVisualScriptCompiler
{
public:
  plVisualScriptCompiler();
  ~plVisualScriptCompiler();

  void InitModule(plStringView sBaseClassName, plStringView sScriptClassName);

  plResult AddFunction(plStringView sName, const plDocumentObject* pEntryObject, const plDocumentObject* pParentObject = nullptr);

  plResult Compile(plStringView sDebugAstOutputPath = plStringView());

  struct CompiledFunction
  {
    plString m_sName;
    plEnum<plVisualScriptNodeDescription::Type> m_Type;
    plEnum<plScriptCoroutineCreationMode> m_CoroutineCreationMode;
    plDynamicArray<plVisualScriptNodeDescription> m_NodeDescriptions;
    plVisualScriptDataDescription m_LocalDataDesc;
  };

  struct CompiledModule
  {
    CompiledModule();

    plResult Serialize(plStreamWriter& inout_stream) const;

    plString m_sBaseClassName;
    plString m_sScriptClassName;
    plHybridArray<CompiledFunction, 16> m_Functions;

    plVisualScriptDataDescription m_InstanceDataDesc;
    plVisualScriptInstanceDataMapping m_InstanceDataMapping;

    plVisualScriptDataDescription m_ConstantDataDesc;
    plVisualScriptDataStorage m_ConstantDataStorage;
  };

  const CompiledModule& GetCompiledModule() const { return m_Module; }

  struct AstNode;

  struct DataInput
  {
    PL_DECLARE_POD_TYPE();

    AstNode* m_pSourceNode = nullptr;
    plUInt32 m_uiId = 0;
    plUInt8 m_uiSourcePinIndex = 0;
    plEnum<plVisualScriptDataType> m_DataType;
  };

  struct DataOutput
  {
    plSmallArray<AstNode*, 3> m_TargetNodes;
    plUInt32 m_uiId = 0;
    plEnum<plVisualScriptDataType> m_DataType;
  };

  struct AstNode
  {
    plEnum<plVisualScriptNodeDescription::Type> m_Type;
    plEnum<plVisualScriptDataType> m_DeductedDataType;
    bool m_bImplicitExecution = false;

    plHashedString m_sTargetTypeName;
    plVariant m_Value;

    plSmallArray<AstNode*, 4> m_Next;
    plSmallArray<DataInput, 5> m_Inputs;
    plSmallArray<DataOutput, 2> m_Outputs;
  };

#if PL_ENABLED(PL_PLATFORM_64BIT)
  static_assert(sizeof(AstNode) == 256);
#endif

private:
  using DataOffset = plVisualScriptDataDescription::DataOffset;

  PL_ALWAYS_INLINE static plStringView GetNiceTypeName(const plDocumentObject* pObject)
  {
    return plVisualScriptNodeManager::GetNiceTypeName(pObject);
  }

  PL_ALWAYS_INLINE plVisualScriptDataType::Enum GetDeductedType(const plDocumentObject* pObject) const
  {
    return m_pManager->GetDeductedType(pObject);
  }

  plUInt32 GetPinId(const plVisualScriptPin* pPin);
  DataOutput& GetDataOutput(const DataInput& dataInput);
  AstNode& CreateAstNode(plVisualScriptNodeDescription::Type::Enum type, plVisualScriptDataType::Enum deductedDataType = plVisualScriptDataType::Invalid, bool bImplicitExecution = false);
  PL_ALWAYS_INLINE AstNode& CreateAstNode(plVisualScriptNodeDescription::Type::Enum type, bool bImplicitExecution)
  {
    return CreateAstNode(type, plVisualScriptDataType::Invalid, bImplicitExecution);
  }

  void AddDataInput(AstNode& node, AstNode* pSourceNode, plUInt8 uiSourcePinIndex, plVisualScriptDataType::Enum dataType);
  void AddDataOutput(AstNode& node, plVisualScriptDataType::Enum dataType);

  struct DefaultInput
  {
    AstNode* m_pSourceNode = nullptr;
    plUInt8 m_uiSourcePinIndex = 0;
  };

  DefaultInput GetDefaultPointerInput(const plRTTI* pDataType);
  AstNode* CreateConstantNode(const plVariant& value);
  AstNode* CreateJumpNode(AstNode* pTargetNode);

  DataOffset GetInstanceDataOffset(plHashedString sName, plVisualScriptDataType::Enum dataType);

  struct ConnectionType
  {
    enum Enum
    {
      Execution,
      Data,
    };
  };

  struct Connection
  {
    AstNode* m_pPrev = nullptr;
    AstNode* m_pCurrent = nullptr;
    ConnectionType::Enum m_Type = ConnectionType::Execution;
    plUInt32 m_uiPrevPinIndex = 0;
  };

  AstNode* BuildAST(const plDocumentObject* pEntryNode);
  void MarkAsCoroutine(AstNode* pEntryAstNode);
  plResult ReplaceUnsupportedNodes(AstNode* pEntryAstNode);
  plResult ReplaceLoop(Connection& connection);
  plResult InsertTypeConversions(AstNode* pEntryAstNode);
  plResult InlineConstants(AstNode* pEntryAstNode);
  plResult InlineVariables(AstNode* pEntryAstNode);
  plResult BuildDataStack(AstNode* pEntryAstNode, plDynamicArray<AstNode*>& out_Stack);
  plResult BuildDataExecutions(AstNode* pEntryAstNode);
  plResult FillDataOutputConnections(AstNode* pEntryAstNode);
  plResult AssignLocalVariables(AstNode* pEntryAstNode, plVisualScriptDataDescription& inout_localDataDesc);
  plResult BuildNodeDescriptions(AstNode* pEntryAstNode, plDynamicArray<plVisualScriptNodeDescription>& out_NodeDescriptions);

  struct ConnectionHasher
  {
    static plUInt32 Hash(const Connection& c);
    static bool Equal(const Connection& a, const Connection& b);
  };

  enum class VisitorResult
  {
    Continue,
    Skip,
    Stop,
    Error,
  };

  using AstNodeVisitorFunc = plDelegate<VisitorResult(Connection& connection)>;
  plResult TraverseExecutionConnections(AstNode* pEntryAstNode, AstNodeVisitorFunc func, bool bDeduplicate = true);
  plResult TraverseDataConnections(AstNode* pEntryAstNode, AstNodeVisitorFunc func, bool bDeduplicate = true, bool bClearReportedConnections = true);
  plResult TraverseAllConnections(AstNode* pEntryAstNode, AstNodeVisitorFunc func, bool bDeduplicate = true);

  plResult FinalizeDataOffsets();
  plResult FinalizeConstantData();

  void DumpAST(AstNode* pEntryAstNode, plStringView sOutputPath, plStringView sFunctionName, plStringView sSuffix);
  void DumpGraph(plArrayPtr<const plVisualScriptNodeDescription> nodeDescriptions, plStringView sOutputPath, plStringView sFunctionName, plStringView sSuffix);

  const plVisualScriptNodeManager* m_pManager = nullptr;

  plDeque<AstNode> m_AstNodes;
  plHybridArray<AstNode*, 8> m_EntryAstNodes;
  plHashTable<const plRTTI*, DefaultInput> m_DefaultInputs;

  plHashSet<Connection, ConnectionHasher> m_ReportedConnections;

  plHashTable<const plVisualScriptPin*, plUInt32> m_PinToId;
  plUInt32 m_uiNextPinId = 0;

  struct DataDesc
  {
    PL_DECLARE_POD_TYPE();

    DataOffset m_DataOffset;
    plUInt32 m_uiUsageCounter = 0;
  };

  plHashTable<plUInt32, DataDesc> m_PinIdToDataDesc;

  plHashTable<plVariant, plUInt32> m_ConstantDataToIndex;

  CompiledModule m_Module;
};
