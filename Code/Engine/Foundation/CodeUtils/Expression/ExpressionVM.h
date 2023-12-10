#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/Types/UniquePtr.h>

class PLASMA_FOUNDATION_DLL plExpressionVM
{
public:
  plExpressionVM();
  ~plExpressionVM();

  void RegisterFunction(const plExpressionFunction& func);
  void UnregisterFunction(const plExpressionFunction& func);

  plResult Execute(const plExpressionByteCode& byteCode, plArrayPtr<const plProcessingStream> inputs, plArrayPtr<plProcessingStream> outputs, plUInt32 uiNumInstances, const plExpression::GlobalData& globalData = plExpression::GlobalData());

private:
  void RegisterDefaultFunctions();

  static plResult ScalarizeStreams(plArrayPtr<const plProcessingStream> streams, plDynamicArray<plProcessingStream>& out_ScalarizedStreams);
  static plResult MapStreams(plArrayPtr<const plExpression::StreamDesc> streamDescs, plArrayPtr<plProcessingStream> streams, plStringView sStreamType, plUInt32 uiNumInstances, plDynamicArray<plProcessingStream*>& out_MappedStreams);
  plResult MapFunctions(plArrayPtr<const plExpression::FunctionDesc> functionDescs, const plExpression::GlobalData& globalData);

  plDynamicArray<plExpression::Register, plAlignedAllocatorWrapper> m_Registers;

  plDynamicArray<plProcessingStream> m_ScalarizedInputs;
  plDynamicArray<plProcessingStream> m_ScalarizedOutputs;

  plDynamicArray<plProcessingStream*> m_MappedInputs;
  plDynamicArray<plProcessingStream*> m_MappedOutputs;
  plDynamicArray<const plExpressionFunction*> m_MappedFunctions;

  plDynamicArray<plExpressionFunction> m_Functions;
  plHashTable<plHashedString, plUInt32> m_FunctionNamesToIndex;
};
