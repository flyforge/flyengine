#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Utilities/DGMLWriter.h>

// static
bool plExpressionAST::NodeType::IsUnary(Enum nodeType)
{
  return nodeType > FirstUnary && nodeType < LastUnary;
}

// static
bool plExpressionAST::NodeType::IsBinary(Enum nodeType)
{
  return nodeType > FirstBinary && nodeType < LastBinary;
}

// static
bool plExpressionAST::NodeType::IsTernary(Enum nodeType)
{
  return nodeType > FirstTernary && nodeType < LastTernary;
}

// static
bool plExpressionAST::NodeType::IsConstant(Enum nodeType)
{
  return nodeType == Constant;
}

// static
bool plExpressionAST::NodeType::IsSwizzle(Enum nodeType)
{
  return nodeType == Swizzle;
}

// static
bool plExpressionAST::NodeType::IsInput(Enum nodeType)
{
  return nodeType == Input;
}

// static
bool plExpressionAST::NodeType::IsOutput(Enum nodeType)
{
  return nodeType == Output;
}

// static
bool plExpressionAST::NodeType::IsFunctionCall(Enum nodeType)
{
  return nodeType == FunctionCall;
}

// static
bool plExpressionAST::NodeType::IsConstructorCall(Enum nodeType)
{
  return nodeType == ConstructorCall;
}

// static
bool plExpressionAST::NodeType::IsCommutative(Enum nodeType)
{
  return nodeType == Add || nodeType == Multiply ||
         nodeType == Min || nodeType == Max ||
         nodeType == BitwiseAnd || nodeType == BitwiseXor || nodeType == BitwiseOr ||
         nodeType == Equal || nodeType == NotEqual ||
         nodeType == LogicalAnd || nodeType == LogicalOr;
}

// static
bool plExpressionAST::NodeType::AlwaysReturnsSingleElement(Enum nodeType)
{
  return nodeType == Length ||
         nodeType == All || nodeType == Any ||
         nodeType == Dot;
}

namespace
{
  static const char* s_szNodeTypeNames[] = {
    "Invalid",

    // Unary
    "",
    "Negate",
    "Absolute",
    "Saturate",
    "Sqrt",
    "Exp",
    "Ln",
    "Log2",
    "Log10",
    "Pow2",
    "Sin",
    "Cos",
    "Tan",
    "ASin",
    "ACos",
    "ATan",
    "RadToDeg",
    "DegToRad",
    "Round",
    "Floor",
    "Ceil",
    "Trunc",
    "Frac",
    "Length",
    "Normalize",
    "BitwiseNot",
    "LogicalNot",
    "All",
    "Any",
    "TypeConversion",
    "",

    // Binary
    "",
    "Add",
    "Subtract",
    "Multiply",
    "Divide",
    "Modulo",
    "Log",
    "Pow",
    "Min",
    "Max",
    "Dot",
    "Cross",
    "Reflect",
    "BitshiftLeft",
    "BitshiftRight",
    "BitwiseAnd",
    "BitwiseXor",
    "BitwiseOr",
    "Equal",
    "NotEqual",
    "Less",
    "LessEqual",
    "Greater",
    "GreaterEqual",
    "LogicalAnd",
    "LogicalOr",
    "",

    // Ternary
    "",
    "Clamp",
    "Select",
    "Lerp",
    "SmoothStep",
    "SmootherStep",
    "",

    "Constant",
    "Swizzle",
    "Input",
    "Output",

    "FunctionCall",
    "ConstructorCall",
  };

  static_assert(PL_ARRAY_SIZE(s_szNodeTypeNames) == plExpressionAST::NodeType::Count);

  static constexpr plUInt16 BuildSignature(plExpression::RegisterType::Enum returnType, plExpression::RegisterType::Enum a, plExpression::RegisterType::Enum b = plExpression::RegisterType::Unknown, plExpression::RegisterType::Enum c = plExpression::RegisterType::Unknown)
  {
    plUInt32 signature = static_cast<plUInt32>(returnType);
    signature |= a << plExpression::RegisterType::MaxNumBits * 1;
    signature |= b << plExpression::RegisterType::MaxNumBits * 2;
    signature |= c << plExpression::RegisterType::MaxNumBits * 3;
    return static_cast<plUInt16>(signature);
  }

  static constexpr plExpression::RegisterType::Enum GetReturnTypeFromSignature(plUInt16 uiSignature)
  {
    plUInt32 uiMask = PL_BIT(plExpression::RegisterType::MaxNumBits) - 1;
    return static_cast<plExpression::RegisterType::Enum>(uiSignature & uiMask);
  }

  static constexpr plExpression::RegisterType::Enum GetArgumentTypeFromSignature(plUInt16 uiSignature, plUInt32 uiArgumentIndex)
  {
    plUInt32 uiShift = plExpression::RegisterType::MaxNumBits * (uiArgumentIndex + 1);
    plUInt32 uiMask = PL_BIT(plExpression::RegisterType::MaxNumBits) - 1;
    return static_cast<plExpression::RegisterType::Enum>((uiSignature >> uiShift) & uiMask);
  }

#define SIG1(r, a) BuildSignature(plExpression::RegisterType::r, plExpression::RegisterType::a)
#define SIG2(r, a, b) BuildSignature(plExpression::RegisterType::r, plExpression::RegisterType::a, plExpression::RegisterType::b)
#define SIG3(r, a, b, c) BuildSignature(plExpression::RegisterType::r, plExpression::RegisterType::a, plExpression::RegisterType::b, plExpression::RegisterType::c)

  struct Overloads
  {
    plUInt16 m_Signatures[4] = {};
  };

