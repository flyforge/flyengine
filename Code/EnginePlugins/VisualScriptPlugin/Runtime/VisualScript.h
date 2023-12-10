#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScriptData.h>

class plVisualScriptInstance;
class plVisualScriptExecutionContext;

struct PLASMA_VISUALSCRIPTPLUGIN_DLL plVisualScriptNodeDescription
{
  /// \brief Native node types for visual script graphs.
  /// Editor only types are not supported at runtime and will be replaced by the visual script compiler during asset transform.
  struct PLASMA_VISUALSCRIPTPLUGIN_DLL Type
  {
    using StorageType = plUInt8;

    enum Enum
    {
      Invalid,
      EntryCall,
      EntryCall_Coroutine,
      MessageHandler,
      MessageHandler_Coroutine,
      ReflectedFunction,
      GetReflectedProperty,
      SetReflectedProperty,
      InplaceCoroutine,
      GetScriptOwner,
      SendMessage,

      FirstBuiltin,

      Builtin_Constant, // Editor Only
      Builtin_GetVariable, // Editor Only
      Builtin_SetVariable,
      Builtin_IncVariable,
      Builtin_DecVariable,

      Builtin_Branch,
      Builtin_Switch,
      Builtin_WhileLoop,          // Editor only
      Builtin_ForLoop,            // Editor only
      Builtin_ForEachLoop,        // Editor only
      Builtin_ReverseForEachLoop, // Editor only
      Builtin_Break,              // Editor only
      Builtin_Jump,               // Editor only

      Builtin_And,
      Builtin_Or,
      Builtin_Not,
      Builtin_Compare,
      Builtin_CompareExec, // Editor only
      Builtin_IsValid,
      Builtin_Select, // TODO

      Builtin_Add,
      Builtin_Subtract,
      Builtin_Multiply,
      Builtin_Divide,
      Builtin_Expression, // TODO

      Builtin_ToBool,
      Builtin_ToByte,
      Builtin_ToInt,
      Builtin_ToInt64,
      Builtin_ToFloat,
      Builtin_ToDouble,
      Builtin_ToString,
      Builtin_String_Format,
      Builtin_ToHashedString,
      Builtin_ToVariant,
      Builtin_Variant_ConvertTo,

      Builtin_MakeArray,
      Builtin_Array_GetElement, // TODO
      Builtin_Array_SetElement, // TODO
      Builtin_Array_GetCount,   // TODO
      Builtin_Array_IsEmpty,    // TODO
      Builtin_Array_Clear,      // TODO
      Builtin_Array_Contains,   // TODO
      Builtin_Array_IndexOf,    // TODO
      Builtin_Array_Insert,     // TODO
      Builtin_Array_PushBack,   // TODO
      Builtin_Array_Remove,     // TODO
      Builtin_Array_RemoveAt,   // TODO

      Builtin_TryGetComponentOfBaseType,

      Builtin_StartCoroutine,
      Builtin_StopCoroutine,
      Builtin_StopAllCoroutines,
      Builtin_WaitForAll,
      Builtin_WaitForAny,
      Builtin_Yield,

      LastBuiltin,

      Count,
      Default = Invalid
    };

    PLASMA_ALWAYS_INLINE static bool IsEntry(Enum type) { return type >= EntryCall && type <= MessageHandler_Coroutine; }
    PLASMA_ALWAYS_INLINE static bool IsLoop(Enum type) { return type >= Builtin_WhileLoop && type <= Builtin_ReverseForEachLoop; }

    PLASMA_ALWAYS_INLINE static bool MakesOuterCoroutine(Enum type) { return type == InplaceCoroutine || (type >= Builtin_WaitForAll && type <= Builtin_Yield); }

    PLASMA_ALWAYS_INLINE static bool IsBuiltin(Enum type) { return type > FirstBuiltin && type < LastBuiltin; }

    static Enum GetConversionType(plVisualScriptDataType::Enum targetDataType);

    static const char* GetName(Enum type);
  };

  using DataOffset = plVisualScriptDataDescription::DataOffset;

  plEnum<Type> m_Type;
  plEnum<plVisualScriptDataType> m_DeductedDataType;
  plSmallArray<plUInt16, 4> m_ExecutionIndices;
  plSmallArray<DataOffset, 4> m_InputDataOffsets;
  plSmallArray<DataOffset, 2> m_OutputDataOffsets;

  plHashedString m_sTargetTypeName;

  plVariant m_Value;

  void AppendUserDataName(plStringBuilder& out_sResult) const;
};

class PLASMA_VISUALSCRIPTPLUGIN_DLL plVisualScriptGraphDescription : public plRefCounted
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plVisualScriptGraphDescription);

