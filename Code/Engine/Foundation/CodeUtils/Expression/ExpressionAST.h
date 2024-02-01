#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <Foundation/Memory/LinearAllocator.h>

class plDGMLGraph;

class PL_FOUNDATION_DLL plExpressionAST
{
public:
  struct NodeType
  {
    using StorageType = plUInt8;

    enum Enum
    {
      Invalid,
      Default = Invalid,

      // Unary
      FirstUnary,
      Negate,
      Absolute,
      Saturate,
      Sqrt,
      Exp,
      Ln,
      Log2,
      Log10,
      Pow2,
      Sin,
      Cos,
      Tan,
      ASin,
      ACos,
      ATan,
      RadToDeg,
      DegToRad,
      Round,
      Floor,
      Ceil,
      Trunc,
      Frac,
      Length,
      Normalize,
      BitwiseNot,
      LogicalNot,
      All,
      Any,
      TypeConversion,
      LastUnary,

      // Binary
      FirstBinary,
      Add,
      Subtract,
      Multiply,
      Divide,
      Modulo,
      Log,
      Pow,
      Min,
      Max,
      Dot,
      Cross,
      Reflect,
      BitshiftLeft,
      BitshiftRight,
      BitwiseAnd,
      BitwiseXor,
      BitwiseOr,
      Equal,
      NotEqual,
      Less,
      LessEqual,
      Greater,
      GreaterEqual,
      LogicalAnd,
      LogicalOr,
      LastBinary,

      // Ternary
      FirstTernary,
      Clamp,
      Select,
      Lerp,
      SmoothStep,
      SmootherStep,
      LastTernary,

      Constant,
      Swizzle,
      Input,
      Output,

      FunctionCall,
      ConstructorCall,

      Count
    };

    static bool IsUnary(Enum nodeType);
    static bool IsBinary(Enum nodeType);
    static bool IsTernary(Enum nodeType);
    static bool IsConstant(Enum nodeType);
    static bool IsSwizzle(Enum nodeType);
    static bool IsInput(Enum nodeType);
    static bool IsOutput(Enum nodeType);
    static bool IsFunctionCall(Enum nodeType);
    static bool IsConstructorCall(Enum nodeType);

    static bool IsCommutative(Enum nodeType);
    static bool AlwaysReturnsSingleElement(Enum nodeType);

    static const char* GetName(Enum nodeType);
  };

  struct DataType
  {
    using StorageType = plUInt8;

    enum Enum
    {
      Unknown,
      Unknown2,
      Unknown3,
      Unknown4,

      Bool,
      Bool2,
      Bool3,
      Bool4,

      Int,
      Int2,
      Int3,
      Int4,

      Float,
      Float2,
      Float3,
      Float4,

      Count,

      Default = Unknown,
    };

    static plVariantType::Enum GetVariantType(Enum dataType);

    static Enum FromStreamType(plProcessingStream::DataType dataType);

    PL_ALWAYS_INLINE static plExpression::RegisterType::Enum GetRegisterType(Enum dataType)
    {
      return static_cast<plExpression::RegisterType::Enum>(dataType >> 2);
    }

    PL_ALWAYS_INLINE static Enum FromRegisterType(plExpression::RegisterType::Enum registerType, plUInt32 uiElementCount = 1)
    {
      return static_cast<plExpressionAST::DataType::Enum>((registerType << 2) + uiElementCount - 1);
    }

    PL_ALWAYS_INLINE static plUInt32 GetElementCount(Enum dataType) { return (dataType & 0x3) + 1; }

    static const char* GetName(Enum dataType);
  };

  struct VectorComponent
  {
    using StorageType = plUInt8;

    enum Enum
    {
      X,
      Y,
      Z,
      W,

      R = X,
      G = Y,
      B = Z,
      A = W,

      Count,

      Default = X
    };

    static const char* GetName(Enum vectorComponent);

    static Enum FromChar(plUInt32 uiChar);
  };

  struct Node
  {
    plEnum<NodeType> m_Type;
    plEnum<DataType> m_ReturnType;
    plUInt8 m_uiOverloadIndex = 0xFF;
    plUInt8 m_uiNumInputElements = 0;

    plUInt32 m_uiHash = 0;
  };