  static Overloads s_NodeTypeOverloads[] = {
    {}, // Invalid,

    // Unary
    {},                                   // FirstUnary,
    {SIG1(Float, Float), SIG1(Int, Int)}, // Negate,
    {SIG1(Float, Float), SIG1(Int, Int)}, // Absolute,
    {SIG1(Float, Float), SIG1(Int, Int)}, // Saturate,
    {SIG1(Float, Float)},                 // Sqrt,
    {SIG1(Float, Float)},                 // Exp,
    {SIG1(Float, Float)},                 // Ln,
    {SIG1(Float, Float), SIG1(Int, Int)}, // Log2,
    {SIG1(Float, Float)},                 // Log10,
    {SIG1(Float, Float), SIG1(Int, Int)}, // Pow2,
    {SIG1(Float, Float)},                 // Sin,
    {SIG1(Float, Float)},                 // Cos,
    {SIG1(Float, Float)},                 // Tan,
    {SIG1(Float, Float)},                 // ASin,
    {SIG1(Float, Float)},                 // ACos,
    {SIG1(Float, Float)},                 // ATan,
    {SIG1(Float, Float)},                 // RadToDeg,
    {SIG1(Float, Float)},                 // DegToRad,
    {SIG1(Float, Float)},                 // Round,
    {SIG1(Float, Float)},                 // Floor,
    {SIG1(Float, Float)},                 // Ceil,
    {SIG1(Float, Float)},                 // Trunc,
    {SIG1(Float, Float)},                 // Frac,
    {SIG1(Float, Float)},                 // Length,
    {SIG1(Float, Float)},                 // Normalize,
    {SIG1(Int, Int)},                     // BitwiseNot,
    {SIG1(Bool, Bool)},                   // LogicalNot,
    {SIG1(Bool, Bool)},                   // All,
    {SIG1(Bool, Bool)},                   // Any,
    {},                                   // TypeConversion,
    {},                                   // LastUnary,

    // Binary
    {},                                                                       // FirstBinary,
    {SIG2(Float, Float, Float), SIG2(Int, Int, Int)},                         // Add,
    {SIG2(Float, Float, Float), SIG2(Int, Int, Int)},                         // Subtract,
    {SIG2(Float, Float, Float), SIG2(Int, Int, Int)},                         // Multiply,
    {SIG2(Float, Float, Float), SIG2(Int, Int, Int)},                         // Divide,
    {SIG2(Float, Float, Float), SIG2(Int, Int, Int)},                         // Modulo,
    {SIG2(Float, Float, Float)},                                              // Log,
    {SIG2(Float, Float, Float), SIG2(Int, Int, Int)},                         // Pow,
    {SIG2(Float, Float, Float), SIG2(Int, Int, Int)},                         // Min,
    {SIG2(Float, Float, Float), SIG2(Int, Int, Int)},                         // Max,
    {SIG2(Float, Float, Float), SIG2(Int, Int, Int)},                         // Dot,
    {SIG2(Float, Float, Float), SIG2(Int, Int, Int)},                         // Cross,
    {SIG2(Float, Float, Float), SIG2(Int, Int, Int)},                         // Reflect,
    {SIG2(Int, Int, Int)},                                                    // BitshiftLeft,
    {SIG2(Int, Int, Int)},                                                    // BitshiftRight,
    {SIG2(Int, Int, Int)},                                                    // BitwiseAnd,
    {SIG2(Int, Int, Int)},                                                    // BitwiseXor,
    {SIG2(Int, Int, Int)},                                                    // BitwiseOr,
    {SIG2(Bool, Float, Float), SIG2(Bool, Int, Int), SIG2(Bool, Bool, Bool)}, // Equal,
    {SIG2(Bool, Float, Float), SIG2(Bool, Int, Int), SIG2(Bool, Bool, Bool)}, // NotEqual,
    {SIG2(Bool, Float, Float), SIG2(Bool, Int, Int)},                         // Less,
    {SIG2(Bool, Float, Float), SIG2(Bool, Int, Int)},                         // LessEqual,
    {SIG2(Bool, Float, Float), SIG2(Bool, Int, Int)},                         // Greater,
    {SIG2(Bool, Float, Float), SIG2(Bool, Int, Int)},                         // GreaterEqual,
    {SIG2(Bool, Bool, Bool)},                                                 // LogicalAnd,
    {SIG2(Bool, Bool, Bool)},                                                 // LogicalOr,
    {},                                                                       // LastBinary,

    // Ternary
    {},                                                                                         // FirstTernary,
    {SIG3(Float, Float, Float, Float), SIG3(Int, Int, Int, Int)},                               // Clamp,
    {SIG3(Float, Bool, Float, Float), SIG3(Int, Bool, Int, Int), SIG3(Bool, Bool, Bool, Bool)}, // Select,
    {SIG3(Float, Float, Float, Float)},                                                         // Lerp,
    {SIG3(Float, Float, Float, Float)},                                                         // SmoothStep,
    {SIG3(Float, Float, Float, Float)},                                                         // SmootherStep,
    {},                                                                                         // LastTernary,

    {}, // Constant,
    {}, // Swizzle,
    {}, // Input,
    {}, // Output,

    {}, // FunctionCall,
    {}, // ConstructorCall,
  };

  static_assert(PL_ARRAY_SIZE(s_NodeTypeOverloads) == plExpressionAST::NodeType::Count);
} // namespace

#undef SIG1
#undef SIG2
#undef SIG3

// static
const char* plExpressionAST::NodeType::GetName(Enum nodeType)
{
  PL_ASSERT_DEBUG(nodeType >= 0 && nodeType < PL_ARRAY_SIZE(s_szNodeTypeNames), "Out of bounds access");
  return s_szNodeTypeNames[nodeType];
}

//////////////////////////////////////////////////////////////////////////

namespace
{
  static plVariantType::Enum s_DataTypeVariantTypes[] = {
    plVariantType::Invalid, // Unknown,
    plVariantType::Invalid, // Unknown2,
    plVariantType::Invalid, // Unknown3,
    plVariantType::Invalid, // Unknown4,

    plVariantType::Bool,    // Bool,
    plVariantType::Invalid, // Bool2,
    plVariantType::Invalid, // Bool3,
    plVariantType::Invalid, // Bool4,

    plVariantType::Int32,    // Int,
    plVariantType::Vector2I, // Int2,
    plVariantType::Vector3I, // Int3,
    plVariantType::Vector4I, // Int4,

    plVariantType::Float,   // Float,
    plVariantType::Vector2, // Float2,
    plVariantType::Vector3, // Float3,
    plVariantType::Vector4, // Float4,
  };
  static_assert(PL_ARRAY_SIZE(s_DataTypeVariantTypes) == (size_t)plExpressionAST::DataType::Count);

  static plExpressionAST::DataType::Enum s_DataTypeFromStreamType[] = {
    plExpressionAST::DataType::Float,  // Half,
    plExpressionAST::DataType::Float2, // Half2,
    plExpressionAST::DataType::Float3, // Half3,
    plExpressionAST::DataType::Float4, // Half4,

    plExpressionAST::DataType::Float,  // Float,
    plExpressionAST::DataType::Float2, // Float2,
    plExpressionAST::DataType::Float3, // Float3,
    plExpressionAST::DataType::Float4, // Float4,

    plExpressionAST::DataType::Int,  // Byte,
    plExpressionAST::DataType::Int2, // Byte2,
    plExpressionAST::DataType::Int3, // Byte3,
    plExpressionAST::DataType::Int4, // Byte4,

    plExpressionAST::DataType::Int,  // Short,
    plExpressionAST::DataType::Int2, // Short2,
    plExpressionAST::DataType::Int3, // Short3,
    plExpressionAST::DataType::Int4, // Short4,

    plExpressionAST::DataType::Int,  // Int,
    plExpressionAST::DataType::Int2, // Int2,
    plExpressionAST::DataType::Int3, // Int3,
    plExpressionAST::DataType::Int4, // Int4,
  };
  static_assert(PL_ARRAY_SIZE(s_DataTypeFromStreamType) == (size_t)plProcessingStream::DataType::Count);

