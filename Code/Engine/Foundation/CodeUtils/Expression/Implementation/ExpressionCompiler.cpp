#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/DGMLWriter.h>

namespace
{
#define ADD_OFFSET(opCode) static_cast<plExpressionByteCode::OpCode::Enum>((opCode) + uiOffset)

  static plExpressionByteCode::OpCode::Enum NodeTypeToOpCode(plExpressionAST::NodeType::Enum nodeType, plExpressionAST::DataType::Enum dataType, bool bRightIsConstant)
  {
    const plExpression::RegisterType::Enum registerType = plExpressionAST::DataType::GetRegisterType(dataType);
    const bool bFloat = registerType == plExpression::RegisterType::Float;
    const bool bInt = registerType == plExpression::RegisterType::Int;
    const plUInt32 uiOffset = bRightIsConstant ? plExpressionByteCode::OpCode::FirstBinaryWithConstant - plExpressionByteCode::OpCode::FirstBinary : 0;

    switch (nodeType)
    {
      case plExpressionAST::NodeType::Absolute:
        return bFloat ? plExpressionByteCode::OpCode::AbsF_R : plExpressionByteCode::OpCode::AbsI_R;
      case plExpressionAST::NodeType::Sqrt:
        return plExpressionByteCode::OpCode::SqrtF_R;

      case plExpressionAST::NodeType::Exp:
        return plExpressionByteCode::OpCode::ExpF_R;
      case plExpressionAST::NodeType::Ln:
        return plExpressionByteCode::OpCode::LnF_R;
      case plExpressionAST::NodeType::Log2:
        return bFloat ? plExpressionByteCode::OpCode::Log2F_R : plExpressionByteCode::OpCode::Log2I_R;
      case plExpressionAST::NodeType::Log10:
        return plExpressionByteCode::OpCode::Log10F_R;
      case plExpressionAST::NodeType::Pow2:
        return plExpressionByteCode::OpCode::Pow2F_R;

      case plExpressionAST::NodeType::Sin:
        return plExpressionByteCode::OpCode::SinF_R;
      case plExpressionAST::NodeType::Cos:
        return plExpressionByteCode::OpCode::CosF_R;
      case plExpressionAST::NodeType::Tan:
        return plExpressionByteCode::OpCode::TanF_R;

      case plExpressionAST::NodeType::ASin:
        return plExpressionByteCode::OpCode::ASinF_R;
      case plExpressionAST::NodeType::ACos:
        return plExpressionByteCode::OpCode::ACosF_R;
      case plExpressionAST::NodeType::ATan:
        return plExpressionByteCode::OpCode::ATanF_R;

      case plExpressionAST::NodeType::Round:
        return plExpressionByteCode::OpCode::RoundF_R;
      case plExpressionAST::NodeType::Floor:
        return plExpressionByteCode::OpCode::FloorF_R;
      case plExpressionAST::NodeType::Ceil:
        return plExpressionByteCode::OpCode::CeilF_R;
      case plExpressionAST::NodeType::Trunc:
        return plExpressionByteCode::OpCode::TruncF_R;

      case plExpressionAST::NodeType::BitwiseNot:
        return plExpressionByteCode::OpCode::NotI_R;
      case plExpressionAST::NodeType::LogicalNot:
        return plExpressionByteCode::OpCode::NotB_R;

      case plExpressionAST::NodeType::TypeConversion:
        return bFloat ? plExpressionByteCode::OpCode::IToF_R : plExpressionByteCode::OpCode::FToI_R;

      case plExpressionAST::NodeType::Add:
        return ADD_OFFSET(bFloat ? plExpressionByteCode::OpCode::AddF_RR : plExpressionByteCode::OpCode::AddI_RR);
      case plExpressionAST::NodeType::Subtract:
        return ADD_OFFSET(bFloat ? plExpressionByteCode::OpCode::SubF_RR : plExpressionByteCode::OpCode::SubI_RR);
      case plExpressionAST::NodeType::Multiply:
        return ADD_OFFSET(bFloat ? plExpressionByteCode::OpCode::MulF_RR : plExpressionByteCode::OpCode::MulI_RR);
      case plExpressionAST::NodeType::Divide:
        return ADD_OFFSET(bFloat ? plExpressionByteCode::OpCode::DivF_RR : plExpressionByteCode::OpCode::DivI_RR);
      case plExpressionAST::NodeType::Min:
        return ADD_OFFSET(bFloat ? plExpressionByteCode::OpCode::MinF_RR : plExpressionByteCode::OpCode::MinI_RR);
      case plExpressionAST::NodeType::Max:
        return ADD_OFFSET(bFloat ? plExpressionByteCode::OpCode::MaxF_RR : plExpressionByteCode::OpCode::MaxI_RR);

      case plExpressionAST::NodeType::BitshiftLeft:
        return ADD_OFFSET(plExpressionByteCode::OpCode::ShlI_RR);
      case plExpressionAST::NodeType::BitshiftRight:
        return ADD_OFFSET(plExpressionByteCode::OpCode::ShrI_RR);
      case plExpressionAST::NodeType::BitwiseAnd:
        return ADD_OFFSET(plExpressionByteCode::OpCode::AndI_RR);
      case plExpressionAST::NodeType::BitwiseXor:
        return ADD_OFFSET(plExpressionByteCode::OpCode::XorI_RR);
      case plExpressionAST::NodeType::BitwiseOr:
        return ADD_OFFSET(plExpressionByteCode::OpCode::OrI_RR);

      case plExpressionAST::NodeType::Equal:
        if (bFloat)
          return ADD_OFFSET(plExpressionByteCode::OpCode::EqF_RR);
        else if (bInt)
          return ADD_OFFSET(plExpressionByteCode::OpCode::EqI_RR);
        else
          return ADD_OFFSET(plExpressionByteCode::OpCode::EqB_RR);
      case plExpressionAST::NodeType::NotEqual:
        if (bFloat)
          return ADD_OFFSET(plExpressionByteCode::OpCode::NEqF_RR);
        else if (bInt)
          return ADD_OFFSET(plExpressionByteCode::OpCode::NEqI_RR);
        else
          return ADD_OFFSET(plExpressionByteCode::OpCode::NEqB_RR);
      case plExpressionAST::NodeType::Less:
        return ADD_OFFSET(bFloat ? plExpressionByteCode::OpCode::LtF_RR : plExpressionByteCode::OpCode::LtI_RR);
      case plExpressionAST::NodeType::LessEqual:
        return ADD_OFFSET(bFloat ? plExpressionByteCode::OpCode::LEqF_RR : plExpressionByteCode::OpCode::LEqI_RR);
      case plExpressionAST::NodeType::Greater:
        return ADD_OFFSET(bFloat ? plExpressionByteCode::OpCode::GtF_RR : plExpressionByteCode::OpCode::GtI_RR);
      case plExpressionAST::NodeType::GreaterEqual:
        return ADD_OFFSET(bFloat ? plExpressionByteCode::OpCode::GEqF_RR : plExpressionByteCode::OpCode::GEqI_RR);

      case plExpressionAST::NodeType::LogicalAnd:
        return ADD_OFFSET(plExpressionByteCode::OpCode::AndB_RR);
      case plExpressionAST::NodeType::LogicalOr:
        return ADD_OFFSET(plExpressionByteCode::OpCode::OrB_RR);

      case plExpressionAST::NodeType::Select:
        if (bFloat)
          return plExpressionByteCode::OpCode::SelF_RRR;
        else if (bInt)
          return plExpressionByteCode::OpCode::SelI_RRR;
        else
          return plExpressionByteCode::OpCode::SelB_RRR;

      case plExpressionAST::NodeType::Constant:
        return plExpressionByteCode::OpCode::MovX_C;
      case plExpressionAST::NodeType::Input:
        return bFloat ? plExpressionByteCode::OpCode::LoadF : plExpressionByteCode::OpCode::LoadI;
      case plExpressionAST::NodeType::Output:
        return bFloat ? plExpressionByteCode::OpCode::StoreF : plExpressionByteCode::OpCode::StoreI;
      case plExpressionAST::NodeType::FunctionCall:
        return plExpressionByteCode::OpCode::Call;
      case plExpressionAST::NodeType::ConstructorCall:
        PLASMA_REPORT_FAILURE("Constructor calls should not exist anymore after AST transformations");
        return plExpressionByteCode::OpCode::Nop;

      default:
        PLASMA_ASSERT_NOT_IMPLEMENTED;
        return plExpressionByteCode::OpCode::Nop;
    }
  }

#undef ADD_OFFSET
} // namespace

