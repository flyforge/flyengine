#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/CodeUtils/Tokenizer.h>

namespace
{
  struct AssignOperator
  {
    plStringView m_sName;
    plExpressionAST::NodeType::Enum m_NodeType;
  };

  static constexpr AssignOperator s_assignOperators[] = {
    {"+="_plsv, plExpressionAST::NodeType::Add},
    {"-="_plsv, plExpressionAST::NodeType::Subtract},
    {"*="_plsv, plExpressionAST::NodeType::Multiply},
    {"/="_plsv, plExpressionAST::NodeType::Divide},
    {"%="_plsv, plExpressionAST::NodeType::Modulo},
    {"<<="_plsv, plExpressionAST::NodeType::BitshiftLeft},
    {">>="_plsv, plExpressionAST::NodeType::BitshiftRight},
    {"&="_plsv, plExpressionAST::NodeType::BitwiseAnd},
    {"^="_plsv, plExpressionAST::NodeType::BitwiseXor},
    {"|="_plsv, plExpressionAST::NodeType::BitwiseOr},
  };

  struct BinaryOperator
  {
    plStringView m_sName;
    plExpressionAST::NodeType::Enum m_NodeType;
    int m_iPrecedence;
  };

  // Operator precedence according to https://en.cppreference.com/w/cpp/language/operator_precedence,
  // lower value means higher precedence
  // sorted by string length to simplify the test against a token stream
  static constexpr BinaryOperator s_binaryOperators[] = {
    {"&&"_plsv, plExpressionAST::NodeType::LogicalAnd, 14},
    {"||"_plsv, plExpressionAST::NodeType::LogicalOr, 15},
    {"<<"_plsv, plExpressionAST::NodeType::BitshiftLeft, 7},
    {">>"_plsv, plExpressionAST::NodeType::BitshiftRight, 7},
    {"=="_plsv, plExpressionAST::NodeType::Equal, 10},
    {"!="_plsv, plExpressionAST::NodeType::NotEqual, 10},
    {"<="_plsv, plExpressionAST::NodeType::LessEqual, 9},
    {">="_plsv, plExpressionAST::NodeType::GreaterEqual, 9},
    {"<"_plsv, plExpressionAST::NodeType::Less, 9},
    {">"_plsv, plExpressionAST::NodeType::Greater, 9},
    {"&"_plsv, plExpressionAST::NodeType::BitwiseAnd, 11},
    {"^"_plsv, plExpressionAST::NodeType::BitwiseXor, 12},
    {"|"_plsv, plExpressionAST::NodeType::BitwiseOr, 13},
    {"?"_plsv, plExpressionAST::NodeType::Select, 16},
    {"+"_plsv, plExpressionAST::NodeType::Add, 6},
    {"-"_plsv, plExpressionAST::NodeType::Subtract, 6},
    {"*"_plsv, plExpressionAST::NodeType::Multiply, 5},
    {"/"_plsv, plExpressionAST::NodeType::Divide, 5},
    {"%"_plsv, plExpressionAST::NodeType::Modulo, 5},
  };

} // namespace

using namespace plTokenParseUtils;

plExpressionParser::plExpressionParser()
{
  RegisterKnownTypes();
  RegisterBuiltinFunctions();
}

plExpressionParser::~plExpressionParser() = default;

void plExpressionParser::RegisterFunction(const plExpression::FunctionDesc& funcDesc)
{
  PLASMA_ASSERT_DEV(funcDesc.m_uiNumRequiredInputs <= funcDesc.m_InputTypes.GetCount(), "Not enough input types defined. {} inputs are required but only {} types given.", funcDesc.m_uiNumRequiredInputs, funcDesc.m_InputTypes.GetCount());

  auto& functionDescs = m_FunctionDescs[funcDesc.m_sName];
  if (functionDescs.Contains(funcDesc) == false)
  {
    functionDescs.PushBack(funcDesc);
  }
}

void plExpressionParser::UnregisterFunction(const plExpression::FunctionDesc& funcDesc)
{
  if (auto pFunctionDescs = m_FunctionDescs.GetValue(funcDesc.m_sName))
  {
    pFunctionDescs->RemoveAndCopy(funcDesc);
  }
}