  static_assert(plExpressionAST::DataType::Float >> 2 == plExpression::RegisterType::Float);
  static_assert(plExpressionAST::DataType::Int >> 2 == plExpression::RegisterType::Int);
  static_assert(plExpressionAST::DataType::Bool >> 2 == plExpression::RegisterType::Bool);
  static_assert(plExpressionAST::DataType::Unknown >> 2 == plExpression::RegisterType::Unknown);

  static const char* s_szDataTypeNames[] = {
    "Unknown",  // Unknown,
    "Unknown2", // Unknown2,
    "Unknown3", // Unknown3,
    "Unknown4", // Unknown4,

    "Bool",  // Bool,
    "Bool2", // Bool2,
    "Bool3", // Bool3,
    "Bool4", // Bool4,

    "Int",  // Int,
    "Int2", // Int2,
    "Int3", // Int3,
    "Int4", // Int4,

    "Float",  // Float,
    "Float2", // Float2,
    "Float3", // Float3,
    "Float4", // Float4,
  };

  static_assert(PL_ARRAY_SIZE(s_szDataTypeNames) == plExpressionAST::DataType::Count);
} // namespace


// static
plVariantType::Enum plExpressionAST::DataType::GetVariantType(Enum dataType)
{
  PL_ASSERT_DEBUG(dataType >= 0 && dataType < PL_ARRAY_SIZE(s_DataTypeVariantTypes), "Out of bounds access");
  return s_DataTypeVariantTypes[dataType];
}

// static
plExpressionAST::DataType::Enum plExpressionAST::DataType::FromStreamType(plProcessingStream::DataType dataType)
{
  PL_ASSERT_DEBUG(static_cast<plUInt32>(dataType) >= 0 && static_cast<plUInt32>(dataType) < PL_ARRAY_SIZE(s_DataTypeFromStreamType), "Out of bounds access");
  return s_DataTypeFromStreamType[static_cast<plUInt32>(dataType)];
}

// static
const char* plExpressionAST::DataType::GetName(Enum dataType)
{
  PL_ASSERT_DEBUG(dataType >= 0 && dataType < PL_ARRAY_SIZE(s_szDataTypeNames), "Out of bounds access");
  return s_szDataTypeNames[dataType];
}

//////////////////////////////////////////////////////////////////////////

namespace
{
  static const char* s_szVectorComponentNames[] = {
    "x",
    "y",
    "z",
    "w",
  };

  static const char* s_szVectorComponentAltNames[] = {
    "r",
    "g",
    "b",
    "a",
  };

  static_assert(PL_ARRAY_SIZE(s_szVectorComponentNames) == plExpressionAST::VectorComponent::Count);
  static_assert(PL_ARRAY_SIZE(s_szVectorComponentAltNames) == plExpressionAST::VectorComponent::Count);
} // namespace

// static
const char* plExpressionAST::VectorComponent::GetName(Enum vectorComponent)
{
  PL_ASSERT_DEBUG(vectorComponent >= 0 && vectorComponent < PL_ARRAY_SIZE(s_szVectorComponentNames), "Out of bounds access");
  return s_szVectorComponentNames[vectorComponent];
}

plExpressionAST::VectorComponent::Enum plExpressionAST::VectorComponent::FromChar(plUInt32 uiChar)
{
  for (plUInt32 i = 0; i < Count; ++i)
  {
    if (uiChar == s_szVectorComponentNames[i][0] || uiChar == s_szVectorComponentAltNames[i][0])
    {
      return static_cast<Enum>(i);
    }
  }

  return Count;
}

//////////////////////////////////////////////////////////////////////////

plExpressionAST::plExpressionAST()
  : m_Allocator("Expression AST", plFoundation::GetAlignedAllocator())
{
  static_assert(sizeof(Node) == 8);
#if PL_ENABLED(PL_PLATFORM_64BIT)
  static_assert(sizeof(UnaryOperator) == 16);
  static_assert(sizeof(BinaryOperator) == 24);
  static_assert(sizeof(TernaryOperator) == 32);
  static_assert(sizeof(Constant) == 32);
  static_assert(sizeof(Swizzle) == 24);
  static_assert(sizeof(Input) == 24);
  static_assert(sizeof(Output) == 32);
  static_assert(sizeof(FunctionCall) == 96);
  static_assert(sizeof(ConstructorCall) == 48);
#endif
}

plExpressionAST::~plExpressionAST() = default;

plExpressionAST::UnaryOperator* plExpressionAST::CreateUnaryOperator(NodeType::Enum type, Node* pOperand, DataType::Enum returnType /*= DataType::Unknown*/)
{
  PL_ASSERT_DEBUG(NodeType::IsUnary(type), "Type '{}' is not an unary operator", NodeType::GetName(type));

  auto pUnaryOperator = PL_NEW(&m_Allocator, UnaryOperator);
  pUnaryOperator->m_Type = type;
  pUnaryOperator->m_ReturnType = returnType;
  pUnaryOperator->m_pOperand = pOperand;

  ResolveOverloads(pUnaryOperator);

  return pUnaryOperator;
}

plExpressionAST::BinaryOperator* plExpressionAST::CreateBinaryOperator(NodeType::Enum type, Node* pLeftOperand, Node* pRightOperand)
{
  PL_ASSERT_DEBUG(NodeType::IsBinary(type), "Type '{}' is not a binary operator", NodeType::GetName(type));

  auto pBinaryOperator = PL_NEW(&m_Allocator, BinaryOperator);
  pBinaryOperator->m_Type = type;
  pBinaryOperator->m_ReturnType = DataType::Unknown;
  pBinaryOperator->m_pLeftOperand = pLeftOperand;
  pBinaryOperator->m_pRightOperand = pRightOperand;

  ResolveOverloads(pBinaryOperator);

  return pBinaryOperator;
}

