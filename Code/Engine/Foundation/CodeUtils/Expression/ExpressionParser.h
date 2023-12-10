#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>

class PLASMA_FOUNDATION_DLL plExpressionParser
{
public:
  plExpressionParser();
  ~plExpressionParser();

  void RegisterFunction(const plExpression::FunctionDesc& funcDesc);
  void UnregisterFunction(const plExpression::FunctionDesc& funcDesc);

  struct Options
  {
    bool m_bTreatUnknownVariablesAsInputs = false;
  };

  plResult Parse(plStringView sCode, plArrayPtr<plExpression::StreamDesc> inputs, plArrayPtr<plExpression::StreamDesc> outputs, const Options& options, plExpressionAST& out_ast);

private:
  static constexpr int s_iLowestPrecedence = 20;

  void RegisterKnownTypes();
  void RegisterBuiltinFunctions();
  void SetupInAndOutputs(plArrayPtr<plExpression::StreamDesc> inputs, plArrayPtr<plExpression::StreamDesc> outputs);

  plResult ParseStatement();
  plResult ParseType(plStringView sTypeName, plEnum<plExpressionAST::DataType>& out_type);
  plResult ParseVariableDefinition(plEnum<plExpressionAST::DataType> type);
  plResult ParseAssignment();

  plExpressionAST::Node* ParseFactor();
  plExpressionAST::Node* ParseExpression(int iPrecedence = s_iLowestPrecedence);
  plExpressionAST::Node* ParseUnaryExpression();
  plExpressionAST::Node* ParseFunctionCall(plStringView sFunctionName);
  plExpressionAST::Node* ParseSwizzle(plExpressionAST::Node* pExpression);

  bool AcceptStatementTerminator();
  bool AcceptOperator(plStringView sName);
  bool AcceptBinaryOperator(plExpressionAST::NodeType::Enum& out_binaryOp, int& out_iOperatorPrecedence, plUInt32& out_uiOperatorLength);
  plExpressionAST::Node* GetVariable(plStringView sVarName);
  plExpressionAST::Node* EnsureExpectedType(plExpressionAST::Node* pNode, plExpressionAST::DataType::Enum expectedType);
  plExpressionAST::Node* Unpack(plExpressionAST::Node* pNode, bool bUnassignedError = true);

  plResult Expect(plStringView sToken, const plToken** pExpectedToken = nullptr);
  plResult Expect(plTokenType::Enum Type, const plToken** pExpectedToken = nullptr);

  void ReportError(const plToken* pToken, const plFormatString& message);

  /// \brief Checks whether all outputs have been written
  plResult CheckOutputs();

  Options m_Options;

  plTokenParseUtils::TokenStream m_TokenStream;
  plUInt32 m_uiCurrentToken = 0;
  plExpressionAST* m_pAST = nullptr;

  plHashTable<plHashedString, plEnum<plExpressionAST::DataType>> m_KnownTypes;

  plHashTable<plHashedString, plExpressionAST::Node*> m_KnownVariables;
  plHashTable<plHashedString, plEnum<plExpressionAST::NodeType>> m_BuiltinFunctions;
  plHashTable<plHashedString, plHybridArray<plExpression::FunctionDesc, 1>> m_FunctionDescs;
};

#include <Foundation/CodeUtils/Expression/Implementation/ExpressionParser_inl.h>