plResult plExpressionParser::Parse(plStringView sCode, plArrayPtr<plExpression::StreamDesc> inputs, plArrayPtr<plExpression::StreamDesc> outputs, const Options& options, plExpressionAST& out_ast)
{
  if (sCode.IsEmpty())
    return PLASMA_FAILURE;

  m_Options = options;

  m_pAST = &out_ast;
  SetupInAndOutputs(inputs, outputs);

  plTokenizer tokenizer;
  tokenizer.Tokenize(plArrayPtr<const plUInt8>((const plUInt8*)sCode.GetStartPointer(), sCode.GetElementCount()), plLog::GetThreadLocalLogSystem());

  plUInt32 readTokens = 0;
  while (tokenizer.GetNextLine(readTokens, m_TokenStream).Succeeded())
  {
    m_uiCurrentToken = 0;

    while (m_uiCurrentToken < m_TokenStream.GetCount())
    {
      PLASMA_SUCCEED_OR_RETURN(ParseStatement());

      if (m_uiCurrentToken < m_TokenStream.GetCount() && AcceptStatementTerminator() == false)
      {
        auto pCurrentToken = m_TokenStream[m_uiCurrentToken];
        ReportError(pCurrentToken, plFmt("Syntax error, unexpected token '{}'", pCurrentToken->m_DataView));
        return PLASMA_FAILURE;
      }
    }
  }

  PLASMA_SUCCEED_OR_RETURN(CheckOutputs());

  return PLASMA_SUCCESS;
}

void plExpressionParser::RegisterKnownTypes()
{
  m_KnownTypes.Insert(plMakeHashedString("var"), plExpressionAST::DataType::Unknown);

  m_KnownTypes.Insert(plMakeHashedString("vec2"), plExpressionAST::DataType::Float2);
  m_KnownTypes.Insert(plMakeHashedString("vec3"), plExpressionAST::DataType::Float3);
  m_KnownTypes.Insert(plMakeHashedString("vec4"), plExpressionAST::DataType::Float4);

  m_KnownTypes.Insert(plMakeHashedString("vec2i"), plExpressionAST::DataType::Int2);
  m_KnownTypes.Insert(plMakeHashedString("vec3i"), plExpressionAST::DataType::Int3);
  m_KnownTypes.Insert(plMakeHashedString("vec4i"), plExpressionAST::DataType::Int4);

  plStringBuilder sTypeName;
  for (plUInt32 type = plExpressionAST::DataType::Bool; type < plExpressionAST::DataType::Count; ++type)
  {
    sTypeName = plExpressionAST::DataType::GetName(static_cast<plExpressionAST::DataType::Enum>(type));
    sTypeName.ToLower();

    plHashedString sTypeNameHashed;
    sTypeNameHashed.Assign(sTypeName);

    m_KnownTypes.Insert(sTypeNameHashed, static_cast<plExpressionAST::DataType::Enum>(type));
  }
}