plExpressionAST::TernaryOperator* plExpressionAST::CreateTernaryOperator(NodeType::Enum type, Node* pFirstOperand, Node* pSecondOperand, Node* pThirdOperand)
{
  PL_ASSERT_DEBUG(NodeType::IsTernary(type), "Type '{}' is not a ternary operator", NodeType::GetName(type));

  auto pTernaryOperator = PL_NEW(&m_Allocator, TernaryOperator);
  pTernaryOperator->m_Type = type;
  pTernaryOperator->m_ReturnType = DataType::Unknown;
  pTernaryOperator->m_pFirstOperand = pFirstOperand;
  pTernaryOperator->m_pSecondOperand = pSecondOperand;
  pTernaryOperator->m_pThirdOperand = pThirdOperand;

  ResolveOverloads(pTernaryOperator);

  return pTernaryOperator;
}

plExpressionAST::Constant* plExpressionAST::CreateConstant(const plVariant& value, DataType::Enum dataType /*= DataType::Float*/)
{
  plVariantType::Enum variantType = DataType::GetVariantType(dataType);
  PL_IGNORE_UNUSED(variantType);
  PL_ASSERT_DEV(variantType != plVariantType::Invalid, "Invalid constant type '{}'", DataType::GetName(dataType));

  auto pConstant = PL_NEW(&m_Allocator, Constant);
  pConstant->m_Type = NodeType::Constant;
  pConstant->m_ReturnType = dataType;
  pConstant->m_Value = value.ConvertTo(DataType::GetVariantType(dataType));

  PL_ASSERT_DEV(pConstant->m_Value.IsValid(), "Invalid constant value or conversion to target data type failed");

  return pConstant;
}

plExpressionAST::Swizzle* plExpressionAST::CreateSwizzle(plStringView sSwizzle, Node* pExpression)
{
  plEnum<VectorComponent> components[4];
  plUInt32 numComponents = 0;

  for (auto it : sSwizzle)
  {
    if (numComponents == PL_ARRAY_SIZE(components))
      return nullptr;

    plEnum<VectorComponent> component = VectorComponent::FromChar(it);
    if (component == VectorComponent::Count)
      return nullptr;

    components[numComponents] = component;
    ++numComponents;
  }

  return CreateSwizzle(plMakeArrayPtr(components, numComponents), pExpression);
}

plExpressionAST::Swizzle* plExpressionAST::CreateSwizzle(plEnum<VectorComponent> component, Node* pExpression)
{
  return CreateSwizzle(plMakeArrayPtr(&component, 1), pExpression);
}

plExpressionAST::Swizzle* plExpressionAST::CreateSwizzle(plArrayPtr<plEnum<VectorComponent>> swizzle, Node* pExpression)
{
  PL_ASSERT_DEV(swizzle.GetCount() >= 1 && swizzle.GetCount() <= 4, "Invalid number of vector components for swizzle.");
  PL_ASSERT_DEV(pExpression->m_ReturnType != DataType::Unknown, "Expression return type must be known.");

  auto pSwizzle = PL_NEW(&m_Allocator, Swizzle);
  pSwizzle->m_Type = NodeType::Swizzle;
  pSwizzle->m_ReturnType = DataType::FromRegisterType(DataType::GetRegisterType(pExpression->m_ReturnType), swizzle.GetCount());

  plMemoryUtils::Copy(pSwizzle->m_Components, swizzle.GetPtr(), swizzle.GetCount());
  pSwizzle->m_NumComponents = swizzle.GetCount();
  pSwizzle->m_pExpression = pExpression;

  return pSwizzle;
}

plExpressionAST::Input* plExpressionAST::CreateInput(const plExpression::StreamDesc& desc)
{
  auto pInput = PL_NEW(&m_Allocator, Input);
  pInput->m_Type = NodeType::Input;
  pInput->m_ReturnType = DataType::FromStreamType(desc.m_DataType);
  pInput->m_uiNumInputElements = static_cast<plUInt8>(DataType::GetElementCount(pInput->m_ReturnType));
  pInput->m_Desc = desc;

  return pInput;
}

plExpressionAST::Output* plExpressionAST::CreateOutput(const plExpression::StreamDesc& desc, Node* pExpression)
{
  auto pOutput = PL_NEW(&m_Allocator, Output);
  pOutput->m_Type = NodeType::Output;
  pOutput->m_ReturnType = DataType::FromStreamType(desc.m_DataType);
  pOutput->m_uiNumInputElements = static_cast<plUInt8>(DataType::GetElementCount(pOutput->m_ReturnType));
  pOutput->m_Desc = desc;
  pOutput->m_pExpression = pExpression;

  return pOutput;
}

plExpressionAST::FunctionCall* plExpressionAST::CreateFunctionCall(const plExpression::FunctionDesc& desc, plArrayPtr<Node*> arguments)
{
  return CreateFunctionCall(plMakeArrayPtr(&desc, 1), arguments);
}

plExpressionAST::FunctionCall* plExpressionAST::CreateFunctionCall(plArrayPtr<const plExpression::FunctionDesc> descs, plArrayPtr<Node*> arguments)
{
  auto pFunctionCall = PL_NEW(&m_Allocator, FunctionCall);
  pFunctionCall->m_Type = NodeType::FunctionCall;
  pFunctionCall->m_ReturnType = DataType::Unknown;

  for (auto& desc : descs)
  {
    auto it = m_FunctionDescs.Insert(desc);

    pFunctionCall->m_Descs.PushBack(&it.Key());
  }

  pFunctionCall->m_Arguments = arguments;

  ResolveOverloads(pFunctionCall);

  return pFunctionCall;
}

plExpressionAST::ConstructorCall* plExpressionAST::CreateConstructorCall(DataType::Enum dataType, plArrayPtr<Node*> arguments)
{
  PL_ASSERT_DEV(dataType >= DataType::Bool, "Invalid data type for constructor");

  auto pConstructorCall = PL_NEW(&m_Allocator, ConstructorCall);
  pConstructorCall->m_Type = NodeType::ConstructorCall;
  pConstructorCall->m_ReturnType = dataType;
  pConstructorCall->m_Arguments = arguments;

  ResolveOverloads(pConstructorCall);

  return pConstructorCall;
}

