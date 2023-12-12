#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/UniquePtr.h>

namespace
{
  static plUInt32 s_uiNumASTDumps = 0;

  void MakeASTOutputPath(plStringView sOutputName, plStringBuilder& out_sOutputPath)
  {
    plUInt32 uiCounter = s_uiNumASTDumps;
    ++s_uiNumASTDumps;

    out_sOutputPath.Format(":output/Expression/{}_{}_AST.dgml", plArgU(uiCounter, 2, true), sOutputName);
  }

  void DumpDisassembly(const plExpressionByteCode& byteCode, plStringView sOutputName, plUInt32 uiCounter)
  {
    plStringBuilder sDisassembly;
    byteCode.Disassemble(sDisassembly);

    plStringBuilder sFileName;
    sFileName.Format(":output/Expression/{}_{}_ByteCode.txt", plArgU(uiCounter, 2, true), sOutputName);

    plFileWriter fileWriter;
    if (fileWriter.Open(sFileName).Succeeded())
    {
      fileWriter.WriteBytes(sDisassembly.GetData(), sDisassembly.GetElementCount()).IgnoreResult();

      plLog::Error("Disassembly was dumped to: {}", sFileName);
    }
    else
    {
      plLog::Error("Failed to dump Disassembly to: {}", sFileName);
    }
  }

  static plUInt32 s_uiNumByteCodeComparisons = 0;

  bool CompareByteCode(const plExpressionByteCode& testCode, const plExpressionByteCode& referenceCode)
  {
    plUInt32 uiCounter = s_uiNumByteCodeComparisons;
    ++s_uiNumByteCodeComparisons;

    if (testCode != referenceCode)
    {
      DumpDisassembly(referenceCode, "Reference", uiCounter);
      DumpDisassembly(testCode, "Test", uiCounter);
      return false;
    }

    return true;
  }

  static plHashedString s_sA = plMakeHashedString("a");
  static plHashedString s_sB = plMakeHashedString("b");
  static plHashedString s_sC = plMakeHashedString("c");
  static plHashedString s_sD = plMakeHashedString("d");
  static plHashedString s_sOutput = plMakeHashedString("output");

  static plUniquePtr<plExpressionParser> s_pParser;
  static plUniquePtr<plExpressionCompiler> s_pCompiler;
  static plUniquePtr<plExpressionVM> s_pVM;

  template <typename T>
  struct StreamDataTypeDeduction
  {
  };

  template <>
  struct StreamDataTypeDeduction<plFloat16>
  {
    static constexpr plProcessingStream::DataType Type = plProcessingStream::DataType::Half;
    static plFloat16 Default() { return plMath::MinValue<float>(); }
  };

  template <>
  struct StreamDataTypeDeduction<float>
  {
    static constexpr plProcessingStream::DataType Type = plProcessingStream::DataType::Float;
    static float Default() { return plMath::MinValue<float>(); }
  };

  template <>
  struct StreamDataTypeDeduction<plInt8>
  {
    static constexpr plProcessingStream::DataType Type = plProcessingStream::DataType::Byte;
    static plInt8 Default() { return plMath::MinValue<plInt8>(); }
  };

  template <>
  struct StreamDataTypeDeduction<plInt16>
  {
    static constexpr plProcessingStream::DataType Type = plProcessingStream::DataType::Short;
    static plInt16 Default() { return plMath::MinValue<plInt16>(); }
  };

  template <>
  struct StreamDataTypeDeduction<int>
  {
    static constexpr plProcessingStream::DataType Type = plProcessingStream::DataType::Int;
    static int Default() { return plMath::MinValue<int>(); }
  };

  template <>
  struct StreamDataTypeDeduction<plVec3>
  {
    static constexpr plProcessingStream::DataType Type = plProcessingStream::DataType::Float3;
    static plVec3 Default() { return plVec3(plMath::MinValue<float>()); }
  };

  template <>
  struct StreamDataTypeDeduction<plVec3I32>
  {
    static constexpr plProcessingStream::DataType Type = plProcessingStream::DataType::Int3;
    static plVec3I32 Default() { return plVec3I32(plMath::MinValue<int>()); }
  };

  template <typename T>
  void Compile(plStringView sCode, plExpressionByteCode& out_byteCode, plStringView sDumpAstOutputName = plStringView())
  {
    plExpression::StreamDesc inputs[] = {
      {s_sA, StreamDataTypeDeduction<T>::Type},
      {s_sB, StreamDataTypeDeduction<T>::Type},
      {s_sC, StreamDataTypeDeduction<T>::Type},
      {s_sD, StreamDataTypeDeduction<T>::Type},
    };

    plExpression::StreamDesc outputs[] = {
      {s_sOutput, StreamDataTypeDeduction<T>::Type},
    };

    plExpressionAST ast;
    PLASMA_TEST_BOOL(s_pParser->Parse(sCode, inputs, outputs, {}, ast).Succeeded());

    plStringBuilder sOutputPath;
    if (sDumpAstOutputName.IsEmpty() == false)
    {
      MakeASTOutputPath(sDumpAstOutputName, sOutputPath);
    }
    PLASMA_TEST_BOOL(s_pCompiler->Compile(ast, out_byteCode, sOutputPath).Succeeded());
  }