void plExpressionParser::RegisterBuiltinFunctions()
{
  // Unary
  m_BuiltinFunctions.Insert(plMakeHashedString("abs"), plExpressionAST::NodeType::Absolute);
  m_BuiltinFunctions.Insert(plMakeHashedString("saturate"), plExpressionAST::NodeType::Saturate);
  m_BuiltinFunctions.Insert(plMakeHashedString("sqrt"), plExpressionAST::NodeType::Sqrt);
  m_BuiltinFunctions.Insert(plMakeHashedString("exp"), plExpressionAST::NodeType::Exp);
  m_BuiltinFunctions.Insert(plMakeHashedString("ln"), plExpressionAST::NodeType::Ln);
  m_BuiltinFunctions.Insert(plMakeHashedString("log2"), plExpressionAST::NodeType::Log2);
  m_BuiltinFunctions.Insert(plMakeHashedString("log10"), plExpressionAST::NodeType::Log10);
  m_BuiltinFunctions.Insert(plMakeHashedString("pow2"), plExpressionAST::NodeType::Pow2);
  m_BuiltinFunctions.Insert(plMakeHashedString("sin"), plExpressionAST::NodeType::Sin);
  m_BuiltinFunctions.Insert(plMakeHashedString("cos"), plExpressionAST::NodeType::Cos);
  m_BuiltinFunctions.Insert(plMakeHashedString("tan"), plExpressionAST::NodeType::Tan);
  m_BuiltinFunctions.Insert(plMakeHashedString("asin"), plExpressionAST::NodeType::ASin);
  m_BuiltinFunctions.Insert(plMakeHashedString("acos"), plExpressionAST::NodeType::ACos);
  m_BuiltinFunctions.Insert(plMakeHashedString("atan"), plExpressionAST::NodeType::ATan);
  m_BuiltinFunctions.Insert(plMakeHashedString("radToDeg"), plExpressionAST::NodeType::RadToDeg);
  m_BuiltinFunctions.Insert(plMakeHashedString("rad_to_deg"), plExpressionAST::NodeType::RadToDeg);
  m_BuiltinFunctions.Insert(plMakeHashedString("degToRad"), plExpressionAST::NodeType::DegToRad);
  m_BuiltinFunctions.Insert(plMakeHashedString("deg_to_rad"), plExpressionAST::NodeType::DegToRad);
  m_BuiltinFunctions.Insert(plMakeHashedString("round"), plExpressionAST::NodeType::Round);
  m_BuiltinFunctions.Insert(plMakeHashedString("floor"), plExpressionAST::NodeType::Floor);
  m_BuiltinFunctions.Insert(plMakeHashedString("ceil"), plExpressionAST::NodeType::Ceil);
  m_BuiltinFunctions.Insert(plMakeHashedString("trunc"), plExpressionAST::NodeType::Trunc);
  m_BuiltinFunctions.Insert(plMakeHashedString("frac"), plExpressionAST::NodeType::Frac);
  m_BuiltinFunctions.Insert(plMakeHashedString("length"), plExpressionAST::NodeType::Length);
  m_BuiltinFunctions.Insert(plMakeHashedString("normalize"), plExpressionAST::NodeType::Normalize);
  m_BuiltinFunctions.Insert(plMakeHashedString("all"), plExpressionAST::NodeType::All);
  m_BuiltinFunctions.Insert(plMakeHashedString("any"), plExpressionAST::NodeType::Any);

  // Binary
  m_BuiltinFunctions.Insert(plMakeHashedString("mod"), plExpressionAST::NodeType::Modulo);
  m_BuiltinFunctions.Insert(plMakeHashedString("log"), plExpressionAST::NodeType::Log);
  m_BuiltinFunctions.Insert(plMakeHashedString("pow"), plExpressionAST::NodeType::Pow);
  m_BuiltinFunctions.Insert(plMakeHashedString("min"), plExpressionAST::NodeType::Min);
  m_BuiltinFunctions.Insert(plMakeHashedString("max"), plExpressionAST::NodeType::Max);
  m_BuiltinFunctions.Insert(plMakeHashedString("dot"), plExpressionAST::NodeType::Dot);
  m_BuiltinFunctions.Insert(plMakeHashedString("cross"), plExpressionAST::NodeType::Cross);
  m_BuiltinFunctions.Insert(plMakeHashedString("reflect"), plExpressionAST::NodeType::Reflect);

  // Ternary
  m_BuiltinFunctions.Insert(plMakeHashedString("clamp"), plExpressionAST::NodeType::Clamp);
  m_BuiltinFunctions.Insert(plMakeHashedString("lerp"), plExpressionAST::NodeType::Lerp);
}

void plExpressionParser::SetupInAndOutputs(plArrayPtr<plExpression::StreamDesc> inputs, plArrayPtr<plExpression::StreamDesc> outputs)
{
  m_KnownVariables.Clear();

  for (auto& inputDesc : inputs)
  {
    auto pInput = m_pAST->CreateInput(inputDesc);
    m_KnownVariables.Insert(inputDesc.m_sName, pInput);
  }

  for (auto& outputDesc : outputs)
  {
    auto pOutputNode = m_pAST->CreateOutput(outputDesc, nullptr);
    m_pAST->m_OutputNodes.PushBack(pOutputNode);
    m_KnownVariables.Insert(outputDesc.m_sName, pOutputNode);
  }
}