plExpressionAST::ConstructorCall* plExpressionAST::CreateConstructorCall(Node* pOldValue, Node* pNewValue, plStringView sPartialAssignmentMask)
{
  plExpression::RegisterType::Enum registerType = plExpression::RegisterType::Unknown;
  plSmallArray<Node*, 4> arguments;

  if (pOldValue != nullptr)
  {
    registerType = DataType::GetRegisterType(pOldValue->m_ReturnType);

    if (NodeType::IsConstructorCall(pOldValue->m_Type))
    {
      auto pConstructorCall = static_cast<ConstructorCall*>(pOldValue);
      arguments = pConstructorCall->m_Arguments;
    }
    else
    {
      const plUInt32 uiNumElements = DataType::GetElementCount(pOldValue->m_ReturnType);
      if (uiNumElements == 1)
      {
        arguments.PushBack(pOldValue);
      }
      else
      {
        for (plUInt32 i = 0; i < uiNumElements; ++i)
        {
          auto pSwizzle = CreateSwizzle(static_cast<VectorComponent::Enum>(i), pOldValue);
          arguments.PushBack(pSwizzle);
        }
      }
    }
  }

  const plUInt32 uiNewValueElementCount = DataType::GetElementCount(pNewValue->m_ReturnType);
  plUInt32 uiNewValueElementIndex = 0;
  for (auto it : sPartialAssignmentMask)
  {
    auto component = plExpressionAST::VectorComponent::FromChar(it);
    if (component == plExpressionAST::VectorComponent::Count)
    {
      return nullptr;
    }

    Node* pNewValueElement = nullptr;
    if (uiNewValueElementCount == 1)
    {
      pNewValueElement = pNewValue;
    }
    else
    {
      if (uiNewValueElementIndex >= uiNewValueElementCount)
      {
        return nullptr;
      }

      pNewValueElement = CreateSwizzle(static_cast<VectorComponent::Enum>(uiNewValueElementIndex), pNewValue);
      ++uiNewValueElementIndex;
    }

    plUInt32 componentIndex = component;
    if (componentIndex >= arguments.GetCount())
    {
      while (componentIndex > arguments.GetCount())
      {
        arguments.PushBack(CreateConstant(0));
      }

      arguments.PushBack(pNewValueElement);
    }
    else
    {
      arguments[componentIndex] = pNewValueElement;
    }

    if (pOldValue == nullptr)
    {
      registerType = plMath::Max(registerType, DataType::GetRegisterType(pNewValueElement->m_ReturnType));
    }
  }

  plEnum<DataType> newType = DataType::FromRegisterType(registerType, arguments.GetCount());
  return CreateConstructorCall(newType, arguments);
}

// static
plArrayPtr<plExpressionAST::Node*> plExpressionAST::GetChildren(Node* pNode)
{
  NodeType::Enum nodeType = pNode->m_Type;
  if (NodeType::IsUnary(nodeType))
  {
    auto& pChild = static_cast<UnaryOperator*>(pNode)->m_pOperand;
    return plMakeArrayPtr(&pChild, 1);
  }
  else if (NodeType::IsBinary(nodeType))
  {
    auto& pChildren = static_cast<BinaryOperator*>(pNode)->m_pLeftOperand;
    return plMakeArrayPtr(&pChildren, 2);
  }
  else if (NodeType::IsTernary(nodeType))
  {
    auto& pChildren = static_cast<TernaryOperator*>(pNode)->m_pFirstOperand;
    return plMakeArrayPtr(&pChildren, 3);
  }
  else if (NodeType::IsSwizzle(nodeType))
  {
    auto& pChild = static_cast<Swizzle*>(pNode)->m_pExpression;
    return plMakeArrayPtr(&pChild, 1);
  }
  else if (NodeType::IsOutput(nodeType))
  {
    auto& pChild = static_cast<Output*>(pNode)->m_pExpression;
    return plMakeArrayPtr(&pChild, 1);
  }
  else if (NodeType::IsFunctionCall(nodeType))
  {
    auto& args = static_cast<FunctionCall*>(pNode)->m_Arguments;
    return args;
  }
  else if (NodeType::IsConstructorCall(nodeType))
  {
    auto& args = static_cast<ConstructorCall*>(pNode)->m_Arguments;
    return args;
  }

  PL_ASSERT_DEV(NodeType::IsInput(nodeType) || NodeType::IsConstant(nodeType), "Unknown node type");
  return plArrayPtr<Node*>();
}

// static
plArrayPtr<const plExpressionAST::Node*> plExpressionAST::GetChildren(const Node* pNode)
{
  NodeType::Enum nodeType = pNode->m_Type;
  if (NodeType::IsUnary(nodeType))
  {
    auto& pChild = static_cast<const UnaryOperator*>(pNode)->m_pOperand;
    return plMakeArrayPtr((const Node**)&pChild, 1);
  }
  else if (NodeType::IsBinary(nodeType))
  {
    auto& pChildren = static_cast<const BinaryOperator*>(pNode)->m_pLeftOperand;
    return plMakeArrayPtr((const Node**)&pChildren, 2);
  }
  else if (NodeType::IsTernary(nodeType))
  {
    auto& pChildren = static_cast<const TernaryOperator*>(pNode)->m_pFirstOperand;
    return plMakeArrayPtr((const Node**)&pChildren, 3);
  }
  else if (NodeType::IsSwizzle(nodeType))
  {
    auto& pChild = static_cast<const Swizzle*>(pNode)->m_pExpression;
    return plMakeArrayPtr((const Node**)&pChild, 1);
  }
  else if (NodeType::IsOutput(nodeType))
  {
    auto& pChild = static_cast<const Output*>(pNode)->m_pExpression;
    return plMakeArrayPtr((const Node**)&pChild, 1);
  }
  else if (NodeType::IsFunctionCall(nodeType))
  {
    auto& args = static_cast<const FunctionCall*>(pNode)->m_Arguments;
    return plArrayPtr<const Node*>((const Node**)args.GetData(), args.GetCount());
  }
  else if (NodeType::IsConstructorCall(nodeType))
  {
    auto& args = static_cast<const ConstructorCall*>(pNode)->m_Arguments;
    return plArrayPtr<const Node*>((const Node**)args.GetData(), args.GetCount());
  }

  PL_ASSERT_DEV(NodeType::IsInput(nodeType) || NodeType::IsConstant(nodeType), "Unknown node type");
  return plArrayPtr<const Node*>();
}

namespace
{
  struct NodeInfo
  {
    PL_DECLARE_POD_TYPE();

    const plExpressionAST::Node* m_pNode;
    plUInt32 m_uiParentGraphNode;
  };
} // namespace