  template <typename T>
  T Execute(const plExpressionByteCode& byteCode, T a = T(0), T b = T(0), T c = T(0), T d = T(0))
  {
    plProcessingStream inputs[] = {
      plProcessingStream(s_sA, plMakeArrayPtr(&a, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
      plProcessingStream(s_sB, plMakeArrayPtr(&b, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
      plProcessingStream(s_sC, plMakeArrayPtr(&c, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
      plProcessingStream(s_sD, plMakeArrayPtr(&d, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
    };

    T output = StreamDataTypeDeduction<T>::Default();
    plProcessingStream outputs[] = {
      plProcessingStream(s_sOutput, plMakeArrayPtr(&output, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
    };

    PLASMA_TEST_BOOL(s_pVM->Execute(byteCode, inputs, outputs, 1).Succeeded());

    return output;
  };

  template <typename T>
  T TestInstruction(plStringView sCode, T a = T(0), T b = T(0), T c = T(0), T d = T(0), bool bDumpASTs = false)
  {
    plExpressionByteCode byteCode;
    Compile<T>(sCode, byteCode, bDumpASTs ? "TestInstruction" : "");
    return Execute<T>(byteCode, a, b, c, d);
  }

  template <typename T>
  T TestConstant(plStringView sCode, bool bDumpASTs = false)
  {
    plExpressionByteCode byteCode;
    Compile<T>(sCode, byteCode, bDumpASTs ? "TestConstant" : "");
    PLASMA_TEST_INT(byteCode.GetNumInstructions(), 2); // MovX_C, StoreX
    PLASMA_TEST_INT(byteCode.GetNumTempRegisters(), 1);
    return Execute<T>(byteCode);
  }

  enum TestBinaryFlags
  {
    LeftConstantOptimization = PLASMA_BIT(0),
    NoInstructionsCountCheck = PLASMA_BIT(2),
  };

  template <typename R, typename T, plUInt32 flags>
  void TestBinaryInstruction(plStringView sOp, T a, T b, T expectedResult, bool bDumpASTs = false)
  {
    constexpr bool boolInputs = std::is_same<T, bool>::value;
    using U = typename std::conditional<boolInputs, int, T>::type;

    U aAsU;
    U bAsU;
    U expectedResultAsU;
    if constexpr (boolInputs)
    {
      aAsU = a ? 1 : 0;
      bAsU = b ? 1 : 0;
      expectedResultAsU = expectedResult ? 1 : 0;
    }
    else
    {
      aAsU = a;
      bAsU = b;
      expectedResultAsU = expectedResult;
    }

    auto TestRes = [](U res, U expectedRes, const char* szCode, const char* szAValue, const char* szBValue) {
      if constexpr (std::is_same<T, float>::value)
      {
        PLASMA_TEST_FLOAT_MSG(res, expectedRes, plMath::DefaultEpsilon<float>(), "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else if constexpr (std::is_same<T, int>::value)
      {
        PLASMA_TEST_INT_MSG(res, expectedRes, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else if constexpr (std::is_same<T, bool>::value)
      {
        const char* szRes = (res != 0) ? "true" : "false";
        const char* szExpectedRes = (expectedRes != 0) ? "true" : "false";
        PLASMA_TEST_STRING_MSG(szRes, szExpectedRes, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else if constexpr (std::is_same<T, plVec3>::value)
      {
        PLASMA_TEST_VEC3_MSG(res, expectedRes, plMath::DefaultEpsilon<float>(), "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else if constexpr (std::is_same<T, plVec3I32>::value)
      {
        PLASMA_TEST_INT_MSG(res.x, expectedRes.x, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
        PLASMA_TEST_INT_MSG(res.y, expectedRes.y, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
        PLASMA_TEST_INT_MSG(res.z, expectedRes.z, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else
      {
        PLASMA_ASSERT_NOT_IMPLEMENTED;
      }
    };

    const bool functionStyleSyntax = sOp.FindSubString("(");
    const char* formatString = functionStyleSyntax ? "output = {0}{1}, {2})" : "output = {1} {0} {2}";
    const char* aInput = boolInputs ? "(a != 0)" : "a";
    const char* bInput = boolInputs ? "(b != 0)" : "b";

    plStringBuilder aValue;
    plStringBuilder bValue;
    if constexpr (std::is_same<T, plVec3>::value || std::is_same<T, plVec3I32>::value)
    {
      aValue.Format("vec3({}, {}, {})", a.x, a.y, a.z);
      bValue.Format("vec3({}, {}, {})", b.x, b.y, b.z);
    }
    else
    {
      aValue.Format("{}", a);
      bValue.Format("{}", b);
    }

    int oneConstantInstructions = 3; // LoadX, OpX_RC, StoreX
    int oneConstantRegisters = 1;
    if constexpr (std::is_same<R, bool>::value)
    {
      oneConstantInstructions += 3; // + MovX_C, MovX_C, SelI_RRR
      oneConstantRegisters += 2;    // Two more registers needed for constants above
    }
    if constexpr (boolInputs)
    {
      oneConstantInstructions += 1; // + NotEqI_RC
    }

    int numOutputElements = 1;
    bool hasDifferentOutputElements = false;
    if constexpr (std::is_same<T, plVec3>::value || std::is_same<T, plVec3I32>::value)
    {
      numOutputElements = 3;

      for (int i = 1; i < 3; ++i)
      {
        if (expectedResult.GetData()[i] != expectedResult.GetData()[i - 1])
        {
          hasDifferentOutputElements = true;
          break;
        }
      }
    }

    plStringBuilder code;
    plExpressionByteCode byteCode;

    code.Format(formatString, sOp, aInput, bInput);
    Compile<U>(code, byteCode, bDumpASTs ? "BinaryNoConstants" : "");
    TestRes(Execute<U>(byteCode, aAsU, bAsU), expectedResultAsU, code, aValue, bValue);

    code.Format(formatString, sOp, aValue, bInput);
    Compile<U>(code, byteCode, bDumpASTs ? "BinaryLeftConstant" : "");
    if constexpr ((flags & NoInstructionsCountCheck) == 0)
    {
      int leftConstantInstructions = oneConstantInstructions;
      int leftConstantRegisters = oneConstantRegisters;
      if constexpr ((flags & LeftConstantOptimization) == 0)
      {
        leftConstantInstructions += 1;
        leftConstantRegisters += 1;
      }

      if (byteCode.GetNumInstructions() != leftConstantInstructions || byteCode.GetNumTempRegisters() != leftConstantRegisters)
      {
        DumpDisassembly(byteCode, "BinaryLeftConstant", 0);
        PLASMA_TEST_INT(byteCode.GetNumInstructions(), leftConstantInstructions);
        PLASMA_TEST_INT(byteCode.GetNumTempRegisters(), leftConstantRegisters);
      }
    }
    TestRes(Execute<U>(byteCode, aAsU, bAsU), expectedResultAsU, code, aValue, bValue);

    code.Format(formatString, sOp, aInput, bValue);
    Compile<U>(code, byteCode, bDumpASTs ? "BinaryRightConstant" : "");
    if constexpr ((flags & NoInstructionsCountCheck) == 0)
    {
      if (byteCode.GetNumInstructions() != oneConstantInstructions || byteCode.GetNumTempRegisters() != oneConstantRegisters)
      {
        DumpDisassembly(byteCode, "BinaryRightConstant", 0);
        PLASMA_TEST_INT(byteCode.GetNumInstructions(), oneConstantInstructions);
        PLASMA_TEST_INT(byteCode.GetNumTempRegisters(), oneConstantRegisters);
      }
    }
    TestRes(Execute<U>(byteCode, aAsU, bAsU), expectedResultAsU, code, aValue, bValue);

    code.Format(formatString, sOp, aValue, bValue);
    Compile<U>(code, byteCode, bDumpASTs ? "BinaryConstant" : "");
    if (hasDifferentOutputElements == false)
    {
      int bothConstantsInstructions = 1 + numOutputElements; // MovX_C + StoreX * numOutputElements
      int bothConstantsRegisters = 1;
      if (byteCode.GetNumInstructions() != bothConstantsInstructions || byteCode.GetNumTempRegisters() != bothConstantsRegisters)
      {
        DumpDisassembly(byteCode, "BinaryConstant", 0);
        PLASMA_TEST_INT(byteCode.GetNumInstructions(), bothConstantsInstructions);
        PLASMA_TEST_INT(byteCode.GetNumTempRegisters(), bothConstantsRegisters);
      }
    }
    TestRes(Execute<U>(byteCode), expectedResultAsU, code, aValue, bValue);
  }

  template <typename T>
  bool CompareCode(plStringView sTestCode, plStringView sReferenceCode, plExpressionByteCode& out_testByteCode, bool bDumpASTs = false)
  {
    Compile<T>(sTestCode, out_testByteCode, bDumpASTs ? "Test" : "");

    plExpressionByteCode referenceByteCode;
    Compile<T>(sReferenceCode, referenceByteCode, bDumpASTs ? "Reference" : "");

    return CompareByteCode(out_testByteCode, referenceByteCode);
  }

  template <typename T>
  void TestInputOutput()
  {
    plStringView testCode = "output = a + b * 2";
    plExpressionByteCode testByteCode;
    Compile<T>(testCode, testByteCode);

    constexpr plUInt32 uiCount = 17;
    plHybridArray<T, uiCount> a;
    plHybridArray<T, uiCount> b;
    plHybridArray<T, uiCount> o;
    plHybridArray<float, uiCount> expectedOutput;
    a.SetCountUninitialized(uiCount);
    b.SetCountUninitialized(uiCount);
    o.SetCount(uiCount);
    expectedOutput.SetCountUninitialized(uiCount);

    for (plUInt32 i = 0; i < uiCount; ++i)
    {
      a[i] = static_cast<T>(3.0f * i);
      b[i] = static_cast<T>(1.5f * i);
      expectedOutput[i] = a[i] + b[i] * 2.0f;
    }

    plProcessingStream inputs[] = {
      plProcessingStream(s_sA, a.GetByteArrayPtr(), StreamDataTypeDeduction<T>::Type),
      plProcessingStream(s_sB, b.GetByteArrayPtr(), StreamDataTypeDeduction<T>::Type),
    };

    plProcessingStream outputs[] = {
      plProcessingStream(s_sOutput, o.GetByteArrayPtr(), StreamDataTypeDeduction<T>::Type),
    };

    PLASMA_TEST_BOOL(s_pVM->Execute(testByteCode, inputs, outputs, uiCount).Succeeded());

    for (plUInt32 i = 0; i < uiCount; ++i)
    {
      PLASMA_TEST_FLOAT(static_cast<float>(o[i]), expectedOutput[i], plMath::DefaultEpsilon<float>());
    }
  }

  static const plEnum<plExpression::RegisterType> s_TestFunc1InputTypes[] = {plExpression::RegisterType::Float, plExpression::RegisterType::Int};
  static const plEnum<plExpression::RegisterType> s_TestFunc2InputTypes[] = {plExpression::RegisterType::Float, plExpression::RegisterType::Float, plExpression::RegisterType::Int};

  static void TestFunc1(plExpression::Inputs inputs, plExpression::Output output, const plExpression::GlobalData& globalData)
  {
    const plExpression::Register* pX = inputs[0].GetPtr();
    const plExpression::Register* pY = inputs[1].GetPtr();
    const plExpression::Register* pXEnd = inputs[0].GetEndPtr();
    plExpression::Register* pOutput = output.GetPtr();

    while (pX < pXEnd)
    {
      pOutput->f = pX->f.CompMul(pY->i.ToFloat());

      ++pX;
      ++pY;
      ++pOutput;
    }
  }

  static void TestFunc2(plExpression::Inputs inputs, plExpression::Output output, const plExpression::GlobalData& globalData)
  {
    const plExpression::Register* pX = inputs[0].GetPtr();
    const plExpression::Register* pY = inputs[1].GetPtr();
    const plExpression::Register* pXEnd = inputs[0].GetEndPtr();
    plExpression::Register* pOutput = output.GetPtr();

    if (inputs.GetCount() >= 3)
    {
      const plExpression::Register* pZ = inputs[2].GetPtr();

      while (pX < pXEnd)
      {
        pOutput->f = pX->f.CompMul(pY->f) * 2.0f + pZ->i.ToFloat();

        ++pX;
        ++pY;
        ++pZ;
        ++pOutput;
      }
    }
    else
    {
      while (pX < pXEnd)
      {
        pOutput->f = pX->f.CompMul(pY->f) * 2.0f;

        ++pX;
        ++pY;
        ++pOutput;
      }
    }
  }

  plExpressionFunction s_TestFunc1 = {
    {plMakeHashedString("TestFunc"), plMakeArrayPtr(s_TestFunc1InputTypes), 2, plExpression::RegisterType::Float},
    &TestFunc1,
  };

  plExpressionFunction s_TestFunc2 = {
    {plMakeHashedString("TestFunc"), plMakeArrayPtr(s_TestFunc2InputTypes), 3, plExpression::RegisterType::Float},
    &TestFunc2,
  };

} // namespace

PLASMA_CREATE_SIMPLE_TEST(CodeUtils, Expression)
{
  s_uiNumByteCodeComparisons = 0;

  plStringBuilder outputPath = plTestFramework::GetInstance()->GetAbsOutputPath();
  PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(outputPath.GetData(), "test", "output", plFileSystem::AllowWrites) == PLASMA_SUCCESS);

  s_pParser = PLASMA_DEFAULT_NEW(plExpressionParser);
  s_pCompiler = PLASMA_DEFAULT_NEW(plExpressionCompiler);
  s_pVM = PLASMA_DEFAULT_NEW(plExpressionVM);
  PLASMA_SCOPE_EXIT(s_pParser = nullptr; s_pCompiler = nullptr; s_pVM = nullptr;);

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Unary instructions")
  {
    // Negate
    PLASMA_TEST_INT(TestInstruction("output = -a", 2), -2);
    PLASMA_TEST_FLOAT(TestInstruction("output = -a", 2.5f), -2.5f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_INT(TestConstant<int>("output = -2"), -2);
    PLASMA_TEST_FLOAT(TestConstant<float>("output = -2.5"), -2.5f, plMath::DefaultEpsilon<float>());

    // Absolute
    PLASMA_TEST_INT(TestInstruction("output = abs(a)", -2), 2);
    PLASMA_TEST_FLOAT(TestInstruction("output = abs(a)", -2.5f), 2.5f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_INT(TestConstant<int>("output = abs(-2)"), 2);
    PLASMA_TEST_FLOAT(TestConstant<float>("output = abs(-2.5)"), 2.5f, plMath::DefaultEpsilon<float>());

    // Saturate
    PLASMA_TEST_INT(TestInstruction("output = saturate(a)", -1), 0);
    PLASMA_TEST_INT(TestInstruction("output = saturate(a)", 2), 1);
    PLASMA_TEST_FLOAT(TestInstruction("output = saturate(a)", -1.5f), 0.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = saturate(a)", 2.5f), 1.0f, plMath::DefaultEpsilon<float>());

    PLASMA_TEST_INT(TestConstant<int>("output = saturate(-1)"), 0);
    PLASMA_TEST_INT(TestConstant<int>("output = saturate(2)"), 1);
    PLASMA_TEST_FLOAT(TestConstant<float>("output = saturate(-1.5)"), 0.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = saturate(2.5)"), 1.0f, plMath::DefaultEpsilon<float>());

    // Sqrt
    PLASMA_TEST_FLOAT(TestInstruction("output = sqrt(a)", 25.0f), 5.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = sqrt(a)", 2.0f), plMath::Sqrt(2.0f), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = sqrt(25)"), 5.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = sqrt(2)"), plMath::Sqrt(2.0f), plMath::DefaultEpsilon<float>());

    // Exp
    PLASMA_TEST_FLOAT(TestInstruction("output = exp(a)", 0.0f), 1.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = exp(a)", 2.0f), plMath::Exp(2.0f), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = exp(0.0)"), 1.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = exp(2.0)"), plMath::Exp(2.0f), plMath::DefaultEpsilon<float>());

    // Ln
    PLASMA_TEST_FLOAT(TestInstruction("output = ln(a)", 1.0f), 0.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = ln(a)", 2.0f), plMath::Ln(2.0f), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = ln(1.0)"), 0.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = ln(2.0)"), plMath::Ln(2.0f), plMath::DefaultEpsilon<float>());

    // Log2
    PLASMA_TEST_INT(TestInstruction("output = log2(a)", 1), 0);
    PLASMA_TEST_INT(TestInstruction("output = log2(a)", 8), 3);
    PLASMA_TEST_FLOAT(TestInstruction("output = log2(a)", 1.0f), 0.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = log2(a)", 4.0f), 2.0f, plMath::DefaultEpsilon<float>());

    PLASMA_TEST_INT(TestConstant<int>("output = log2(1)"), 0);
    PLASMA_TEST_INT(TestConstant<int>("output = log2(16)"), 4);
    PLASMA_TEST_FLOAT(TestConstant<float>("output = log2(1.0)"), 0.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = log2(32.0)"), 5.0f, plMath::DefaultEpsilon<float>());

    // Log10
    PLASMA_TEST_FLOAT(TestInstruction("output = log10(a)", 10.0f), 1.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = log10(a)", 1000.0f), 3.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = log10(10.0)"), 1.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = log10(100.0)"), 2.0f, plMath::DefaultEpsilon<float>());

    // Pow2
    PLASMA_TEST_INT(TestInstruction("output = pow2(a)", 0), 1);
    PLASMA_TEST_INT(TestInstruction("output = pow2(a)", 3), 8);
    PLASMA_TEST_FLOAT(TestInstruction("output = pow2(a)", 4.0f), 16.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = pow2(a)", 6.0f), 64.0f, plMath::DefaultEpsilon<float>());

    PLASMA_TEST_INT(TestConstant<int>("output = pow2(0)"), 1);
    PLASMA_TEST_INT(TestConstant<int>("output = pow2(3)"), 8);
    PLASMA_TEST_FLOAT(TestConstant<float>("output = pow2(3.0)"), 8.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = pow2(5.0)"), 32.0f, plMath::DefaultEpsilon<float>());

    // Sin
    PLASMA_TEST_FLOAT(TestInstruction("output = sin(a)", plAngle::Degree(90.0f).GetRadian()), 1.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = sin(a)", plAngle::Degree(45.0f).GetRadian()), plMath::Sin(plAngle::Degree(45.0f)), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = sin(PI / 2)"), 1.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = sin(PI / 4)"), plMath::Sin(plAngle::Degree(45.0f)), plMath::DefaultEpsilon<float>());

    // Cos
    PLASMA_TEST_FLOAT(TestInstruction("output = cos(a)", 0.0f), 1.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = cos(a)", plAngle::Degree(45.0f).GetRadian()), plMath::Cos(plAngle::Degree(45.0f)), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = cos(0)"), 1.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = cos(PI / 4)"), plMath::Cos(plAngle::Degree(45.0f)), plMath::DefaultEpsilon<float>());

    // Tan
    PLASMA_TEST_FLOAT(TestInstruction("output = tan(a)", 0.0f), 0.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = tan(a)", plAngle::Degree(45.0f).GetRadian()), 1.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = tan(0)"), 0.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = tan(PI / 4)"), 1.0f, plMath::DefaultEpsilon<float>());

    // ASin
    PLASMA_TEST_FLOAT(TestInstruction("output = asin(a)", 1.0f), plAngle::Degree(90.0f).GetRadian(), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = asin(a)", plMath::Sin(plAngle::Degree(45.0f))), plAngle::Degree(45.0f).GetRadian(), plMath::LargeEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = asin(1)"), plAngle::Degree(90.0f).GetRadian(), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = asin(sin(PI / 4))"), plAngle::Degree(45.0f).GetRadian(), plMath::LargeEpsilon<float>());

    // ACos
    PLASMA_TEST_FLOAT(TestInstruction("output = acos(a)", 1.0f), 0.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = acos(a)", plMath::Cos(plAngle::Degree(45.0f))), plAngle::Degree(45.0f).GetRadian(), plMath::LargeEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = acos(1)"), 0.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = acos(cos(PI / 4))"), plAngle::Degree(45.0f).GetRadian(), plMath::LargeEpsilon<float>());

    // ATan
    PLASMA_TEST_FLOAT(TestInstruction("output = atan(a)", 0.0f), 0.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = atan(a)", 1.0f), plAngle::Degree(45.0f).GetRadian(), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = atan(0)"), 0.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = atan(1)"), plAngle::Degree(45.0f).GetRadian(), plMath::DefaultEpsilon<float>());

    // RadToDeg
    PLASMA_TEST_FLOAT(TestInstruction("output = radToDeg(a)", plAngle::Degree(135.0f).GetRadian()), 135.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = rad_to_deg(a)", plAngle::Degree(180.0f).GetRadian()), 180.0f, plMath::LargeEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = radToDeg(PI / 2)"), 90.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = rad_to_deg(PI/4)"), 45.0f, plMath::DefaultEpsilon<float>());

    // DegToRad
    PLASMA_TEST_FLOAT(TestInstruction("output = degToRad(a)", 135.0f), plAngle::Degree(135.0f).GetRadian(), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = deg_to_rad(a)", 180.0f), plAngle::Degree(180.0f).GetRadian(), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = degToRad(90.0)"), plAngle::Degree(90.0f).GetRadian(), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = deg_to_rad(45)"), plAngle::Degree(45.0f).GetRadian(), plMath::DefaultEpsilon<float>());

    // Round
    PLASMA_TEST_FLOAT(TestInstruction("output = round(a)", 12.34f), 12, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = round(a)", -12.34f), -12, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = round(a)", 12.54f), 13, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = round(a)", -12.54f), -13, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = round(4.3)"), 4, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = round(4.51)"), 5, plMath::DefaultEpsilon<float>());

    // Floor
    PLASMA_TEST_FLOAT(TestInstruction("output = floor(a)", 12.34f), 12, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = floor(a)", -12.34f), -13, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = floor(a)", 12.54f), 12, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = floor(a)", -12.54f), -13, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = floor(4.3)"), 4, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = floor(4.51)"), 4, plMath::DefaultEpsilon<float>());

    // Ceil
    PLASMA_TEST_FLOAT(TestInstruction("output = ceil(a)", 12.34f), 13, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = ceil(a)", -12.34f), -12, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = ceil(a)", 12.54f), 13, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = ceil(a)", -12.54f), -12, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = ceil(4.3)"), 5, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = ceil(4.51)"), 5, plMath::DefaultEpsilon<float>());

    // Trunc
    PLASMA_TEST_FLOAT(TestInstruction("output = trunc(a)", 12.34f), 12, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = trunc(a)", -12.34f), -12, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = trunc(a)", 12.54f), 12, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = trunc(a)", -12.54f), -12, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = trunc(4.3)"), 4, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = trunc(4.51)"), 4, plMath::DefaultEpsilon<float>());

    // Frac
    PLASMA_TEST_FLOAT(TestInstruction("output = frac(a)", 12.34f), 0.34f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = frac(a)", -12.34f), -0.34f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = frac(a)", 12.54f), 0.54f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = frac(a)", -12.54f), -0.54f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = frac(4.3)"), 0.3f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = frac(4.51)"), 0.51f, plMath::DefaultEpsilon<float>());

    // Length
    PLASMA_TEST_VEC3(TestInstruction<plVec3>("output = length(a)", plVec3(0, 4, 3)), plVec3(5), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(TestInstruction<plVec3>("output = length(a)", plVec3(-3, 4, 0)), plVec3(5), plMath::DefaultEpsilon<float>());

    // Normalize
    PLASMA_TEST_VEC3(TestInstruction<plVec3>("output = normalize(a)", plVec3(1, 4, 3)), plVec3(1, 4, 3).GetNormalized(), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(TestInstruction<plVec3>("output = normalize(a)", plVec3(-3, 7, 22)), plVec3(-3, 7, 22).GetNormalized(), plMath::DefaultEpsilon<float>());

    // Length and normalize optimization
    {
      plStringView testCode = "var x = length(a); var na = normalize(a); output = b * x + na";
      plStringView referenceCode = "var x = length(a); var na = a / x; output = b * x + na";

      plExpressionByteCode testByteCode;
      PLASMA_TEST_BOOL(CompareCode<plVec3>(testCode, referenceCode, testByteCode));

      plVec3 a = plVec3(0, 4, 3);
      plVec3 b = plVec3(1, 0, 0);
      plVec3 res = b * a.GetLength() + a.GetNormalized();
      PLASMA_TEST_VEC3(Execute(testByteCode, a, b), res, plMath::DefaultEpsilon<float>());
    }

    // BitwiseNot
    PLASMA_TEST_INT(TestInstruction("output = ~a", 1), ~1);
    PLASMA_TEST_INT(TestInstruction("output = ~a", 8), ~8);
    PLASMA_TEST_INT(TestConstant<int>("output = ~1"), ~1);
    PLASMA_TEST_INT(TestConstant<int>("output = ~17"), ~17);

    // LogicalNot
    PLASMA_TEST_INT(TestInstruction("output = !(a == 1)", 1), 0);
    PLASMA_TEST_INT(TestInstruction("output = !(a == 1)", 8), 1);
    PLASMA_TEST_INT(TestConstant<int>("output = !(1 == 1)"), 0);
    PLASMA_TEST_INT(TestConstant<int>("output = !(8 == 1)"), 1);

    // All
    PLASMA_TEST_VEC3(TestInstruction("var t = (a == b); output = all(t)", plVec3(1, 2, 3), plVec3(1, 2, 3)), plVec3(1), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(TestInstruction("var t = (a == b); output = all(t)", plVec3(1, 2, 3), plVec3(1, 2, 4)), plVec3(0), plMath::DefaultEpsilon<float>());

    // Any
    PLASMA_TEST_VEC3(TestInstruction("var t = (a == b); output = any(t)", plVec3(1, 2, 3), plVec3(4, 5, 3)), plVec3(1), plMath::DefaultEpsilon<float>());
    PLASMA_TEST_VEC3(TestInstruction("var t = (a == b); output = any(t)", plVec3(1, 2, 3), plVec3(4, 5, 6)), plVec3(0), plMath::DefaultEpsilon<float>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Binary instructions")
  {
    // Add
    TestBinaryInstruction<int, int, LeftConstantOptimization>("+", 3, 5, 8);
    TestBinaryInstruction<float, float, LeftConstantOptimization>("+", 3.5f, 5.3f, 8.8f);

    // Subtract
    TestBinaryInstruction<int, int, 0>("-", 9, 5, 4);
    TestBinaryInstruction<float, float, 0>("-", 9.5f, 5.3f, 4.2f);

    // Multiply
    TestBinaryInstruction<int, int, LeftConstantOptimization>("*", 3, 5, 15);
    TestBinaryInstruction<float, float, LeftConstantOptimization>("*", 3.5f, 5.3f, 18.55f);

    // Divide
    TestBinaryInstruction<int, int, 0>("/", 11, 5, 2);
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("/", -11, 4, -2); // divide by power of 2 optimization
    TestBinaryInstruction<int, int, 0>("/", 11, -4, -2);                        // divide by power of 2 optimization only works for positive divisors
    TestBinaryInstruction<float, float, 0>("/", 12.6f, 3.0f, 4.2f);

    // Modulo
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("%", 13, 5, 3);
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("%", -13, 5, -3);
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("%", 13, 4, 1);
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("%", -13, 4, -1);
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("%", 13.5, 5.0, 3.5);
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("mod(", -13.5, 5.0, -3.5);

    // Log
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("log(", 2.0f, 1024.0f, 10.0f);
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("log(", 7.1f, 81.62f, plMath::Log(7.1f, 81.62f));

    // Pow
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("pow(", 2, 5, 32);
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("pow(", 3, 3, 27);

    // Pow is replaced by multiplication for constant exponents up until 16.
    // Test all of them to ensure the multiplication tables are correct.
    for (int i = 0; i <= 16; ++i)
    {
      plStringBuilder testCode;
      testCode.Format("output = pow(a, {})", i);

      plExpressionByteCode testByteCode;
      Compile<int>(testCode, testByteCode);
      PLASMA_TEST_INT(Execute(testByteCode, 3), plMath::Pow(3, i));
    }

    {
      plStringView testCode = "output = pow(a, 7)";
      plStringView referenceCode = "var a2 = a * a; var a3 = a2 * a; var a6 = a3 * a3; output = a6 * a";

      plExpressionByteCode testByteCode;
      PLASMA_TEST_BOOL(CompareCode<int>(testCode, referenceCode, testByteCode));
      PLASMA_TEST_INT(Execute(testByteCode, 3), 2187);
    }

    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("pow(", 2.0, 5.0, 32.0);
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("pow(", 3.0f, 7.9f, plMath::Pow(3.0f, 7.9f));

    {
      plStringView testCode = "output = pow(a, 15.0)";
      plStringView referenceCode = "var a2 = a * a; var a3 = a2 * a; var a6 = a3 * a3; var a12 = a6 * a6; output = a12 * a3";

      plExpressionByteCode testByteCode;
      PLASMA_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));
      PLASMA_TEST_FLOAT(Execute(testByteCode, 2.1f), plMath::Pow(2.1f, 15.0f), plMath::DefaultEpsilon<float>());
    }

    // Min
    TestBinaryInstruction<int, int, LeftConstantOptimization>("min(", 11, 5, 5);
    TestBinaryInstruction<float, float, LeftConstantOptimization>("min(", 12.6f, 3.0f, 3.0f);

    // Max
    TestBinaryInstruction<int, int, LeftConstantOptimization>("max(", 11, 5, 11);
    TestBinaryInstruction<float, float, LeftConstantOptimization>("max(", 12.6f, 3.0f, 12.6f);

    // Dot
    TestBinaryInstruction<plVec3, plVec3, NoInstructionsCountCheck>("dot(", plVec3(1, -2, 3), plVec3(-5, -6, 7), plVec3(28));
    TestBinaryInstruction<plVec3I32, plVec3I32, NoInstructionsCountCheck>("dot(", plVec3I32(1, -2, 3), plVec3I32(-5, -6, 7), plVec3I32(28));

    // Cross
    TestBinaryInstruction<plVec3, plVec3, NoInstructionsCountCheck>("cross(", plVec3(1, 0, 0), plVec3(0, 1, 0), plVec3(0, 0, 1));
    TestBinaryInstruction<plVec3, plVec3, NoInstructionsCountCheck>("cross(", plVec3(0, 1, 0), plVec3(0, 0, 1), plVec3(1, 0, 0));
    TestBinaryInstruction<plVec3, plVec3, NoInstructionsCountCheck>("cross(", plVec3(0, 0, 1), plVec3(1, 0, 0), plVec3(0, 1, 0));

    // Reflect
    TestBinaryInstruction<plVec3, plVec3, NoInstructionsCountCheck>("reflect(", plVec3(1, 2, -1), plVec3(0, 0, 1), plVec3(1, 2, 1));

    // BitshiftLeft
    TestBinaryInstruction<int, int, 0>("<<", 11, 5, 11 << 5);

    // BitshiftRight
    TestBinaryInstruction<int, int, 0>(">>", 0xABCD, 8, 0xAB);

    // BitwiseAnd
    TestBinaryInstruction<int, int, LeftConstantOptimization>("&", 0xFFCD, 0xABFF, 0xABCD);

    // BitwiseXor
    TestBinaryInstruction<int, int, LeftConstantOptimization>("^", 0xFFCD, 0xABFF, 0xFFCD ^ 0xABFF);

    // BitwiseOr
    TestBinaryInstruction<int, int, LeftConstantOptimization>("|", 0x00CD, 0xAB00, 0xABCD);

    // Equal
    TestBinaryInstruction<bool, int, LeftConstantOptimization>("==", 11, 5, 0);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("==", 12.6f, 3.0f, 0.0f);
    TestBinaryInstruction<bool, bool, LeftConstantOptimization>("==", true, false, false);

    // NotEqual
    TestBinaryInstruction<bool, int, LeftConstantOptimization>("!=", 11, 5, 1);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("!=", 12.6f, 3.0f, 1.0f);
    TestBinaryInstruction<bool, bool, LeftConstantOptimization>("!=", true, false, true);

    // Less
    TestBinaryInstruction<bool, int, LeftConstantOptimization>("<", 11, 5, 0);
    TestBinaryInstruction<bool, int, LeftConstantOptimization>("<", 11, 11, 0);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("<", 12.6f, 3.0f, 0.0f);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("<", 12.6f, 12.6f, 0.0f);

    // LessEqual
    TestBinaryInstruction<bool, int, LeftConstantOptimization>("<=", 11, 5, 0);
    TestBinaryInstruction<bool, int, LeftConstantOptimization>("<=", 11, 11, 1);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("<=", 12.6f, 3.0f, 0.0f);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("<=", 12.6f, 12.6f, 1.0f);

    // Greater
    TestBinaryInstruction<bool, int, LeftConstantOptimization>(">", 11, 5, 1);
    TestBinaryInstruction<bool, int, LeftConstantOptimization>(">", 11, 11, 0);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>(">", 12.6f, 3.0f, 1.0f);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>(">", 12.6f, 12.6f, 0.0f);

    // GreaterEqual
    TestBinaryInstruction<bool, int, LeftConstantOptimization>(">=", 11, 5, 1);
    TestBinaryInstruction<bool, int, LeftConstantOptimization>(">=", 11, 11, 1);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>(">=", 12.6f, 3.0f, 1.0f);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>(">=", 12.6f, 12.6f, 1.0f);

    // LogicalAnd
    TestBinaryInstruction<bool, bool, LeftConstantOptimization | NoInstructionsCountCheck>("&&", true, false, false);
    TestBinaryInstruction<bool, bool, LeftConstantOptimization>("&&", true, true, true);

    // LogicalOr
    TestBinaryInstruction<bool, bool, LeftConstantOptimization | NoInstructionsCountCheck>("||", true, false, true);
    TestBinaryInstruction<bool, bool, LeftConstantOptimization>("||", false, false, false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Ternary instructions")
  {
    // Clamp
    PLASMA_TEST_INT(TestInstruction("output = clamp(a, b, c)", -1, 0, 10), 0);
    PLASMA_TEST_INT(TestInstruction("output = clamp(a, b, c)", 2, 0, 10), 2);
    PLASMA_TEST_FLOAT(TestInstruction("output = clamp(a, b, c)", -1.5f, 0.0f, 1.0f), 0.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = clamp(a, b, c)", 2.5f, 0.0f, 1.0f), 1.0f, plMath::DefaultEpsilon<float>());

    PLASMA_TEST_INT(TestConstant<int>("output = clamp(-1, 0, 10)"), 0);
    PLASMA_TEST_INT(TestConstant<int>("output = clamp(2, 0, 10)"), 2);
    PLASMA_TEST_FLOAT(TestConstant<float>("output = clamp(-1.5, 0, 2)"), 0.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = clamp(2.5, 0, 2)"), 2.0f, plMath::DefaultEpsilon<float>());

    // Select
    PLASMA_TEST_INT(TestInstruction("output = (a == 1) ? b : c", 1, 2, 3), 2);
    PLASMA_TEST_INT(TestInstruction("output = a != 1 ? b : c", 1, 2, 3), 3);
    PLASMA_TEST_FLOAT(TestInstruction("output = (a == 1) ? b : c", 1.0f, 2.4f, 3.5f), 2.4f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = a != 1 ? b : c", 1.0f, 2.4f, 3.5f), 3.5f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_INT(TestInstruction("output = (a == 1) ? (b > 2) : (c > 2)", 1, 2, 3), 0);
    PLASMA_TEST_INT(TestInstruction("output = a != 1 ? b > 2 : c > 2", 1, 2, 3), 1);

    PLASMA_TEST_INT(TestConstant<int>("output = (1 == 1) ? 2 : 3"), 2);
    PLASMA_TEST_INT(TestConstant<int>("output = 1 != 1 ? 2 : 3"), 3);
    PLASMA_TEST_FLOAT(TestConstant<float>("output = (1.0 == 1.0) ? 2.4 : 3.5"), 2.4f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = 1.0 != 1.0 ? 2.4 : 3.5"), 3.5f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_INT(TestConstant<int>("output = (1 == 1) ? false : true"), 0);
    PLASMA_TEST_INT(TestConstant<int>("output = 1 != 1 ? false : true"), 1);

    // Lerp
    PLASMA_TEST_FLOAT(TestInstruction("output = lerp(a, b, c)", 1.0f, 5.0f, 0.75f), 4.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestInstruction("output = lerp(a, b, c)", -1.0f, -11.0f, 0.1f), -2.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = lerp(1, 5, 0.75)"), 4.0f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(TestConstant<float>("output = lerp(-1, -11, 0.1)"), -2.0f, plMath::DefaultEpsilon<float>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Local variables")
  {
    plExpressionByteCode referenceByteCode;
    {
      plStringView code = "output = (a + b) * 2";
      Compile<float>(code, referenceByteCode);
    }

    plExpressionByteCode testByteCode;

    plStringView code = "var e = a + b; output = e * 2";
    Compile<float>(code, testByteCode);
    PLASMA_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    code = "var e = a + b; e = e * 2; output = e";
    Compile<float>(code, testByteCode);
    PLASMA_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    code = "var e = a + b; e *= 2; output = e";
    Compile<float>(code, testByteCode);
    PLASMA_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    code = "var e = a + b; var f = e; e = 2; output = f * e";
    Compile<float>(code, testByteCode);
    PLASMA_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    PLASMA_TEST_FLOAT(Execute(testByteCode, 2.0f, 3.0f), 10.0f, plMath::DefaultEpsilon<float>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Assignment")
  {
    {
      plStringView testCode = "output = 40; output += 2";
      plStringView referenceCode = "output = 42";

      plExpressionByteCode testByteCode;
      PLASMA_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));

      PLASMA_TEST_FLOAT(Execute<float>(testByteCode), 42.0f, plMath::DefaultEpsilon<float>());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Integer arithmetic")
  {
    plExpressionByteCode testByteCode;

    plStringView code = "output = ((a & 0xFF) << 8) | (b & 0xFFFF >> 8)";
    Compile<int>(code, testByteCode);

    const int a = 0xABABABAB;
    const int b = 0xCDCDCDCD;
    PLASMA_TEST_INT(Execute(testByteCode, a, b), 0xABCD);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constant folding")
  {
    plStringView testCode = "var x = abs(-7) + saturate(2) + 2\n"
                            "var v = (sqrt(25) - 4) * 5\n"
                            "var m = min(300, 1000) / max(1, 3);"
                            "var r = m - x * 5 - v - clamp(13, 1, 3);\n"
                            "output = r";

    plStringView referenceCode = "output = 42";

    {
      plExpressionByteCode testByteCode;
      PLASMA_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));

      PLASMA_TEST_FLOAT(Execute<float>(testByteCode), 42.0f, plMath::DefaultEpsilon<float>());
    }

    {
      plExpressionByteCode testByteCode;
      PLASMA_TEST_BOOL(CompareCode<int>(testCode, referenceCode, testByteCode));

      PLASMA_TEST_INT(Execute<int>(testByteCode), 42);
    }

    testCode = "";
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constant instructions")
  {
    // There are special instructions in the vm which take the constant as the first operand in place and
    // don't require an extra mov for the constant.
    // This test checks whether the compiler transforms operations with constants as second operands to the preferred form.

    plStringView testCode = "output = (2 + a) + (-1 + b) + (2 * c) + (d / 5) + min(1, c) + max(2, d)";

    {
      plStringView referenceCode = "output = (a + 2) + (b + -1) + (c * 2) + (d * 0.2) + min(c, 1) + max(d, 2)";

      plExpressionByteCode testByteCode;
      PLASMA_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));
      PLASMA_TEST_INT(testByteCode.GetNumInstructions(), 16);
      PLASMA_TEST_INT(testByteCode.GetNumTempRegisters(), 4);
      PLASMA_TEST_FLOAT(Execute(testByteCode, 1.0f, 2.0f, 3.0f, 40.f), 59.0f, plMath::DefaultEpsilon<float>());
    }

    {
      plStringView referenceCode = "output = (a + 2) + (b + -1) + (c * 2) + (d / 5) + min(c, 1) + max(d, 2)";

      plExpressionByteCode testByteCode;
      PLASMA_TEST_BOOL(CompareCode<int>(testCode, referenceCode, testByteCode));
      PLASMA_TEST_INT(testByteCode.GetNumInstructions(), 16);
      PLASMA_TEST_INT(testByteCode.GetNumTempRegisters(), 4);
      PLASMA_TEST_INT(Execute(testByteCode, 1, 2, 3, 40), 59);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Integer and float conversions")
  {
    plStringView testCode = "var x = 7; var y = 0.6\n"
                            "var e = a * x * b * y\n"
                            "int i = c * 2; i *= i; e += i\n"
                            "output = e";

    plStringView referenceCode = "int i = (int(c) * 2); output = int((float(a * 7 * b) * 0.6) + float(i * i))";

    plExpressionByteCode testByteCode;
    PLASMA_TEST_BOOL(CompareCode<int>(testCode, referenceCode, testByteCode));
    PLASMA_TEST_INT(Execute(testByteCode, 1, 2, 3), 44);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Bool conversions")
  {
    plStringView testCode = "var x = true\n"
                            "bool y = a\n"
                            "output = x == y";

    {
      plStringView referenceCode = "bool r = true == (a != 0); output = r ? 1 : 0";

      plExpressionByteCode testByteCode;
      PLASMA_TEST_BOOL(CompareCode<int>(testCode, referenceCode, testByteCode));
      PLASMA_TEST_INT(Execute(testByteCode, 14), 1);
    }

    {
      plStringView referenceCode = "bool r = true == (a != 0); output = r ? 1.0 : 0.0";

      plExpressionByteCode testByteCode;
      PLASMA_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));
      PLASMA_TEST_FLOAT(Execute(testByteCode, 15.0f), 1.0f, plMath::DefaultEpsilon<float>());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Load Inputs/Store Outputs")
  {
    TestInputOutput<float>();
    TestInputOutput<plFloat16>();

    TestInputOutput<int>();
    TestInputOutput<plInt16>();
    TestInputOutput<plInt8>();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Function overloads")
  {
    s_pParser->RegisterFunction(s_TestFunc1.m_Desc);
    s_pParser->RegisterFunction(s_TestFunc2.m_Desc);

    s_pVM->RegisterFunction(s_TestFunc1);
    s_pVM->RegisterFunction(s_TestFunc2);

    {
      // take TestFunc1 overload for all ints
      plStringView testCode = "output = TestFunc(1, 2, 3)";
      plExpressionByteCode testByteCode;
      Compile<int>(testCode, testByteCode);
      PLASMA_TEST_INT(Execute<int>(testByteCode), 2);
    }

    {
      // take TestFunc1 overload for float, int
      plStringView testCode = "output = TestFunc(1.0, 2, 3)";
      plExpressionByteCode testByteCode;
      Compile<int>(testCode, testByteCode);
      PLASMA_TEST_INT(Execute<int>(testByteCode), 2);
    }

    {
      // take TestFunc2 overload for int, float
      plStringView testCode = "output = TestFunc(1, 2.0, 3)";
      plExpressionByteCode testByteCode;
      Compile<int>(testCode, testByteCode);
      PLASMA_TEST_INT(Execute<int>(testByteCode), 7);
    }

    {
      // take TestFunc2 overload for all float
      plStringView testCode = "output = TestFunc(1.0, 2.0, 3)";
      plExpressionByteCode testByteCode;
      Compile<int>(testCode, testByteCode);
      PLASMA_TEST_INT(Execute<int>(testByteCode), 7);
    }

    {
      // take TestFunc1 overload when only two params are given
      plStringView testCode = "output = TestFunc(1.0, 2.0)";
      plExpressionByteCode testByteCode;
      Compile<int>(testCode, testByteCode);
      PLASMA_TEST_INT(Execute<int>(testByteCode), 2);
    }

    s_pParser->UnregisterFunction(s_TestFunc1.m_Desc);
    s_pParser->UnregisterFunction(s_TestFunc2.m_Desc);

    s_pVM->UnregisterFunction(s_TestFunc1);
    s_pVM->UnregisterFunction(s_TestFunc2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Common subexpression elimination")
  {
    plStringView testCode = "var x1 = a * max(b, c)\n"
                            "var x2 = max(c, b) * a\n"
                            "var y1 = a * pow(2, 3)\n"
                            "var y2 = 8 * a\n"
                            "output = x1 + x2 + y1 + y2";

    plStringView referenceCode = "var x = a * max(b, c); var y = a * 8; output = x + x + y + y";

    plExpressionByteCode testByteCode;
    PLASMA_TEST_BOOL(CompareCode<int>(testCode, referenceCode, testByteCode));
    PLASMA_TEST_INT(Execute(testByteCode, 2, 4, 8), 64);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Vector constructors")
  {
    {
      plStringView testCode = "var x = vec3(1, 2, 3)\n"
                              "var y = vec4(x, 4)\n"
                              "vec3 z = vec2(1, 2)\n"
                              "var w = vec4()\n"
                              "output = vec4(x) + y + vec4(z) + w";

      plExpressionByteCode testByteCode;
      Compile<plVec3>(testCode, testByteCode);
      PLASMA_TEST_VEC3(Execute<plVec3>(testByteCode), plVec3(3, 6, 6), plMath::DefaultEpsilon<float>());
    }

    {
      plStringView testCode = "var x = vec4(a.xy, (vec2(6, 8) - vec2(3, 4)).xy)\n"
                              "var y = vec4(1, vec2(2, 3), 4)\n"
                              "var z = vec4(1, vec3(2, 3, 4))\n"
                              "var w = vec4(1, 2, a.zw)\n"
                              "var one = vec4(1)\n"
                              "output = vec4(x) + y + vec4(z) + w + one";

      plExpressionByteCode testByteCode;
      Compile<plVec3>(testCode, testByteCode);
      PLASMA_TEST_VEC3(Execute(testByteCode, plVec3(1, 2, 3)), plVec3(5, 9, 13), plMath::DefaultEpsilon<float>());
    }

    {
      plStringView testCode = "var x = vec4(1, 2, 3, 4)\n"
                              "var y = x.z\n"
                              "x.yz = 7\n"
                              "x.xz = vec2(2, 7)\n"
                              "output = x * y";

      plExpressionByteCode testByteCode;
      Compile<plVec3>(testCode, testByteCode);
      PLASMA_TEST_VEC3(Execute<plVec3>(testByteCode), plVec3(6, 21, 21), plMath::DefaultEpsilon<float>());
    }

    {
      plStringView testCode = "var x = 1\n"
                              "x.z = 7.5\n"
                              "output = x";

      plExpressionByteCode testByteCode;
      Compile<plVec3>(testCode, testByteCode);
      PLASMA_TEST_VEC3(Execute<plVec3>(testByteCode), plVec3(1, 0, 7), plMath::DefaultEpsilon<float>());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Vector instructions")
  {
    // The VM does only support scalar data types.
    // This test checks whether the compiler transforms everything correctly to scalar operation.

    plStringView testCode = "output = a * vec3(1, 2, 3) + sqrt(b)";

    plStringView referenceCode = "output.x = a.x + sqrt(b.x)\n"
                                 "output.y = a.y * 2 + sqrt(b.y)\n"
                                 "output.z = a.z * 3 + sqrt(b.z)";

    plExpressionByteCode testByteCode;
    PLASMA_TEST_BOOL(CompareCode<plVec3>(testCode, referenceCode, testByteCode));
    PLASMA_TEST_VEC3(Execute(testByteCode, plVec3(1, 3, 5), plVec3(4, 9, 16)), plVec3(3, 9, 19), plMath::DefaultEpsilon<float>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Vector swizzle")
  {
    plStringView testCode = "var n = vec4(1, 2, 3, 4)\n"
                            "var m = vec4(5, 6, 7, 8)\n"
                            "var p = n.xxyy + m.zzww * m.abgr + n.w\n"
                            "output = p";

    // vec3(1, 1, 2) + vec3(7, 7, 8) * vec3(8, 7, 6) + 4
    // output.x = 1 + 7 * 8 + 4 = 61
    // output.y = 1 + 7 * 7 + 4 = 54
    // output.z = 2 + 8 * 6 + 4 = 54

    plExpressionByteCode testByteCode;
    Compile<plVec3>(testCode, testByteCode);
    PLASMA_TEST_VEC3(Execute<plVec3>(testByteCode), plVec3(61, 54, 54), plMath::DefaultEpsilon<float>());
  }
}