plResult plExpressionParser::ParseStatement()
{
  SkipWhitespace(m_TokenStream, m_uiCurrentToken);

  if (AcceptStatementTerminator())
  {
    // empty statement
    return PLASMA_SUCCESS;
  }

  if (m_uiCurrentToken >= m_TokenStream.GetCount())
    return PLASMA_FAILURE;

  const plToken* pIdentifierToken = m_TokenStream[m_uiCurrentToken];
  if (pIdentifierToken->m_iType != plTokenType::Identifier)
  {
    ReportError(pIdentifierToken, "Syntax error, expected type or variable");
  }

  plEnum<plExpressionAST::DataType> type;
  if (ParseType(pIdentifierToken->m_DataView, type).Succeeded())
  {
    return ParseVariableDefinition(type);
  }

  return ParseAssignment();
}

plResult plExpressionParser::ParseType(plStringView sTypeName, plEnum<plExpressionAST::DataType>& out_type)
{
  plTempHashedString sTypeNameHashed(sTypeName);
  if (m_KnownTypes.TryGetValue(sTypeNameHashed, out_type))
  {
    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
}

plResult plExpressionParser::ParseVariableDefinition(plEnum<plExpressionAST::DataType> type)
{
  // skip type
  PLASMA_SUCCEED_OR_RETURN(Expect(plTokenType::Identifier));

  const plToken* pIdentifierToken = nullptr;
  PLASMA_SUCCEED_OR_RETURN(Expect(plTokenType::Identifier, &pIdentifierToken));

  plHashedString sHashedVarName;
  sHashedVarName.Assign(pIdentifierToken->m_DataView);

  plExpressionAST::Node* pVariableNode;
  if (m_KnownVariables.TryGetValue(sHashedVarName, pVariableNode))
  {
    const char* szExisting = "a variable";
    if (plExpressionAST::NodeType::IsInput(pVariableNode->m_Type))
    {
      szExisting = "an input";
    }
    else if (plExpressionAST::NodeType::IsOutput(pVariableNode->m_Type))
    {
      szExisting = "an output";
    }

    ReportError(pIdentifierToken, plFmt("Local variable '{}' cannot be defined because {} of the same name already exists", pIdentifierToken->m_DataView, szExisting));
    return PLASMA_FAILURE;
  }

  PLASMA_SUCCEED_OR_RETURN(Expect("="));
  plExpressionAST::Node* pExpression = ParseExpression();
  if (pExpression == nullptr)
    return PLASMA_FAILURE;

  m_KnownVariables.Insert(sHashedVarName, EnsureExpectedType(pExpression, type));
  return PLASMA_SUCCESS;
}

plResult plExpressionParser::ParseAssignment()
{
  const plToken* pIdentifierToken = nullptr;
  PLASMA_SUCCEED_OR_RETURN(Expect(plTokenType::Identifier, &pIdentifierToken));

  const plStringView sIdentifier = pIdentifierToken->m_DataView;
  plExpressionAST::Node* pVarNode = GetVariable(sIdentifier);
  if (pVarNode == nullptr)
  {
    ReportError(pIdentifierToken, "Syntax error, expected a valid variable");
    return PLASMA_FAILURE;
  }

  plStringView sPartialAssignmentMask;
  if (Accept(m_TokenStream, m_uiCurrentToken, "."))
  {
    const plToken* pSwizzleToken = nullptr;
    if (Expect(plTokenType::Identifier, &pSwizzleToken).Failed())
    {
      ReportError(m_TokenStream[m_uiCurrentToken], "Invalid partial assignment");
      return PLASMA_FAILURE;
    }

    sPartialAssignmentMask = pSwizzleToken->m_DataView;
  }

  SkipWhitespace(m_TokenStream, m_uiCurrentToken);

  plExpressionAST::NodeType::Enum assignOperator = plExpressionAST::NodeType::Invalid;
  for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(s_assignOperators); ++i)
  {
    auto& op = s_assignOperators[i];
    if (AcceptOperator(op.m_sName))
    {
      assignOperator = op.m_NodeType;
      m_uiCurrentToken += op.m_sName.GetElementCount();
      break;
    }
  }

  if (assignOperator == plExpressionAST::NodeType::Invalid)
  {
    PLASMA_SUCCEED_OR_RETURN(Expect("="));
  }

  plExpressionAST::Node* pExpression = ParseExpression();
  if (pExpression == nullptr)
    return PLASMA_FAILURE;

  if (assignOperator != plExpressionAST::NodeType::Invalid)
  {
    pExpression = m_pAST->CreateBinaryOperator(assignOperator, Unpack(pVarNode), pExpression);
  }

  if (sPartialAssignmentMask.IsEmpty() == false)
  {
    auto pConstructor = m_pAST->CreateConstructorCall(Unpack(pVarNode, false), pExpression, sPartialAssignmentMask);
    if (pConstructor == nullptr)
    {
      ReportError(pIdentifierToken, plFmt("Invalid partial assignment .{} = {}", sPartialAssignmentMask, plExpressionAST::DataType::GetName(pExpression->m_ReturnType)));
      return PLASMA_FAILURE;
    }

    pExpression = pConstructor;
  }

  if (plExpressionAST::NodeType::IsInput(pVarNode->m_Type))
  {
    ReportError(pIdentifierToken, plFmt("Input '{}' is not assignable", sIdentifier));
    return PLASMA_FAILURE;
  }
  else if (plExpressionAST::NodeType::IsOutput(pVarNode->m_Type))
  {
    auto pOutput = static_cast<plExpressionAST::Output*>(pVarNode);
    pOutput->m_pExpression = pExpression;
    return PLASMA_SUCCESS;
  }

  plHashedString sHashedVarName;
  sHashedVarName.Assign(sIdentifier);
  m_KnownVariables[sHashedVarName] = EnsureExpectedType(pExpression, pVarNode->m_ReturnType);
  return PLASMA_SUCCESS;
}