void plExpressionAST::PrintGraph(plDGMLGraph& inout_graph) const
{
  plHybridArray<NodeInfo, 64> nodeStack;

  plStringBuilder sTmp;
  for (auto pOutputNode : m_OutputNodes)
  {
    if (pOutputNode == nullptr)
      continue;

    sTmp = NodeType::GetName(pOutputNode->m_Type);
    sTmp.Append("(", DataType::GetName(pOutputNode->m_ReturnType), ")");
    sTmp.Append(": ", pOutputNode->m_Desc.m_sName);

    plDGMLGraph::NodeDesc nd;
    nd.m_Color = plColorScheme::LightUI(plColorScheme::Blue);
    plUInt32 uiGraphNode = inout_graph.AddNode(sTmp, &nd);

    nodeStack.PushBack({pOutputNode->m_pExpression, uiGraphNode});
  }

  plHashTable<const Node*, plUInt32> nodeCache;

  while (!nodeStack.IsEmpty())
  {
    NodeInfo currentNodeInfo = nodeStack.PeekBack();
    nodeStack.PopBack();

    plUInt32 uiGraphNode = 0;
    if (currentNodeInfo.m_pNode != nullptr)
    {
      if (!nodeCache.TryGetValue(currentNodeInfo.m_pNode, uiGraphNode))
      {
        NodeType::Enum nodeType = currentNodeInfo.m_pNode->m_Type;
        sTmp = NodeType::GetName(nodeType);
        sTmp.Append("(", DataType::GetName(currentNodeInfo.m_pNode->m_ReturnType), ")");
        plColor color = plColor::White;

        if (NodeType::IsConstant(nodeType))
        {
          sTmp.AppendFormat(": {0}", static_cast<const Constant*>(currentNodeInfo.m_pNode)->m_Value.ConvertTo<plString>());
        }
        else if (NodeType::IsSwizzle(nodeType))
        {
          auto pSwizzleNode = static_cast<const Swizzle*>(currentNodeInfo.m_pNode);
          sTmp.Append(": ");
          for (plUInt32 i = 0; i < pSwizzleNode->m_NumComponents; ++i)
          {
            sTmp.Append(VectorComponent::GetName(pSwizzleNode->m_Components[i]));
          }
        }
        else if (NodeType::IsInput(nodeType))
        {
          auto pInputNode = static_cast<const Input*>(currentNodeInfo.m_pNode);
          sTmp.Append(": ", pInputNode->m_Desc.m_sName);
          color = plColorScheme::LightUI(plColorScheme::Green);
        }
        else if (NodeType::IsFunctionCall(nodeType))
        {
          auto pFunctionCall = static_cast<const FunctionCall*>(currentNodeInfo.m_pNode);
          if (pFunctionCall->m_uiOverloadIndex != 0xFF)
          {
            auto pDesc = pFunctionCall->m_Descs[currentNodeInfo.m_pNode->m_uiOverloadIndex];
            sTmp.Append(": ", pDesc->GetMangledName());
          }
          else
          {
            sTmp.Append(": ", pFunctionCall->m_Descs[0]->m_sName);
          }
          color = plColorScheme::LightUI(plColorScheme::Yellow);
        }

        plDGMLGraph::NodeDesc nd;
        nd.m_Color = color;
        uiGraphNode = inout_graph.AddNode(sTmp, &nd);
        nodeCache.Insert(currentNodeInfo.m_pNode, uiGraphNode);

        // push children
        auto children = GetChildren(currentNodeInfo.m_pNode);
        for (auto pChild : children)
        {
          nodeStack.PushBack({pChild, uiGraphNode});
        }
      }
    }
    else
    {
      plDGMLGraph::NodeDesc nd;
      nd.m_Color = plColor::OrangeRed;
      uiGraphNode = inout_graph.AddNode("Invalid", &nd);
    }

    inout_graph.AddConnection(uiGraphNode, currentNodeInfo.m_uiParentGraphNode);
  }
}

