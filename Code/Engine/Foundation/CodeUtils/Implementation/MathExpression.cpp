#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/CodeUtils/MathExpression.h>

static plHashedString s_sOutput = plMakeHashedString("output");

plMathExpression::plMathExpression() = default;

plMathExpression::plMathExpression(plStringView sExpressionString)
{
  Reset(sExpressionString);
}

void plMathExpression::Reset(plStringView sExpressionString)
{
  m_sOriginalExpression.Assign(sExpressionString);
  m_ByteCode.Clear();
  m_bIsValid = false;

  if (sExpressionString.IsEmpty())
    return;

  plStringBuilder tmp = s_sOutput.GetView();
  tmp.Append(" = ", sExpressionString);

  plExpression::StreamDesc outputs[] = {
    {s_sOutput, plProcessingStream::DataType::Float},
  };

  plExpressionParser parser;
  plExpressionParser::Options parserOptions;
  parserOptions.m_bTreatUnknownVariablesAsInputs = true;

  plExpressionAST ast;
  if (parser.Parse(tmp, plArrayPtr<plExpression::StreamDesc>(), outputs, parserOptions, ast).Failed())
    return;

  plExpressionCompiler compiler;
  if (compiler.Compile(ast, m_ByteCode).Failed())
    return;

  m_bIsValid = true;
}

float plMathExpression::Evaluate(plArrayPtr<Input> inputs)
{
  float fOutput = plMath::NaN<float>();

  if (!IsValid() || m_ByteCode.IsEmpty())
  {
    plLog::Error("Can't evaluate invalid math expression '{0}'", m_sOriginalExpression);
    return fOutput;
  }

  plHybridArray<plProcessingStream, 8> inputStreams;
  for (auto& input : inputs)
  {
    if (input.m_sName.IsEmpty())
      continue;

    inputStreams.PushBack(plProcessingStream(input.m_sName, plMakeArrayPtr(&input.m_fValue, 1).ToByteArray(), plProcessingStream::DataType::Float));
  }

  plProcessingStream outputStream(s_sOutput, plMakeArrayPtr(&fOutput, 1).ToByteArray(), plProcessingStream::DataType::Float);
  plArrayPtr<plProcessingStream> outputStreams = plMakeArrayPtr(&outputStream, 1);

  if (m_VM.Execute(m_ByteCode, inputStreams, outputStreams, 1).Failed())
  {
    plLog::Error("Failed to execute expression VM");
  }

  return fOutput;
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Implementation_MathExpression);