plExpressionAST::Node* plExpressionParser::ParseFactor()
{
  plUInt32 uiIdentifierToken = 0;
  if (Accept(m_TokenStream, m_uiCurrentToken, plTokenType::Identifier, &uiIdentifierToken))
  {
    auto pIdentifierToken = m_TokenStream[uiIdentifierToken];
    const plStringView sIdentifier = pIdentifierToken->m_DataView;

    if (Accept(m_TokenStream, m_uiCurrentToken, "("))
    {
      return ParseSwizzle(ParseFunctionCall(sIdentifier));
    }
    else if (sIdentifier == "true")
    {
      return m_pAST->CreateConstant(true, plExpressionAST::DataType::Bool);
    }
    else if (sIdentifier == "false")
    {
      return m_pAST->CreateConstant(false, plExpressionAST::DataType::Bool);
    }
    else if (sIdentifier == "PI")
    {
      return m_pAST->CreateConstant(plMath::Pi<float>(), plExpressionAST::DataType::Float);
    }
    else
    {
      auto pVariable = GetVariable(sIdentifier);
      if (pVariable == nullptr)
      {
        ReportError(pIdentifierToken, plFmt("Undeclared identifier '{}'", sIdentifier));
        return nullptr;
      }
      return ParseSwizzle(Unpack(pVariable));
    }
  }

  plUInt32 uiValueToken = 0;
  if (Accept(m_TokenStream, m_uiCurrentToken, plTokenType::Integer, &uiValueToken))
  {
    const plString sVal = m_TokenStream[uiValueToken]->m_DataView;

    plInt64 iConstant = 0;
    if (sVal.StartsWith_NoCase("0x"))
    {
      plUInt64 uiHexConstant = 0;
      plConversionUtils::ConvertHexStringToUInt64(sVal, uiHexConstant).IgnoreResult();
      iConstant = uiHexConstant;
    }
    else
    {
      plConversionUtils::StringToInt64(sVal, iConstant).IgnoreResult();
    }

    return m_pAST->CreateConstant((int)iConstant, plExpressionAST::DataType::Int);
  }
  else if (Accept(m_TokenStream, m_uiCurrentToken, plTokenType::Float, &uiValueToken))
  {
    const plString sVal = m_TokenStream[uiValueToken]->m_DataView;

    double fConstant = 0;
    plConversionUtils::StringToFloat(sVal, fConstant).IgnoreResult();

    return m_pAST->CreateConstant((float)fConstant, plExpressionAST::DataType::Float);
  }

  if (Accept(m_TokenStream, m_uiCurrentToken, "("))
  {
    auto pExpression = ParseExpression();
    if (Expect(")").Failed())
      return nullptr;

    return ParseSwizzle(pExpression);
  }

  return nullptr;
}