void plExpressionAST::ResolveOverloads(Node* pNode)
{
  if (pNode->m_uiOverloadIndex != 0xFF)
  {
    // already resolved
    return;
  }

  const NodeType::Enum nodeType = pNode->m_Type;
  if (nodeType == NodeType::TypeConversion)
  {
    PL_ASSERT_DEV(pNode->m_ReturnType != DataType::Unknown, "Return type must be specified for conversion nodes");
    pNode->m_uiOverloadIndex = 0;
    return;
  }

  auto CalculateMatchDistance = [](plArrayPtr<Node*> children, plArrayPtr<const plEnum<plExpression::RegisterType>> expectedTypes, plUInt32 uiNumRequiredArgs, plUInt32& ref_uiMaxNumElements)
  {
    if (children.GetCount() < uiNumRequiredArgs)
    {
      return plInvalidIndex;
    }

    plUInt32 uiMatchDistance = 0;
    ref_uiMaxNumElements = 1;
    for (plUInt32 i = 0; i < plMath::Min(children.GetCount(), expectedTypes.GetCount()); ++i)
    {
      auto& pChildNode = children[i];
      PL_ASSERT_DEV(pChildNode != nullptr && pChildNode->m_ReturnType != DataType::Unknown, "Invalid child node");

      auto childType = DataType::GetRegisterType(pChildNode->m_ReturnType);
      int iDistance = expectedTypes[i] - childType;
      if (iDistance < 0)
      {
        // Penalty to prevent 'narrowing' conversions
        iDistance *= -plExpression::RegisterType::Count;
      }
      uiMatchDistance += iDistance;
      ref_uiMaxNumElements = plMath::Max(ref_uiMaxNumElements, DataType::GetElementCount(pChildNode->m_ReturnType));
    }
    return uiMatchDistance;
  };

  if (NodeType::IsUnary(nodeType) || NodeType::IsBinary(nodeType) || NodeType::IsTernary(nodeType))
  {
    auto children = GetChildren(pNode);
    plSmallArray<plEnum<plExpression::RegisterType>, 4> expectedTypes;
    plUInt32 uiBestMatchDistance = plInvalidIndex;

    for (plUInt32 uiSigIndex = 0; uiSigIndex < PL_ARRAY_SIZE(Overloads::m_Signatures); ++uiSigIndex)
    {
      const plUInt16 uiSignature = s_NodeTypeOverloads[nodeType].m_Signatures[uiSigIndex];
      if (uiSignature == 0)
        break;

      expectedTypes.Clear();
      for (plUInt32 i = 0; i < children.GetCount(); ++i)
      {
        expectedTypes.PushBack(GetArgumentTypeFromSignature(uiSignature, i));
      }

      plUInt32 uiMaxNumElements = 1;
      plUInt32 uiMatchDistance = CalculateMatchDistance(children, expectedTypes, expectedTypes.GetCount(), uiMaxNumElements);
      if (uiMatchDistance < uiBestMatchDistance)
      {
        const plUInt32 uiReturnTypeElements = NodeType::AlwaysReturnsSingleElement(nodeType) ? 1 : uiMaxNumElements;
        pNode->m_ReturnType = DataType::FromRegisterType(GetReturnTypeFromSignature(uiSignature), uiReturnTypeElements);
        pNode->m_uiNumInputElements = static_cast<plUInt8>(uiMaxNumElements);
        pNode->m_uiOverloadIndex = static_cast<plUInt8>(uiSigIndex);
        uiBestMatchDistance = uiMatchDistance;
      }
    }
  }
  else if (NodeType::IsFunctionCall(nodeType))
  {
    auto pFunctionCall = static_cast<FunctionCall*>(pNode);
    plUInt32 uiBestMatchDistance = plInvalidIndex;

    for (plUInt32 uiOverloadIndex = 0; uiOverloadIndex < pFunctionCall->m_Descs.GetCount(); ++uiOverloadIndex)
    {
      auto pFuncDesc = pFunctionCall->m_Descs[uiOverloadIndex];

      plUInt32 uiMaxNumElements = 1;
      plUInt32 uiMatchDistance = CalculateMatchDistance(pFunctionCall->m_Arguments, pFuncDesc->m_InputTypes, pFuncDesc->m_uiNumRequiredInputs, uiMaxNumElements);
      if (uiMatchDistance < uiBestMatchDistance)
      {
        pNode->m_ReturnType = DataType::FromRegisterType(pFuncDesc->m_OutputType, uiMaxNumElements);
        pNode->m_uiNumInputElements = static_cast<plUInt8>(uiMaxNumElements);
        pNode->m_uiOverloadIndex = static_cast<plUInt8>(uiOverloadIndex);
        uiBestMatchDistance = uiMatchDistance;
      }
    }

    if (pNode->m_ReturnType != DataType::Unknown)
    {
      auto pFuncDesc = pFunctionCall->m_Descs[pNode->m_uiOverloadIndex];

      // Trim arguments array to number of inputs
      if (pFunctionCall->m_Arguments.GetCount() > pFuncDesc->m_InputTypes.GetCount())
      {
        pFunctionCall->m_Arguments.SetCount(static_cast<plUInt16>(pFuncDesc->m_InputTypes.GetCount()));
      }
    }
  }
  else if (NodeType::IsConstructorCall(nodeType))
  {
    auto pConstructorCall = static_cast<ConstructorCall*>(pNode);
    auto& args = pConstructorCall->m_Arguments;
    const plUInt32 uiElementCount = plExpressionAST::DataType::GetElementCount(pNode->m_ReturnType);

    if (uiElementCount > 1 && args.GetCount() == 1 && plExpressionAST::DataType::GetElementCount(args[0]->m_ReturnType) == 1)
    {
      for (plUInt32 i = 0; i < uiElementCount - 1; ++i)
      {
        pConstructorCall->m_Arguments.PushBack(args[0]);
      }

      return;
    }

    plSmallArray<Node*, 4> newArguments;
    Node* pZero = nullptr;

    plUInt32 uiArgumentIndex = 0;
    plUInt32 uiArgumentElementIndex = 0;

    for (plUInt32 i = 0; i < uiElementCount; ++i)
    {
      if (uiArgumentIndex < args.GetCount())
      {
        auto pArg = args[uiArgumentIndex];
        PL_ASSERT_DEV(pArg != nullptr && pArg->m_ReturnType != DataType::Unknown, "Invalid argument node");

        const plUInt32 uiArgElementCount = plExpressionAST::DataType::GetElementCount(pArg->m_ReturnType);
        if (uiArgElementCount == 1)
        {
          newArguments.PushBack(pArg);
        }
        else if (uiArgumentElementIndex < uiArgElementCount)
        {
          newArguments.PushBack(CreateSwizzle(static_cast<VectorComponent::Enum>(uiArgumentElementIndex), pArg));
        }

        ++uiArgumentElementIndex;
        if (uiArgumentElementIndex >= uiArgElementCount)
        {
          ++uiArgumentIndex;
          uiArgumentElementIndex = 0;
        }
      }
      else
      {
        if (pZero == nullptr)
        {
          pZero = CreateConstant(0);
        }
        newArguments.PushBack(pZero);
      }
    }

    PL_ASSERT_DEBUG(newArguments.GetCount() == uiElementCount, "Not enough arguments");
    pConstructorCall->m_Arguments = newArguments;
  }
}

// static
plExpressionAST::DataType::Enum plExpressionAST::GetExpectedChildDataType(const Node* pNode, plUInt32 uiChildIndex)
{
  const NodeType::Enum nodeType = pNode->m_Type;
  const DataType::Enum returnType = pNode->m_ReturnType;
  const plUInt32 uiOverloadIndex = pNode->m_uiOverloadIndex;
  PL_ASSERT_DEV(returnType != DataType::Unknown, "Return type must not be unknown");

  if (nodeType == NodeType::TypeConversion || NodeType::IsSwizzle(nodeType))
  {
    return DataType::Unknown;
  }
  else if (NodeType::IsUnary(nodeType) || NodeType::IsBinary(nodeType) || NodeType::IsTernary(nodeType))
  {
    PL_ASSERT_DEV(uiOverloadIndex != 0xFF, "Unresolved overload");
    plUInt16 uiSignature = s_NodeTypeOverloads[nodeType].m_Signatures[uiOverloadIndex];
    return DataType::FromRegisterType(GetArgumentTypeFromSignature(uiSignature, uiChildIndex), pNode->m_uiNumInputElements);
  }
  else if (NodeType::IsOutput(nodeType))
  {
    return returnType;
  }
  else if (NodeType::IsFunctionCall(nodeType))
  {
    PL_ASSERT_DEV(uiOverloadIndex != 0xFF, "Unresolved overload");

    auto pDesc = static_cast<const FunctionCall*>(pNode)->m_Descs[uiOverloadIndex];
    return DataType::FromRegisterType(pDesc->m_InputTypes[uiChildIndex], pNode->m_uiNumInputElements);
  }
  else if (NodeType::IsConstructorCall(nodeType))
  {
    return DataType::FromRegisterType(DataType::GetRegisterType(returnType));
  }

  PL_ASSERT_NOT_IMPLEMENTED;
  return DataType::Unknown;
}

