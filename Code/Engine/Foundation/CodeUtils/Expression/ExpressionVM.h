#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/Types/UniquePtr.h>

class PL_FOUNDATION_DLL plExpressionVM
{
public:
  plExpressionVM();
  ~plExpressionVM();

  void RegisterFunction(const plExpressionFunction& func);
  void UnregisterFunction(const plExpressionFunction& func);

  struct Flags
  {
    using StorageType = plUInt32;

    enum Enum
    {
      MapStreamsByName = PL_BIT(0),
      ScalarizeStreams = PL_BIT(1),

      UserFriendly = MapStreamsByName | ScalarizeStreams,
      BestPerformance = 0,

      Default = UserFriendly
    };

    struct Bits
    {
      StorageType MapStreamsByName : 1;
      StorageType ScalarizeStreams : 1;
    };
  };

  plResult Execute(const plExpressionByteCode& byteCode, plArrayPtr<const plProcessingStream> inputs, plArrayPtr<plProcessingStream> outputs, plUInt32 uiNumInstances, const plExpression::GlobalData& globalData = plExpression::GlobalData(), plBitflags<Flags> flags = Flags::Default);

private:
  void RegisterDefaultFunctions();

  static plResult ScalarizeStreams(plArrayPtr<const plProcessingStream> streams, plDynamicArray<plProcessingStream>& out_ScalarizedStreams);
  static plResult AreStreamsScalarized(plArrayPtr<const plProcessingStream> streams);
  static plResult ValidateStream(const plProcessingStream& stream, const plExpression::StreamDesc& streamDesc, plStringView sStreamType, plUInt32 uiNumInstances);

  template <typename T>
  static plResult MapStreams(plArrayPtr<const plExpression::StreamDesc> streamDescs, plArrayPtr<T> streams, plStringView sStreamType, plUInt32 uiNumInstances, plBitflags<Flags> flags, plDynamicArray<T*>& out_MappedStreams);
  plResult MapFunctions(plArrayPtr<const plExpression::FunctionDesc> functionDescs, const plExpression::GlobalData& globalData);

  plDynamicArray<plExpression::Register, plAlignedAllocatorWrapper> m_Registers;

  plDynamicArray<plProcessingStream> m_ScalarizedInputs;
  plDynamicArray<plProcessingStream> m_ScalarizedOutputs;

  plDynamicArray<const plProcessingStream*> m_MappedInputs;
  plDynamicArray<plProcessingStream*> m_MappedOutputs;
  plDynamicArray<const plExpressionFunction*> m_MappedFunctions;

  plDynamicArray<plExpressionFunction> m_Functions;
  plHashTable<plHashedString, plUInt32> m_FunctionNamesToIndex;
};