plExpressionCompiler::plExpressionCompiler() = default;
plExpressionCompiler::~plExpressionCompiler() = default;

plResult plExpressionCompiler::Compile(plExpressionAST& ref_ast, plExpressionByteCode& out_byteCode, plStringView sDebugAstOutputPath /*= plStringView()*/)
{
  out_byteCode.Clear();

  PLASMA_SUCCEED_OR_RETURN(TransformAndOptimizeAST(ref_ast, sDebugAstOutputPath));
  PLASMA_SUCCEED_OR_RETURN(BuildNodeInstructions(ref_ast));
  PLASMA_SUCCEED_OR_RETURN(UpdateRegisterLifetime(ref_ast));
  PLASMA_SUCCEED_OR_RETURN(AssignRegisters());
  PLASMA_SUCCEED_OR_RETURN(GenerateByteCode(ref_ast, out_byteCode));

  return PLASMA_SUCCESS;
}

plResult plExpressionCompiler::TransformAndOptimizeAST(plExpressionAST& ast, plStringView sDebugAstOutputPath)
{
  DumpAST(ast, sDebugAstOutputPath, "_00");

  PLASMA_SUCCEED_OR_RETURN(TransformASTPostOrder(ast, plMakeDelegate(&plExpressionAST::TypeDeductionAndConversion, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_01_TypeConv");

  PLASMA_SUCCEED_OR_RETURN(TransformASTPreOrder(ast, plMakeDelegate(&plExpressionAST::ReplaceVectorInstructions, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_02_ReplacedVectorInst");

  PLASMA_SUCCEED_OR_RETURN(ast.ScalarizeOutputs());
  PLASMA_SUCCEED_OR_RETURN(TransformASTPreOrder(ast, plMakeDelegate(&plExpressionAST::ScalarizeVectorInstructions, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_03_Scalarized");

  PLASMA_SUCCEED_OR_RETURN(TransformASTPostOrder(ast, plMakeDelegate(&plExpressionAST::FoldConstants, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_04_ConstantFolded1");

  PLASMA_SUCCEED_OR_RETURN(TransformASTPreOrder(ast, plMakeDelegate(&plExpressionAST::ReplaceUnsupportedInstructions, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_05_ReplacedUnsupportedInst");

  PLASMA_SUCCEED_OR_RETURN(TransformASTPostOrder(ast, plMakeDelegate(&plExpressionAST::FoldConstants, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_06_ConstantFolded2");

  PLASMA_SUCCEED_OR_RETURN(TransformASTPostOrder(ast, plMakeDelegate(&plExpressionAST::CommonSubexpressionElimination, &ast)));
  PLASMA_SUCCEED_OR_RETURN(TransformASTPreOrder(ast, plMakeDelegate(&plExpressionAST::Validate, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_07_Optimized");

  return PLASMA_SUCCESS;
}

plResult plExpressionCompiler::BuildNodeInstructions(const plExpressionAST& ast)
{
  m_NodeStack.Clear();
  m_NodeInstructions.Clear();
  auto& nodeStackTemp = m_NodeInstructions;

  // Build node instruction order aka post order tree traversal
  for (plExpressionAST::Node* pOutputNode : ast.m_OutputNodes)
  {
    if (pOutputNode == nullptr)
      return PLASMA_FAILURE;

    PLASMA_ASSERT_DEV(nodeStackTemp.IsEmpty(), "Implementation error");

    nodeStackTemp.PushBack(pOutputNode);

    while (!nodeStackTemp.IsEmpty())
    {
      auto pCurrentNode = nodeStackTemp.PeekBack();
      nodeStackTemp.PopBack();

      if (pCurrentNode == nullptr)
      {
        return PLASMA_FAILURE;
      }

      m_NodeStack.PushBack(pCurrentNode);

      if (plExpressionAST::NodeType::IsBinary(pCurrentNode->m_Type))
      {
        auto pBinary = static_cast<const plExpressionAST::BinaryOperator*>(pCurrentNode);
        nodeStackTemp.PushBack(pBinary->m_pLeftOperand);

        // Do not push the right operand if it is a constant, we don't want a separate mov instruction for it
        // since all binary operators can take a constant as right operand in place.
        const bool bRightIsConstant = plExpressionAST::NodeType::IsConstant(pBinary->m_pRightOperand->m_Type);
        if (!bRightIsConstant)
        {
          nodeStackTemp.PushBack(pBinary->m_pRightOperand);
        }
      }
      else
      {
        auto children = plExpressionAST::GetChildren(pCurrentNode);
        for (auto pChild : children)
        {
          nodeStackTemp.PushBack(pChild);
        }
      }
    }
  }

  if (m_NodeStack.IsEmpty())
  {
    // Nothing to compile
    return PLASMA_FAILURE;
  }

  PLASMA_ASSERT_DEV(m_NodeInstructions.IsEmpty(), "Implementation error");

  m_NodeToRegisterIndex.Clear();
  m_LiveIntervals.Clear();
  plUInt32 uiNextRegisterIndex = 0;

  // De-duplicate nodes, build final instruction list and assign virtual register indices. Also determine their lifetime start.
  while (!m_NodeStack.IsEmpty())
  {
    auto pCurrentNode = m_NodeStack.PeekBack();
    m_NodeStack.PopBack();

    if (!m_NodeToRegisterIndex.Contains(pCurrentNode))
    {
      m_NodeInstructions.PushBack(pCurrentNode);

      if (plExpressionAST::NodeType::IsOutput(pCurrentNode->m_Type))
        continue;

      m_NodeToRegisterIndex.Insert(pCurrentNode, uiNextRegisterIndex);
      ++uiNextRegisterIndex;

      plUInt32 uiCurrentInstructionIndex = m_NodeInstructions.GetCount() - 1;
      m_LiveIntervals.PushBack({uiCurrentInstructionIndex, uiCurrentInstructionIndex, pCurrentNode});
      PLASMA_ASSERT_DEV(m_LiveIntervals.GetCount() == uiNextRegisterIndex, "Implementation error");
    }
  }

  return PLASMA_SUCCESS;
}

plResult plExpressionCompiler::UpdateRegisterLifetime(const plExpressionAST& ast)
{
  plUInt32 uiNumInstructions = m_NodeInstructions.GetCount();
  for (plUInt32 uiInstructionIndex = 0; uiInstructionIndex < uiNumInstructions; ++uiInstructionIndex)
  {
    auto pCurrentNode = m_NodeInstructions[uiInstructionIndex];

    auto children = plExpressionAST::GetChildren(pCurrentNode);
    for (auto pChild : children)
    {
      plUInt32 uiRegisterIndex = plInvalidIndex;
      if (m_NodeToRegisterIndex.TryGetValue(pChild, uiRegisterIndex))
      {
        auto& liveRegister = m_LiveIntervals[uiRegisterIndex];

        liveRegister.m_uiStart = plMath::Min(liveRegister.m_uiStart, uiInstructionIndex);
        liveRegister.m_uiEnd = plMath::Max(liveRegister.m_uiEnd, uiInstructionIndex);
      }
      else
      {
        PLASMA_ASSERT_DEV(plExpressionAST::NodeType::IsConstant(pChild->m_Type), "Must have a valid register for nodes that are not constants");
      }
    }
  }

  return PLASMA_SUCCESS;
}

plResult plExpressionCompiler::AssignRegisters()
{
  // This is an implementation of the linear scan register allocation algorithm without spilling
  // https://www2.seas.gwu.edu/~hchoi/teaching/cs160d/linearscan.pdf

  // Sort register lifetime by start index
  m_LiveIntervals.Sort([](const LiveInterval& a, const LiveInterval& b) { return a.m_uiStart < b.m_uiStart; });

  // Assign registers
  plHybridArray<LiveInterval, 64> activeIntervals;
  plHybridArray<plUInt32, 64> freeRegisters;

  for (auto& liveInterval : m_LiveIntervals)
  {
    // Expire old intervals
    for (plUInt32 uiActiveIndex = activeIntervals.GetCount(); uiActiveIndex-- > 0;)
    {
      auto& activeInterval = activeIntervals[uiActiveIndex];
      if (activeInterval.m_uiEnd <= liveInterval.m_uiStart)
      {
        plUInt32 uiRegisterIndex = m_NodeToRegisterIndex[activeInterval.m_pNode];
        freeRegisters.PushBack(uiRegisterIndex);

        activeIntervals.RemoveAtAndCopy(uiActiveIndex);
      }
    }

    // Allocate register
    plUInt32 uiNewRegister = 0;
    if (!freeRegisters.IsEmpty())
    {
      uiNewRegister = freeRegisters.PeekBack();
      freeRegisters.PopBack();
    }
    else
    {
      uiNewRegister = activeIntervals.GetCount();
    }
    m_NodeToRegisterIndex[liveInterval.m_pNode] = uiNewRegister;

    activeIntervals.PushBack(liveInterval);
  }

  return PLASMA_SUCCESS;
}

plResult plExpressionCompiler::GenerateByteCode(const plExpressionAST& ast, plExpressionByteCode& out_byteCode)
{
  auto& byteCode = out_byteCode.m_ByteCode;

  plUInt32 uiMaxRegisterIndex = 0;

  m_InputToIndex.Clear();
  m_OutputToIndex.Clear();
  m_FunctionToIndex.Clear();

  for (auto pCurrentNode : m_NodeInstructions)
  {
    const plExpressionAST::NodeType::Enum nodeType = pCurrentNode->m_Type;
    plExpressionAST::DataType::Enum dataType = pCurrentNode->m_ReturnType;
    if (dataType == plExpressionAST::DataType::Unknown)
    {
      return PLASMA_FAILURE;
    }

    bool bRightIsConstant = false;
    if (plExpressionAST::NodeType::IsBinary(nodeType))
    {
      auto pBinary = static_cast<const plExpressionAST::BinaryOperator*>(pCurrentNode);
      dataType = pBinary->m_pLeftOperand->m_ReturnType;
      bRightIsConstant = plExpressionAST::NodeType::IsConstant(pBinary->m_pRightOperand->m_Type);
    }

    const auto opCode = NodeTypeToOpCode(nodeType, dataType, bRightIsConstant);
    if (opCode == plExpressionByteCode::OpCode::Nop)
      return PLASMA_FAILURE;

    plUInt32 uiTargetRegister = m_NodeToRegisterIndex[pCurrentNode];
    if (plExpressionAST::NodeType::IsOutput(nodeType) == false)
    {
      uiMaxRegisterIndex = plMath::Max(uiMaxRegisterIndex, uiTargetRegister);
    }

    if (plExpressionAST::NodeType::IsUnary(nodeType))
    {
      auto pUnary = static_cast<const plExpressionAST::UnaryOperator*>(pCurrentNode);

      byteCode.PushBack(opCode);
      byteCode.PushBack(uiTargetRegister);
      byteCode.PushBack(m_NodeToRegisterIndex[pUnary->m_pOperand]);
    }
    else if (plExpressionAST::NodeType::IsBinary(nodeType))
    {
      auto pBinary = static_cast<const plExpressionAST::BinaryOperator*>(pCurrentNode);

      byteCode.PushBack(opCode);
      byteCode.PushBack(uiTargetRegister);
      byteCode.PushBack(m_NodeToRegisterIndex[pBinary->m_pLeftOperand]);

      if (bRightIsConstant)
      {
        PLASMA_SUCCEED_OR_RETURN(GenerateConstantByteCode(static_cast<const plExpressionAST::Constant*>(pBinary->m_pRightOperand), out_byteCode));
      }
      else
      {
        byteCode.PushBack(m_NodeToRegisterIndex[pBinary->m_pRightOperand]);
      }
    }
    else if (plExpressionAST::NodeType::IsTernary(nodeType))
    {
      auto pTernary = static_cast<const plExpressionAST::TernaryOperator*>(pCurrentNode);

      byteCode.PushBack(opCode);
      byteCode.PushBack(uiTargetRegister);
      byteCode.PushBack(m_NodeToRegisterIndex[pTernary->m_pFirstOperand]);
      byteCode.PushBack(m_NodeToRegisterIndex[pTernary->m_pSecondOperand]);
      byteCode.PushBack(m_NodeToRegisterIndex[pTernary->m_pThirdOperand]);
    }
    else if (plExpressionAST::NodeType::IsConstant(nodeType))
    {
      byteCode.PushBack(opCode);
      byteCode.PushBack(uiTargetRegister);
      PLASMA_SUCCEED_OR_RETURN(GenerateConstantByteCode(static_cast<const plExpressionAST::Constant*>(pCurrentNode), out_byteCode));
    }
    else if (plExpressionAST::NodeType::IsInput(nodeType))
    {
      auto& desc = static_cast<const plExpressionAST::Input*>(pCurrentNode)->m_Desc;
      plUInt32 uiInputIndex = 0;
      if (!m_InputToIndex.TryGetValue(desc.m_sName, uiInputIndex))
      {
        uiInputIndex = out_byteCode.m_Inputs.GetCount();
        m_InputToIndex.Insert(desc.m_sName, uiInputIndex);

        out_byteCode.m_Inputs.PushBack(desc);
      }

      byteCode.PushBack(opCode);
      byteCode.PushBack(uiTargetRegister);
      byteCode.PushBack(uiInputIndex);
    }
    else if (plExpressionAST::NodeType::IsOutput(nodeType))
    {
      auto pOutput = static_cast<const plExpressionAST::Output*>(pCurrentNode);
      auto& desc = pOutput->m_Desc;
      plUInt32 uiOutputIndex = 0;
      if (!m_OutputToIndex.TryGetValue(desc.m_sName, uiOutputIndex))
      {
        uiOutputIndex = out_byteCode.m_Outputs.GetCount();
        m_OutputToIndex.Insert(desc.m_sName, uiOutputIndex);

        out_byteCode.m_Outputs.PushBack(desc);
      }

      byteCode.PushBack(opCode);
      byteCode.PushBack(uiOutputIndex);
      byteCode.PushBack(m_NodeToRegisterIndex[pOutput->m_pExpression]);
    }
    else if (plExpressionAST::NodeType::IsFunctionCall(nodeType))
    {
      auto pFunctionCall = static_cast<const plExpressionAST::FunctionCall*>(pCurrentNode);
      auto pDesc = pFunctionCall->m_Descs[pCurrentNode->m_uiOverloadIndex];
      plHashedString sMangledName = pDesc->GetMangledName();

      plUInt32 uiFunctionIndex = 0;
      if (!m_FunctionToIndex.TryGetValue(sMangledName, uiFunctionIndex))
      {
        uiFunctionIndex = out_byteCode.m_Functions.GetCount();
        m_FunctionToIndex.Insert(sMangledName, uiFunctionIndex);

        out_byteCode.m_Functions.PushBack(*pDesc);
        out_byteCode.m_Functions.PeekBack().m_sName = std::move(sMangledName);
      }

      byteCode.PushBack(opCode);
      byteCode.PushBack(uiFunctionIndex);
      byteCode.PushBack(uiTargetRegister);

      byteCode.PushBack(pFunctionCall->m_Arguments.GetCount());
      for (auto pArg : pFunctionCall->m_Arguments)
      {
        plUInt32 uiArgRegister = m_NodeToRegisterIndex[pArg];
        byteCode.PushBack(uiArgRegister);
      }
    }
    else
    {
      PLASMA_ASSERT_NOT_IMPLEMENTED;
    }
  }

  out_byteCode.m_uiNumInstructions = m_NodeInstructions.GetCount();
  out_byteCode.m_uiNumTempRegisters = uiMaxRegisterIndex + 1;

  return PLASMA_SUCCESS;
}

plResult plExpressionCompiler::GenerateConstantByteCode(const plExpressionAST::Constant* pConstant, plExpressionByteCode& out_byteCode)
{
  auto& byteCode = out_byteCode.m_ByteCode;

  if (pConstant->m_ReturnType == plExpressionAST::DataType::Float)
  {
    byteCode.PushBack(*reinterpret_cast<const plUInt32*>(&pConstant->m_Value.Get<float>()));
    return PLASMA_SUCCESS;
  }
  else if (pConstant->m_ReturnType == plExpressionAST::DataType::Int)
  {
    byteCode.PushBack(pConstant->m_Value.Get<int>());
    return PLASMA_SUCCESS;
  }
  else if (pConstant->m_ReturnType == plExpressionAST::DataType::Bool)
  {
    byteCode.PushBack(pConstant->m_Value.Get<bool>() ? 0xFFFFFFFF : 0);
    return PLASMA_SUCCESS;
  }

  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return PLASMA_FAILURE;
}

plResult plExpressionCompiler::TransformASTPreOrder(plExpressionAST& ast, TransformFunc func)
{
  m_NodeStack.Clear();
  m_TransformCache.Clear();

  for (plExpressionAST::Output*& pOutputNode : ast.m_OutputNodes)
  {
    if (pOutputNode == nullptr)
      return PLASMA_FAILURE;

    PLASMA_SUCCEED_OR_RETURN(TransformOutputNode(pOutputNode, func));

    m_NodeStack.PushBack(pOutputNode);

    while (!m_NodeStack.IsEmpty())
    {
      auto pParent = m_NodeStack.PeekBack();
      m_NodeStack.PopBack();

      auto children = plExpressionAST::GetChildren(pParent);
      for (auto& pChild : children)
      {
        PLASMA_SUCCEED_OR_RETURN(TransformNode(pChild, func));

        m_NodeStack.PushBack(pChild);
      }
    }
  }

  return PLASMA_SUCCESS;
}

plResult plExpressionCompiler::TransformASTPostOrder(plExpressionAST& ast, TransformFunc func)
{
  m_NodeStack.Clear();
  m_NodeInstructions.Clear();
  auto& nodeStackTemp = m_NodeInstructions;

  for (plExpressionAST::Node* pOutputNode : ast.m_OutputNodes)
  {
    if (pOutputNode == nullptr)
      return PLASMA_FAILURE;

    nodeStackTemp.PushBack(pOutputNode);

    while (!nodeStackTemp.IsEmpty())
    {
      auto pParent = nodeStackTemp.PeekBack();
      nodeStackTemp.PopBack();

      m_NodeStack.PushBack(pParent);

      auto children = plExpressionAST::GetChildren(pParent);
      for (auto pChild : children)
      {
        if (pChild != nullptr)
        {
          nodeStackTemp.PushBack(pChild);
        }
      }
    }
  }

  m_TransformCache.Clear();

  while (!m_NodeStack.IsEmpty())
  {
    auto pParent = m_NodeStack.PeekBack();
    m_NodeStack.PopBack();

    auto children = plExpressionAST::GetChildren(pParent);
    for (auto& pChild : children)
    {
      PLASMA_SUCCEED_OR_RETURN(TransformNode(pChild, func));
    }
  }

  for (plExpressionAST::Output*& pOutputNode : ast.m_OutputNodes)
  {
    PLASMA_SUCCEED_OR_RETURN(TransformOutputNode(pOutputNode, func));
  }

  return PLASMA_SUCCESS;
}

plResult plExpressionCompiler::TransformNode(plExpressionAST::Node*& pNode, TransformFunc& func)
{
  if (pNode == nullptr)
    return PLASMA_SUCCESS;

  plExpressionAST::Node* pNewNode = nullptr;
  if (m_TransformCache.TryGetValue(pNode, pNewNode) == false)
  {
    pNewNode = func(pNode);
    if (pNewNode == nullptr)
    {
      return PLASMA_FAILURE;
    }

    m_TransformCache.Insert(pNode, pNewNode);
  }

  pNode = pNewNode;

  return PLASMA_SUCCESS;
}

plResult plExpressionCompiler::TransformOutputNode(plExpressionAST::Output*& pOutputNode, TransformFunc& func)
{
  if (pOutputNode == nullptr)
    return PLASMA_SUCCESS;

  auto pNewOutput = func(pOutputNode);
  if (pNewOutput != pOutputNode)
  {
    if (pNewOutput != nullptr && plExpressionAST::NodeType::IsOutput(pNewOutput->m_Type))
    {
      pOutputNode = static_cast<plExpressionAST::Output*>(pNewOutput);
    }
    else
    {
      plLog::Error("Transformed output node for '{}' is invalid", pOutputNode->m_Desc.m_sName);
      return PLASMA_FAILURE;
    }
  }

  return PLASMA_SUCCESS;
}

void plExpressionCompiler::DumpAST(const plExpressionAST& ast, plStringView sOutputPath, plStringView sSuffix)
{
  if (sOutputPath.IsEmpty())
    return;

  plDGMLGraph dgmlGraph;
  ast.PrintGraph(dgmlGraph);

  plStringView sExt = sOutputPath.GetFileExtension();
  plStringBuilder sFullPath;
  sFullPath.Append(sOutputPath.GetFileDirectory(), sOutputPath.GetFileName(), sSuffix, ".", sExt);

  plDGMLGraphWriter dgmlGraphWriter;
  if (dgmlGraphWriter.WriteGraphToFile(sFullPath, dgmlGraph).Succeeded())
  {
    plLog::Info("AST was dumped to: {}", sFullPath);
  }
  else
  {
    plLog::Error("Failed to dump AST to: {}", sFullPath);
  }
}


PLASMA_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Expression_Implementation_ExpressionCompiler);