// static
void plExpressionAST::UpdateHash(Node* pNode)
{
  plHybridArray<plUInt32, 16> valuesToHash;

  const plUInt32* pBaseValues = reinterpret_cast<const plUInt32*>(pNode);
  valuesToHash.PushBack(pBaseValues[0]);
  valuesToHash.PushBack(pBaseValues[1]);

  NodeType::Enum nodeType = pNode->m_Type;
  if (NodeType::IsUnary(nodeType))
  {
    auto pUnary = static_cast<const UnaryOperator*>(pNode);
    valuesToHash.PushBack(pUnary->m_pOperand->m_uiHash);
  }
  else if (NodeType::IsBinary(nodeType))
  {
    auto pBinary = static_cast<const BinaryOperator*>(pNode);
    plUInt32 uiHashLeft = pBinary->m_pLeftOperand->m_uiHash;
    plUInt32 uiHashRight = pBinary->m_pRightOperand->m_uiHash;

    // Sort by hash value for commutative operations so operand order doesn't matter
    if (NodeType::IsCommutative(nodeType) && uiHashLeft > uiHashRight)
    {
      plMath::Swap(uiHashLeft, uiHashRight);
    }

    valuesToHash.PushBack(uiHashLeft);
    valuesToHash.PushBack(uiHashRight);
  }
  else if (NodeType::IsTernary(nodeType))
  {
    auto pTernary = static_cast<const TernaryOperator*>(pNode);
    valuesToHash.PushBack(pTernary->m_pFirstOperand->m_uiHash);
    valuesToHash.PushBack(pTernary->m_pSecondOperand->m_uiHash);
    valuesToHash.PushBack(pTernary->m_pThirdOperand->m_uiHash);
  }
  else if (NodeType::IsConstant(nodeType))
  {
    auto pConstant = static_cast<const Constant*>(pNode);
    const plUInt64 uiValueHash = pConstant->m_Value.ComputeHash();
    valuesToHash.PushBack(static_cast<plUInt32>(uiValueHash));
    valuesToHash.PushBack(static_cast<plUInt32>(uiValueHash >> 32u));
  }
  else if (NodeType::IsInput(nodeType))
  {
    auto pInput = static_cast<const Input*>(pNode);
    const plUInt64 uiNameHash = pInput->m_Desc.m_sName.GetHash();
    valuesToHash.PushBack(static_cast<plUInt32>(uiNameHash));
    valuesToHash.PushBack(static_cast<plUInt32>(uiNameHash >> 32u));
  }
  else if (NodeType::IsOutput(nodeType))
  {
    auto pOutput = static_cast<const Output*>(pNode);
    const plUInt64 uiNameHash = pOutput->m_Desc.m_sName.GetHash();
    valuesToHash.PushBack(static_cast<plUInt32>(uiNameHash));
    valuesToHash.PushBack(static_cast<plUInt32>(uiNameHash >> 32u));
    valuesToHash.PushBack(pOutput->m_pExpression->m_uiHash);
  }
  else if (NodeType::IsFunctionCall(nodeType))
  {
    auto pFunctionCall = static_cast<const FunctionCall*>(pNode);
    const plUInt64 uiNameHash = pFunctionCall->m_Descs[0]->m_sName.GetHash();
    valuesToHash.PushBack(static_cast<plUInt32>(uiNameHash));
    valuesToHash.PushBack(static_cast<plUInt32>(uiNameHash >> 32u));

    for (auto pArg : pFunctionCall->m_Arguments)
    {
      valuesToHash.PushBack(pArg->m_uiHash);
    }
  }
  else
  {
    PL_ASSERT_NOT_IMPLEMENTED;
  }

  pNode->m_uiHash = plHashingUtils::xxHash32(valuesToHash.GetData(), valuesToHash.GetCount() * sizeof(plUInt32));
}

// static
bool plExpressionAST::IsEqual(const Node* pNodeA, const Node* pNodeB)
{
  const plUInt32 uiBaseValuesA = *reinterpret_cast<const plUInt32*>(pNodeA);
  const plUInt32 uiBaseValuesB = *reinterpret_cast<const plUInt32*>(pNodeB);
  if (uiBaseValuesA != uiBaseValuesB)
  {
    return false;
  }

  NodeType::Enum nodeType = pNodeA->m_Type;
  if (NodeType::IsUnary(nodeType))
  {
    auto pUnaryA = static_cast<const UnaryOperator*>(pNodeA);
    auto pUnaryB = static_cast<const UnaryOperator*>(pNodeB);

    return pUnaryA->m_pOperand == pUnaryB->m_pOperand;
  }
  else if (NodeType::IsBinary(nodeType))
  {
    auto pBinaryA = static_cast<const BinaryOperator*>(pNodeA);
    auto pBinaryB = static_cast<const BinaryOperator*>(pNodeB);

    auto pLeftA = pBinaryA->m_pLeftOperand;
    auto pLeftB = pBinaryB->m_pLeftOperand;
    auto pRightA = pBinaryA->m_pRightOperand;
    auto pRightB = pBinaryB->m_pRightOperand;

    if (NodeType::IsCommutative(nodeType))
    {
      if (pLeftA > pRightA)
        plMath::Swap(pLeftA, pRightA);

      if (pLeftB > pRightB)
        plMath::Swap(pLeftB, pRightB);
    }

    return pLeftA == pLeftB && pRightA == pRightB;
  }
  else if (NodeType::IsTernary(nodeType))
  {
    auto pTernaryA = static_cast<const TernaryOperator*>(pNodeA);
    auto pTernaryB = static_cast<const TernaryOperator*>(pNodeB);

    return pTernaryA->m_pFirstOperand == pTernaryB->m_pFirstOperand &&
           pTernaryA->m_pSecondOperand == pTernaryB->m_pSecondOperand &&
           pTernaryA->m_pThirdOperand == pTernaryB->m_pThirdOperand;
  }
  else if (NodeType::IsConstant(nodeType))
  {
    auto pConstantA = static_cast<const Constant*>(pNodeA);
    auto pConstantB = static_cast<const Constant*>(pNodeB);

    return pConstantA->m_Value == pConstantB->m_Value;
  }
  else if (NodeType::IsInput(nodeType))
  {
    auto pInputA = static_cast<const Input*>(pNodeA);
    auto pInputB = static_cast<const Input*>(pNodeB);

    return pInputA->m_Desc == pInputB->m_Desc;
  }
  else if (NodeType::IsOutput(nodeType))
  {
    auto pOutputA = static_cast<const Output*>(pNodeA);
    auto pOutputB = static_cast<const Output*>(pNodeB);

    return pOutputA->m_Desc == pOutputB->m_Desc && pOutputA->m_pExpression == pOutputB->m_pExpression;
  }
  else if (NodeType::IsFunctionCall(nodeType))
  {
    auto pFunctionCallA = static_cast<const FunctionCall*>(pNodeA);
    auto pFunctionCallB = static_cast<const FunctionCall*>(pNodeB);

    return pFunctionCallA->m_Descs[pFunctionCallA->m_uiOverloadIndex] == pFunctionCallB->m_Descs[pFunctionCallB->m_uiOverloadIndex] &&
           pFunctionCallA->m_Arguments == pFunctionCallB->m_Arguments;
  }

  PL_ASSERT_NOT_IMPLEMENTED;
  return false;
}
