#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/Types/Delegate.h>

class plExpressionByteCode;

class PL_FOUNDATION_DLL plExpressionCompiler
{
public:
  plExpressionCompiler();
  ~plExpressionCompiler();

  plResult Compile(plExpressionAST& ref_ast, plExpressionByteCode& out_byteCode, plStringView sDebugAstOutputPath = plStringView());

private:
  plResult TransformAndOptimizeAST(plExpressionAST& ast, plStringView sDebugAstOutputPath);
  plResult BuildNodeInstructions(const plExpressionAST& ast);
  plResult UpdateRegisterLifetime(const plExpressionAST& ast);
  plResult AssignRegisters();
  plResult GenerateByteCode(const plExpressionAST& ast, plExpressionByteCode& out_byteCode);
  plResult GenerateConstantByteCode(const plExpressionAST::Constant* pConstant);

  using TransformFunc = plDelegate<plExpressionAST::Node*(plExpressionAST::Node*)>;
  plResult TransformASTPreOrder(plExpressionAST& ast, TransformFunc func);
  plResult TransformASTPostOrder(plExpressionAST& ast, TransformFunc func);
  plResult TransformNode(plExpressionAST::Node*& pNode, TransformFunc& func);
  plResult TransformOutputNode(plExpressionAST::Output*& pOutputNode, TransformFunc& func);

  void DumpAST(const plExpressionAST& ast, plStringView sOutputPath, plStringView sSuffix);

  plHybridArray<plExpressionAST::Node*, 64> m_NodeStack;
  plHybridArray<plExpressionAST::Node*, 64> m_NodeInstructions;
  plHashTable<const plExpressionAST::Node*, plUInt32> m_NodeToRegisterIndex;
  plHashTable<plExpressionAST::Node*, plExpressionAST::Node*> m_TransformCache;

  plHashTable<plHashedString, plUInt32> m_InputToIndex;
  plHashTable<plHashedString, plUInt32> m_OutputToIndex;
  plHashTable<plHashedString, plUInt32> m_FunctionToIndex;

  plDynamicArray<plUInt32> m_ByteCode;

  struct LiveInterval
  {
    PL_DECLARE_POD_TYPE();

    plUInt32 m_uiStart;
    plUInt32 m_uiEnd;
    const plExpressionAST::Node* m_pNode;
  };

  plDynamicArray<LiveInterval> m_LiveIntervals;
};