public:
  plVisualScriptGraphDescription();
  ~plVisualScriptGraphDescription();

  static plResult Serialize(plArrayPtr<const plVisualScriptNodeDescription> nodes, const plVisualScriptDataDescription& localDataDesc, plStreamWriter& inout_stream);
  plResult Deserialize(plStreamReader& inout_stream);

  template <typename T, plUInt32 Size>
  struct EmbeddedArrayOrPointer
  {
    union
    {
      T m_Embedded[Size] = {};
      T* m_Ptr;
    };

    static void AddAdditionalDataSize(plArrayPtr<const T> a, plUInt32& inout_uiAdditionalDataSize);
    static void AddAdditionalDataSize(plUInt32 uiSize, plUInt32 uiAlignment, plUInt32& inout_uiAdditionalDataSize);

    T* Init(plUInt8 uiCount, plUInt8*& inout_pAdditionalData);
    plResult ReadFromStream(plUInt8& out_uiCount, plStreamReader& inout_stream, plUInt8*& inout_pAdditionalData);
  };

  struct ExecResult
  {
    struct State
    {
      enum Enum
      {
        Completed = 0,
        ContinueLater = -1,

        Error = -100,
      };
    };

    static PLASMA_ALWAYS_INLINE ExecResult Completed() { return ExecResult::Completed(); }
    static PLASMA_ALWAYS_INLINE ExecResult RunNext(int iExecSlot) { return {iExecSlot}; }
    static PLASMA_ALWAYS_INLINE ExecResult ContinueLater(plTime maxDelay) { return {State::ContinueLater, maxDelay}; }
    static PLASMA_ALWAYS_INLINE ExecResult Error() { return {State::Error}; }

    int m_NextExecAndState = 0;
    plTime m_MaxDelay = plTime::MakeZero();
  };

  struct Node;
  using ExecuteFunction = ExecResult (*)(plVisualScriptExecutionContext& inout_context, const Node& node);
  using DataOffset = plVisualScriptDataDescription::DataOffset;
  using ExecutionIndicesArray = EmbeddedArrayOrPointer<plUInt16, 4>;
  using InputDataOffsetsArray = EmbeddedArrayOrPointer<DataOffset, 4>;
  using OutputDataOffsetsArray = EmbeddedArrayOrPointer<DataOffset, 2>;
  using UserDataArray = EmbeddedArrayOrPointer<plUInt32, 4>;

  struct Node
  {
    ExecuteFunction m_Function = nullptr;
#if PLASMA_ENABLED(PLASMA_PLATFORM_32BIT)
    plUInt32 m_uiPadding = 0;
#endif

    ExecutionIndicesArray m_ExecutionIndices;
    InputDataOffsetsArray m_InputDataOffsets;
    OutputDataOffsetsArray m_OutputDataOffsets;
    UserDataArray m_UserData;

    plEnum<plVisualScriptNodeDescription::Type> m_Type;
    plUInt8 m_NumExecutionIndices;
    plUInt8 m_NumInputDataOffsets;
    plUInt8 m_NumOutputDataOffsets;

    plUInt16 m_UserDataByteSize;
    plEnum<plVisualScriptDataType> m_DeductedDataType;
    plUInt8 m_Reserved = 0;

    plUInt32 GetExecutionIndex(plUInt32 uiSlot) const;
    DataOffset GetInputDataOffset(plUInt32 uiSlot) const;
    DataOffset GetOutputDataOffset(plUInt32 uiSlot) const;

    template <typename T>
    const T& GetUserData() const;

    template <typename T>
    T& InitUserData(plUInt8*& inout_pAdditionalData, plUInt32 uiByteSize = sizeof(T));
  };

  const Node* GetNode(plUInt32 uiIndex) const;

  bool IsCoroutine() const;
  plScriptMessageDesc GetMessageDesc() const;

  const plSharedPtr<const plVisualScriptDataDescription>& GetLocalDataDesc() const;

private:
  plArrayPtr<const Node> m_Nodes;
  plBlob m_Storage;

  plSharedPtr<const plVisualScriptDataDescription> m_pLocalDataDesc;
};


class PLASMA_VISUALSCRIPTPLUGIN_DLL plVisualScriptExecutionContext
{
public:
  plVisualScriptExecutionContext(const plSharedPtr<const plVisualScriptGraphDescription>& pDesc);
  ~plVisualScriptExecutionContext();

  void Initialize(plVisualScriptInstance& inout_instance, plVisualScriptDataStorage& inout_localDataStorage, plArrayPtr<plVariant> arguments);
  void Deinitialize();

  using ExecResult = plVisualScriptGraphDescription::ExecResult;
  ExecResult Execute(plTime deltaTimeSinceLastExecution);

  plVisualScriptInstance& GetInstance() { return *m_pInstance; }

  using DataOffset = plVisualScriptDataDescription::DataOffset;

  template <typename T>
  const T& GetData(DataOffset dataOffset) const;

  template <typename T>
  T& GetWritableData(DataOffset dataOffset);

  template <typename T>
  void SetData(DataOffset dataOffset, const T& value);

  plTypedPointer GetPointerData(DataOffset dataOffset);

  template <typename T>
  void SetPointerData(DataOffset dataOffset, T ptr, const plRTTI* pType = nullptr);

  plVariant GetDataAsVariant(DataOffset dataOffset, const plRTTI* pExpectedType) const;
  void SetDataFromVariant(DataOffset dataOffset, const plVariant& value);

  plScriptCoroutine* GetCurrentCoroutine() { return m_pCurrentCoroutine; }
  void SetCurrentCoroutine(plScriptCoroutine* pCoroutine);

  plTime GetDeltaTimeSinceLastExecution();

private:
  plSharedPtr<const plVisualScriptGraphDescription> m_pDesc;
  plVisualScriptInstance* m_pInstance = nullptr;
  plUInt32 m_uiCurrentNode = 0;
  plUInt32 m_uiExecutionCounter = 0;
  plTime m_DeltaTimeSinceLastExecution;

  plVisualScriptDataStorage* m_DataStorage[DataOffset::Source::Count] = {};

  plScriptCoroutine* m_pCurrentCoroutine = nullptr;
};

struct plVisualScriptSendMessageMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    Direct,    ///< Directly send the message to the target game object
    Recursive, ///< Send the message to the target game object and its children
    Event,     ///< Send the message as event. \sa plGameObject::SendEventMessage()

    Default = Direct
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_VISUALSCRIPTPLUGIN_DLL, plVisualScriptSendMessageMode);

#include <VisualScriptPlugin/Runtime/VisualScript_inl.h>