// Parsing the expression - recursive parser using "precedence climbing".
// http://www.engr.mun.ca/~theo/Misc/exp_parsing.htm
plExpressionAST::Node* plExpressionParser::ParseExpression(int iPrecedence /* = s_iLowestPrecedence*/)
{
  auto pExpression = ParseUnaryExpression();
  if (pExpression == nullptr)
    return nullptr;

  plExpressionAST::NodeType::Enum binaryOp;
  int iBinaryOpPrecedence = 0;
  plUInt32 uiOperatorLength = 0;
  while (AcceptBinaryOperator(binaryOp, iBinaryOpPrecedence, uiOperatorLength) && iBinaryOpPrecedence < iPrecedence)
  {
    // Consume token.
    m_uiCurrentToken += uiOperatorLength;

    auto pSecondOperand = ParseExpression(iBinaryOpPrecedence);
    if (pSecondOperand == nullptr)
      return nullptr;

    if (binaryOp == plExpressionAST::NodeType::Select)
    {
      if (Expect(":").Failed())
        return nullptr;

      auto pThirdOperand = ParseExpression(iBinaryOpPrecedence);
      if (pThirdOperand == nullptr)
        return nullptr;

      pExpression = m_pAST->CreateTernaryOperator(plExpressionAST::NodeType::Select, pExpression, pSecondOperand, pThirdOperand);
    }
    else
    {
      pExpression = m_pAST->CreateBinaryOperator(binaryOp, pExpression, pSecondOperand);
    }
  }

  return pExpression;
}

plExpressionAST::Node* plExpressionParser::ParseUnaryExpression()
{
  while (Accept(m_TokenStream, m_uiCurrentToken, "+"))
  {
  }

  if (Accept(m_TokenStream, m_uiCurrentToken, "-"))
  {
    auto pOperand = ParseUnaryExpression();
    if (pOperand == nullptr)
      return nullptr;

    return m_pAST->CreateUnaryOperator(plExpressionAST::NodeType::Negate, pOperand);
  }
  else if (Accept(m_TokenStream, m_uiCurrentToken, "~"))
  {
    auto pOperand = ParseUnaryExpression();
    if (pOperand == nullptr)
      return nullptr;

    return m_pAST->CreateUnaryOperator(plExpressionAST::NodeType::BitwiseNot, pOperand);
  }
  else if (Accept(m_TokenStream, m_uiCurrentToken, "!"))
  {
    auto pOperand = ParseUnaryExpression();
    if (pOperand == nullptr)
      return nullptr;

    return m_pAST->CreateUnaryOperator(plExpressionAST::NodeType::LogicalNot, pOperand);
  }

  return ParseFactor();
}