  struct UnaryOperator : public Node
  {
    Node* m_pOperand = nullptr;
  };

  struct BinaryOperator : public Node
  {
    Node* m_pLeftOperand = nullptr;
    Node* m_pRightOperand = nullptr;
  };

  struct TernaryOperator : public Node
  {
    Node* m_pFirstOperand = nullptr;
    Node* m_pSecondOperand = nullptr;
    Node* m_pThirdOperand = nullptr;
  };

  struct Constant : public Node
  {
    plVariant m_Value;
  };

  struct Swizzle : public Node
  {
    plEnum<VectorComponent> m_Components[4];
    plUInt32 m_NumComponents = 0;
    Node* m_pExpression = nullptr;
  };

  struct Input : public Node
  {
    plExpression::StreamDesc m_Desc;
  };

  struct Output : public Node
  {
    plExpression::StreamDesc m_Desc;
    Node* m_pExpression = nullptr;
  };

  struct FunctionCall : public Node
  {
    plSmallArray<const plExpression::FunctionDesc*, 1> m_Descs;
    plSmallArray<Node*, 8> m_Arguments;
  };

  struct ConstructorCall : public Node
  {
    plSmallArray<Node*, 4> m_Arguments;
  };

public:
  plExpressionAST();
  ~plExpressionAST();

  UnaryOperator* CreateUnaryOperator(NodeType::Enum type, Node* pOperand, DataType::Enum returnType = DataType::Unknown);
  BinaryOperator* CreateBinaryOperator(NodeType::Enum type, Node* pLeftOperand, Node* pRightOperand);
  TernaryOperator* CreateTernaryOperator(NodeType::Enum type, Node* pFirstOperand, Node* pSecondOperand, Node* pThirdOperand);
  Constant* CreateConstant(const plVariant& value, DataType::Enum dataType = DataType::Float);
  Swizzle* CreateSwizzle(plStringView sSwizzle, Node* pExpression);
  Swizzle* CreateSwizzle(plEnum<VectorComponent> component, Node* pExpression);
  Swizzle* CreateSwizzle(plArrayPtr<plEnum<VectorComponent>> swizzle, Node* pExpression);
  Input* CreateInput(const plExpression::StreamDesc& desc);
  Output* CreateOutput(const plExpression::StreamDesc& desc, Node* pExpression);
  FunctionCall* CreateFunctionCall(const plExpression::FunctionDesc& desc, plArrayPtr<Node*> arguments);
  FunctionCall* CreateFunctionCall(plArrayPtr<const plExpression::FunctionDesc> descs, plArrayPtr<Node*> arguments);
  ConstructorCall* CreateConstructorCall(DataType::Enum dataType, plArrayPtr<Node*> arguments);
  ConstructorCall* CreateConstructorCall(Node* pOldValue, Node* pNewValue, plStringView sPartialAssignmentMask);

  static plArrayPtr<Node*> GetChildren(Node* pNode);
  static plArrayPtr<const Node*> GetChildren(const Node* pNode);

  void PrintGraph(plDGMLGraph& inout_graph) const;

  plSmallArray<Input*, 8> m_InputNodes;
  plSmallArray<Output*, 8> m_OutputNodes;

  // Transforms
  Node* TypeDeductionAndConversion(Node* pNode);
  Node* ReplaceVectorInstructions(Node* pNode);
  Node* ScalarizeVectorInstructions(Node* pNode);
  Node* ReplaceUnsupportedInstructions(Node* pNode);
  Node* FoldConstants(Node* pNode);
  Node* CommonSubexpressionElimination(Node* pNode);
  Node* Validate(Node* pNode);

  plResult ScalarizeInputs();
  plResult ScalarizeOutputs();

private:
  void ResolveOverloads(Node* pNode);

  static DataType::Enum GetExpectedChildDataType(const Node* pNode, plUInt32 uiChildIndex);

  static void UpdateHash(Node* pNode);
  static bool IsEqual(const Node* pNodeA, const Node* pNodeB);

  plLinearAllocator<> m_Allocator;

  plSet<plExpression::FunctionDesc> m_FunctionDescs;

  plHashTable<plUInt32, plSmallArray<Node*, 1>> m_NodeDeduplicationTable;
};
