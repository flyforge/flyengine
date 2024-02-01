#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <Foundation/Containers/Blob.h>

class plStreamWriter;
class plStreamReader;

class PL_FOUNDATION_DLL plExpressionByteCode
{
public:
  struct OpCode
  {
    enum Enum
    {
      Nop,

      FirstUnary,

      AbsF_R,
      AbsI_R,
      SqrtF_R,

      ExpF_R,
      LnF_R,
      Log2F_R,
      Log2I_R,
      Log10F_R,
      Pow2F_R,

      SinF_R,
      CosF_R,
      TanF_R,

      ASinF_R,
      ACosF_R,
      ATanF_R,

      RoundF_R,
      FloorF_R,
      CeilF_R,
      TruncF_R,

      NotI_R,
      NotB_R,

      IToF_R,
      FToI_R,

      LastUnary,

      FirstBinary,

      AddF_RR,
      AddI_RR,

      SubF_RR,
      SubI_RR,

      MulF_RR,
      MulI_RR,

      DivF_RR,
      DivI_RR,

      MinF_RR,
      MinI_RR,

      MaxF_RR,
      MaxI_RR,

      ShlI_RR,
      ShrI_RR,
      AndI_RR,
      XorI_RR,
      OrI_RR,

      EqF_RR,
      EqI_RR,
      EqB_RR,

      NEqF_RR,
      NEqI_RR,
      NEqB_RR,

      LtF_RR,
      LtI_RR,

      LEqF_RR,
      LEqI_RR,

      GtF_RR,
      GtI_RR,

      GEqF_RR,
      GEqI_RR,

      AndB_RR,
      OrB_RR,

      LastBinary,

      FirstBinaryWithConstant,

      AddF_RC,
      AddI_RC,

      SubF_RC,
      SubI_RC,

      MulF_RC,
      MulI_RC,

      DivF_RC,
      DivI_RC,

      MinF_RC,
      MinI_RC,

      MaxF_RC,
      MaxI_RC,

      ShlI_RC,
      ShrI_RC,
      AndI_RC,
      XorI_RC,
      OrI_RC,

      EqF_RC,
      EqI_RC,
      EqB_RC,

      NEqF_RC,
      NEqI_RC,
      NEqB_RC,

      LtF_RC,
      LtI_RC,

      LEqF_RC,
      LEqI_RC,

      GtF_RC,
      GtI_RC,

      GEqF_RC,
      GEqI_RC,

      AndB_RC,
      OrB_RC,

      LastBinaryWithConstant,

      FirstTernary,

      SelF_RRR,
      SelI_RRR,
      SelB_RRR,

      LastTernary,

      FirstSpecial,

      MovX_R,
      MovX_C,
      LoadF,
      LoadI,
      StoreF,
      StoreI,

      Call,

      LastSpecial,

      Count
    };

    static const char* GetName(Enum code);
  };

  using StorageType = plUInt32;

  plExpressionByteCode();
  plExpressionByteCode(const plExpressionByteCode& other);
  ~plExpressionByteCode();

  void operator=(const plExpressionByteCode& other);

  bool operator==(const plExpressionByteCode& other) const;
  bool operator!=(const plExpressionByteCode& other) const { return !(*this == other); }

  void Clear();
  bool IsEmpty() const { return m_uiByteCodeCount == 0; }

  const StorageType* GetByteCodeStart() const;
  const StorageType* GetByteCodeEnd() const;
  plArrayPtr<const StorageType> GetByteCode() const;

  plUInt32 GetNumInstructions() const;
  plUInt32 GetNumTempRegisters() const;
  plArrayPtr<const plExpression::StreamDesc> GetInputs() const;
  plArrayPtr<const plExpression::StreamDesc> GetOutputs() const;
  plArrayPtr<const plExpression::FunctionDesc> GetFunctions() const;

  static OpCode::Enum GetOpCode(const StorageType*& ref_pByteCode);
  static plUInt32 GetRegisterIndex(const StorageType*& ref_pByteCode);
  static plExpression::Register GetConstant(const StorageType*& ref_pByteCode);
  static plUInt32 GetFunctionIndex(const StorageType*& ref_pByteCode);
  static plUInt32 GetFunctionArgCount(const StorageType*& ref_pByteCode);

  void Disassemble(plStringBuilder& out_sDisassembly) const;

  plResult Save(plStreamWriter& inout_stream) const;
  plResult Load(plStreamReader& inout_stream, plByteArrayPtr externalMemory = plByteArrayPtr());

  plConstByteBlobPtr GetDataBlob() const { return m_Data.GetByteBlobPtr(); }

private:
  friend class plExpressionCompiler;

  void Init(plArrayPtr<const StorageType> byteCode, plArrayPtr<const plExpression::StreamDesc> inputs, plArrayPtr<const plExpression::StreamDesc> outputs, plArrayPtr<const plExpression::FunctionDesc> functions, plUInt32 uiNumTempRegisters, plUInt32 uiNumInstructions);

  plBlob m_Data;

  plExpression::StreamDesc* m_pInputs = nullptr;
  plExpression::StreamDesc* m_pOutputs = nullptr;
  plExpression::FunctionDesc* m_pFunctions = nullptr;
  StorageType* m_pByteCode = nullptr;

  plUInt32 m_uiByteCodeCount = 0;
  plUInt16 m_uiNumInputs = 0;
  plUInt16 m_uiNumOutputs = 0;
  plUInt16 m_uiNumFunctions = 0;

  plUInt16 m_uiNumTempRegisters = 0;
  plUInt32 m_uiNumInstructions = 0;
};

#if PL_ENABLED(PL_PLATFORM_64BIT)
static_assert(sizeof(plExpressionByteCode) == 64);
#endif

PL_DECLARE_REFLECTABLE_TYPE(PL_FOUNDATION_DLL, plExpressionByteCode);
PL_DECLARE_CUSTOM_VARIANT_TYPE(plExpressionByteCode);

#include <Foundation/CodeUtils/Expression/Implementation/ExpressionByteCode_inl.h>