plExpressionAST::Node* plExpressionParser::ParseFunctionCall(plStringView sFunctionName)
{
  // "(" of the function call
  const plToken* pFunctionToken = m_TokenStream[m_uiCurrentToken - 1];

  plSmallArray<plExpressionAST::Node*, 8> arguments;
  if (Accept(m_TokenStream, m_uiCurrentToken, ")") == false)
  {
    do
    {
      arguments.PushBack(ParseExpression());
    } while (Accept(m_TokenStream, m_uiCurrentToken, ","));

    if (Expect(")").Failed())
      return nullptr;
  }

  auto CheckArgumentCount = [&](plUInt32 uiExpectedArgumentCount) -> plResult {
    if (arguments.GetCount() != uiExpectedArgumentCount)
    {
      ReportError(pFunctionToken, plFmt("Invalid argument count for '{}'. Expected {} but got {}", sFunctionName, uiExpectedArgumentCount, arguments.GetCount()));
      return PLASMA_FAILURE;
    }
    return PLASMA_SUCCESS;
  };

  plHashedString sHashedFuncName;
  sHashedFuncName.Assign(sFunctionName);

  plEnum<plExpressionAST::DataType> dataType;
  if (m_KnownTypes.TryGetValue(sHashedFuncName, dataType))
  {
    plUInt32 uiElementCount = plExpressionAST::DataType::GetElementCount(dataType);
    if (arguments.GetCount() > uiElementCount)
    {
      ReportError(pFunctionToken, plFmt("Invalid argument count for '{}'. Expected 0 - {} but got {}", sFunctionName, uiElementCount, arguments.GetCount()));
      return nullptr;
    }

    return m_pAST->CreateConstructorCall(dataType, arguments);
  }

  plEnum<plExpressionAST::NodeType> builtinType;
  if (m_BuiltinFunctions.TryGetValue(sHashedFuncName, builtinType))
  {
    if (plExpressionAST::NodeType::IsUnary(builtinType))
    {
      if (CheckArgumentCount(1).Failed())
        return nullptr;

      return m_pAST->CreateUnaryOperator(builtinType, arguments[0]);
    }
    else if (plExpressionAST::NodeType::IsBinary(builtinType))
    {
      if (CheckArgumentCount(2).Failed())
        return nullptr;

      return m_pAST->CreateBinaryOperator(builtinType, arguments[0], arguments[1]);
    }
    else if (plExpressionAST::NodeType::IsTernary(builtinType))
    {
      if (CheckArgumentCount(3).Failed())
        return nullptr;

      return m_pAST->CreateTernaryOperator(builtinType, arguments[0], arguments[1], arguments[2]);
    }

    PLASMA_ASSERT_NOT_IMPLEMENTED;
    return nullptr;
  }

  // external function
  const plHybridArray<plExpression::FunctionDesc, 1>* pFunctionDescs = nullptr;
  if (m_FunctionDescs.TryGetValue(sHashedFuncName, pFunctionDescs))
  {
    plUInt32 uiMinArgumentCount = plInvalidIndex;
    for (auto& funcDesc : *pFunctionDescs)
    {
      uiMinArgumentCount = plMath::Min<plUInt32>(uiMinArgumentCount, funcDesc.m_uiNumRequiredInputs);
    }

    if (arguments.GetCount() < uiMinArgumentCount)
    {
      ReportError(pFunctionToken, plFmt("Invalid argument count for '{}'. Expected at least {} but got {}", sFunctionName, uiMinArgumentCount, arguments.GetCount()));
      return nullptr;
    }

    return m_pAST->CreateFunctionCall(*pFunctionDescs, arguments);
  }

  ReportError(pFunctionToken, plFmt("Undeclared function '{}'", sFunctionName));
  return nullptr;
}

plExpressionAST::Node* plExpressionParser::ParseSwizzle(plExpressionAST::Node* pExpression)
{
  if (Accept(m_TokenStream, m_uiCurrentToken, "."))
  {
    const plToken* pSwizzleToken = nullptr;
    if (Expect(plTokenType::Identifier, &pSwizzleToken).Failed())
      return nullptr;

    pExpression = m_pAST->CreateSwizzle(pSwizzleToken->m_DataView, pExpression);
    if (pExpression == nullptr)
    {
      ReportError(pSwizzleToken, plFmt("Invalid swizzle '{}'", pSwizzleToken->m_DataView));
    }
  }

  return pExpression;
}

// Does NOT advance the current token beyond the operator!
bool plExpressionParser::AcceptOperator(plStringView sName)
{
  const plUInt32 uiOperatorLength = sName.GetElementCount();

  if (m_uiCurrentToken + uiOperatorLength - 1 >= m_TokenStream.GetCount())
    return false;

  for (plUInt32 charIndex = 0; charIndex < uiOperatorLength; ++charIndex)
  {
    if (m_TokenStream[m_uiCurrentToken + charIndex]->m_DataView.GetCharacter() != sName.GetStartPointer()[charIndex])
    {
      return false;
    }
  }

  return true;
}

// Does NOT advance the current token beyond the binary operator!
bool plExpressionParser::AcceptBinaryOperator(plExpressionAST::NodeType::Enum& out_binaryOp, int& out_iOperatorPrecedence, plUInt32& out_uiOperatorLength)
{
  SkipWhitespace(m_TokenStream, m_uiCurrentToken);

  for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(s_binaryOperators); ++i)
  {
    auto& op = s_binaryOperators[i];
    if (AcceptOperator(op.m_sName))
    {
      out_binaryOp = op.m_NodeType;
      out_iOperatorPrecedence = op.m_iPrecedence;
      out_uiOperatorLength = op.m_sName.GetElementCount();
      return true;
    }
  }

  return false;
}

plExpressionAST::Node* plExpressionParser::GetVariable(plStringView sVarName)
{
  plHashedString sHashedVarName;
  sHashedVarName.Assign(sVarName);

  plExpressionAST::Node* pVariableNode = nullptr;
  if (m_KnownVariables.TryGetValue(sHashedVarName, pVariableNode) == false && m_Options.m_bTreatUnknownVariablesAsInputs)
  {
    pVariableNode = m_pAST->CreateInput({sHashedVarName, plProcessingStream::DataType::Float});
    m_KnownVariables.Insert(sHashedVarName, pVariableNode);
  }

  return pVariableNode;
}

plExpressionAST::Node* plExpressionParser::EnsureExpectedType(plExpressionAST::Node* pNode, plExpressionAST::DataType::Enum expectedType)
{
  if (expectedType != plExpressionAST::DataType::Unknown)
  {
    const auto nodeRegisterType = plExpressionAST::DataType::GetRegisterType(pNode->m_ReturnType);
    const auto expectedRegisterType = plExpressionAST::DataType::GetRegisterType(expectedType);
    if (nodeRegisterType != expectedRegisterType)
    {
      pNode = m_pAST->CreateUnaryOperator(plExpressionAST::NodeType::TypeConversion, pNode, expectedType);
    }

    const plUInt32 nodeElementCount = plExpressionAST::DataType::GetElementCount(pNode->m_ReturnType);
    const plUInt32 expectedElementCount = plExpressionAST::DataType::GetElementCount(expectedType);
    if (nodeElementCount < expectedElementCount)
    {
      pNode = m_pAST->CreateConstructorCall(expectedType, plMakeArrayPtr(&pNode, 1));
    }
  }

  return pNode;
}

plExpressionAST::Node* plExpressionParser::Unpack(plExpressionAST::Node* pNode, bool bUnassignedError /*= true*/)
{
  if (plExpressionAST::NodeType::IsOutput(pNode->m_Type))
  {
    auto pOutput = static_cast<plExpressionAST::Output*>(pNode);
    if (pOutput->m_pExpression == nullptr && bUnassignedError)
    {
      ReportError(m_TokenStream[m_uiCurrentToken], plFmt("Output '{}' has not been assigned yet", pOutput->m_Desc.m_sName));
    }

    return pOutput->m_pExpression;
  }

  return pNode;
}

plResult plExpressionParser::CheckOutputs()
{
  for (auto pOutputNode : m_pAST->m_OutputNodes)
  {
    if (pOutputNode->m_pExpression == nullptr)
    {
      plLog::Error("Output '{}' was never written", pOutputNode->m_Desc.m_sName);
      return PLASMA_FAILURE;
    }
  }

  return PLASMA_SUCCESS;
}


PLASMA_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Expression_Implementation_ExpressionParser);
