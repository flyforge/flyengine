#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>

using namespace plTokenParseUtils;

plResult plPreprocessor::CopyTokensAndEvaluateDefined(const TokenStream& Source, plUInt32 uiFirstSourceToken, TokenStream& Destination)
{
  Destination.Clear();
  Destination.Reserve(Source.GetCount() - uiFirstSourceToken);

  {
    // skip all whitespace at the start of the replacement string
    plUInt32 uiCurToken = uiFirstSourceToken;
    SkipWhitespace(Source, uiCurToken);

    // add all the relevant tokens to the definition
    while (uiCurToken < Source.GetCount())
    {
      if (Source[uiCurToken]->m_iType == plTokenType::BlockComment || Source[uiCurToken]->m_iType == plTokenType::LineComment || Source[uiCurToken]->m_iType == plTokenType::EndOfFile || Source[uiCurToken]->m_iType == plTokenType::Newline)
      {
        ++uiCurToken;
        continue;
      }

      if (Source[uiCurToken]->m_DataView.IsEqual("defined"))
      {
        ++uiCurToken;

        const bool bParenthesis = Accept(Source, uiCurToken, "(");

        plUInt32 uiIdentifier = uiCurToken;
        if (Expect(Source, uiCurToken, plTokenType::Identifier, &uiIdentifier).Failed())
          return PLASMA_FAILURE;

        plToken* pReplacement = nullptr;

        const bool bDefined = m_Macros.Find(Source[uiIdentifier]->m_DataView).IsValid();

        // broadcast that 'defined' is being evaluated
        {
          ProcessingEvent pe;
          pe.m_pToken = Source[uiIdentifier];
          pe.m_Type = ProcessingEvent::CheckDefined;
          pe.m_sInfo = bDefined ? "defined" : "undefined";
          m_ProcessingEvents.Broadcast(pe);
        }

        pReplacement = AddCustomToken(Source[uiIdentifier], bDefined ? "1" : "0");

        Destination.PushBack(pReplacement);

        if (bParenthesis)
        {
          if (Expect(Source, uiCurToken, ")").Failed())
            return PLASMA_FAILURE;
        }
      }
      else
      {
        Destination.PushBack(Source[uiCurToken]);
        ++uiCurToken;
      }
    }
  }

  // remove whitespace at end of macro
  while (!Destination.IsEmpty() && Destination.PeekBack()->m_iType == plTokenType::Whitespace)
    Destination.PopBack();

  return PLASMA_SUCCESS;
}

plResult plPreprocessor::EvaluateCondition(const TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult)
{
  iResult = 0;

  TokenStream Copied(&m_ClassAllocator);
  if (CopyTokensAndEvaluateDefined(Tokens, uiCurToken, Copied).Failed())
    return PLASMA_FAILURE;

  TokenStream Expanded(&m_ClassAllocator);

  if (Expand(Copied, Expanded).Failed())
    return PLASMA_FAILURE;

  if (Expanded.IsEmpty())
  {
    PP_LOG0(Error, "After expansion the condition is empty", Tokens[uiCurToken]);
    return PLASMA_FAILURE;
  }

  plUInt32 uiCurToken2 = 0;
  if (ParseExpressionOr(Expanded, uiCurToken2, iResult).Failed())
    return PLASMA_FAILURE;

  return ExpectEndOfLine(Expanded, uiCurToken2);
}

plResult plPreprocessor::ParseFactor(const TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult)
{
  while (Accept(Tokens, uiCurToken, "+"))
  {
  }

  if (Accept(Tokens, uiCurToken, "-"))
  {
    if (ParseFactor(Tokens, uiCurToken, iResult).Failed())
      return PLASMA_FAILURE;

    iResult = -iResult;
    return PLASMA_SUCCESS;
  }

  if (Accept(Tokens, uiCurToken, "~"))
  {
    if (ParseFactor(Tokens, uiCurToken, iResult).Failed())
      return PLASMA_FAILURE;

    iResult = ~iResult;
    return PLASMA_SUCCESS;
  }

  if (Accept(Tokens, uiCurToken, "!"))
  {
    if (ParseFactor(Tokens, uiCurToken, iResult).Failed())
      return PLASMA_FAILURE;

    iResult = (iResult != 0) ? 0 : 1;
    return PLASMA_SUCCESS;
  }

  plUInt32 uiValueToken = uiCurToken;
  if (Accept(Tokens, uiCurToken, plTokenType::Identifier, &uiValueToken) || Accept(Tokens, uiCurToken, plTokenType::Integer, &uiValueToken))
  {
    const plString sVal = Tokens[uiValueToken]->m_DataView;

    plInt32 iResult32 = 0;

    if (sVal == "true")
    {
      iResult32 = 1;
    }
    else if (sVal == "false")
    {
      iResult32 = 0;
    }
    else if (plConversionUtils::StringToInt(sVal, iResult32).Failed())
    {
      // this is not an error, all unknown identifiers are assumed to be zero

      // broadcast that we encountered this unknown identifier
      ProcessingEvent pe;
      pe.m_pToken = Tokens[uiValueToken];
      pe.m_Type = ProcessingEvent::EvaluateUnknown;
      m_ProcessingEvents.Broadcast(pe);
    }

    iResult = (plInt64)iResult32;

    return PLASMA_SUCCESS;
  }
  else if (Accept(Tokens, uiCurToken, "("))
  {
    if (ParseExpressionOr(Tokens, uiCurToken, iResult).Failed())
      return PLASMA_FAILURE;

    return Expect(Tokens, uiCurToken, ")");
  }

  uiCurToken = plMath::Min(uiCurToken, Tokens.GetCount() - 1);
  PP_LOG0(Error, "Syntax error, expected identifier, number or '('", Tokens[uiCurToken]);

  return PLASMA_FAILURE;
}

plResult plPreprocessor::ParseExpressionPlus(const TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult)
{
  if (ParseExpressionMul(Tokens, uiCurToken, iResult).Failed())
    return PLASMA_FAILURE;

  while (true)
  {
    if (Accept(Tokens, uiCurToken, "+"))
    {
      plInt64 iNextValue = 0;
      if (ParseExpressionMul(Tokens, uiCurToken, iNextValue).Failed())
        return PLASMA_FAILURE;

      iResult += iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "-"))
    {
      plInt64 iNextValue = 0;
      if (ParseExpressionMul(Tokens, uiCurToken, iNextValue).Failed())
        return PLASMA_FAILURE;

      iResult -= iNextValue;
    }
    else
      break;
  }

  return PLASMA_SUCCESS;
}

plResult plPreprocessor::ParseExpressionShift(const TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult)
{
  if (ParseExpressionPlus(Tokens, uiCurToken, iResult).Failed())
    return PLASMA_FAILURE;

  while (true)
  {
    if (Accept(Tokens, uiCurToken, ">", ">"))
    {
      plInt64 iNextValue = 0;
      if (ParseExpressionPlus(Tokens, uiCurToken, iNextValue).Failed())
        return PLASMA_FAILURE;

      iResult >>= iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "<", "<"))
    {
      plInt64 iNextValue = 0;
      if (ParseExpressionPlus(Tokens, uiCurToken, iNextValue).Failed())
        return PLASMA_FAILURE;

      iResult <<= iNextValue;
    }
    else
      break;
  }

  return PLASMA_SUCCESS;
}

plResult plPreprocessor::ParseExpressionOr(const TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult)
{
  if (ParseExpressionAnd(Tokens, uiCurToken, iResult).Failed())
    return PLASMA_FAILURE;

  while (Accept(Tokens, uiCurToken, "|", "|"))
  {
    plInt64 iNextValue = 0;
    if (ParseExpressionAnd(Tokens, uiCurToken, iNextValue).Failed())
      return PLASMA_FAILURE;

    iResult = (iResult != 0 || iNextValue != 0) ? 1 : 0;
  }

  return PLASMA_SUCCESS;
}

plResult plPreprocessor::ParseExpressionAnd(const TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult)
{
  if (ParseExpressionBitOr(Tokens, uiCurToken, iResult).Failed())
    return PLASMA_FAILURE;

  while (Accept(Tokens, uiCurToken, "&", "&"))
  {
    plInt64 iNextValue = 0;
    if (ParseExpressionBitOr(Tokens, uiCurToken, iNextValue).Failed())
      return PLASMA_FAILURE;

    iResult = (iResult != 0 && iNextValue != 0) ? 1 : 0;
  }

  return PLASMA_SUCCESS;
}

plResult plPreprocessor::ParseExpressionBitOr(const TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult)
{
  if (ParseExpressionBitXor(Tokens, uiCurToken, iResult).Failed())
    return PLASMA_FAILURE;

  while (AcceptUnless(Tokens, uiCurToken, "|", "|"))
  {
    plInt64 iNextValue = 0;
    if (ParseExpressionBitXor(Tokens, uiCurToken, iNextValue).Failed())
      return PLASMA_FAILURE;

    iResult |= iNextValue;
  }

  return PLASMA_SUCCESS;
}

plResult plPreprocessor::ParseExpressionBitAnd(const TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult)
{
  if (ParseCondition(Tokens, uiCurToken, iResult).Failed())
    return PLASMA_FAILURE;

  while (AcceptUnless(Tokens, uiCurToken, "&", "&"))
  {
    plInt64 iNextValue = 0;
    if (ParseCondition(Tokens, uiCurToken, iNextValue).Failed())
      return PLASMA_FAILURE;

    iResult &= iNextValue;
  }

  return PLASMA_SUCCESS;
}

plResult plPreprocessor::ParseExpressionBitXor(const TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult)
{
  if (ParseExpressionBitAnd(Tokens, uiCurToken, iResult).Failed())
    return PLASMA_FAILURE;

  while (Accept(Tokens, uiCurToken, "^"))
  {
    plInt64 iNextValue = 0;
    if (ParseExpressionBitAnd(Tokens, uiCurToken, iNextValue).Failed())
      return PLASMA_FAILURE;

    iResult ^= iNextValue;
  }

  return PLASMA_SUCCESS;
}
plResult plPreprocessor::ParseExpressionMul(const TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult)
{
  if (ParseFactor(Tokens, uiCurToken, iResult).Failed())
    return PLASMA_FAILURE;

  while (true)
  {
    if (Accept(Tokens, uiCurToken, "*"))
    {
      plInt64 iNextValue = 0;
      if (ParseFactor(Tokens, uiCurToken, iNextValue).Failed())
        return PLASMA_FAILURE;

      iResult *= iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "/"))
    {
      plInt64 iNextValue = 0;
      if (ParseFactor(Tokens, uiCurToken, iNextValue).Failed())
        return PLASMA_FAILURE;

      iResult /= iNextValue;
    }
    else if (Accept(Tokens, uiCurToken, "%"))
    {
      plInt64 iNextValue = 0;
      if (ParseFactor(Tokens, uiCurToken, iNextValue).Failed())
        return PLASMA_FAILURE;

      iResult %= iNextValue;
    }
    else
      break;
  }

  return PLASMA_SUCCESS;
}

enum class Comparison
{
  None,
  Equal,
  Unequal,
  LessThan,
  GreaterThan,
  LessThanEqual,
  GreaterThanEqual
};

plResult plPreprocessor::ParseCondition(const TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult)
{
  plInt64 iResult1 = 0;
  if (ParseExpressionShift(Tokens, uiCurToken, iResult1).Failed())
    return PLASMA_FAILURE;

  Comparison Operator = Comparison::None;

  if (Accept(Tokens, uiCurToken, "=", "="))
    Operator = Comparison::Equal;
  else if (Accept(Tokens, uiCurToken, "!", "="))
    Operator = Comparison::Unequal;
  else if (Accept(Tokens, uiCurToken, ">", "="))
    Operator = Comparison::GreaterThanEqual;
  else if (Accept(Tokens, uiCurToken, "<", "="))
    Operator = Comparison::LessThanEqual;
  else if (AcceptUnless(Tokens, uiCurToken, ">", ">"))
    Operator = Comparison::GreaterThan;
  else if (AcceptUnless(Tokens, uiCurToken, "<", "<"))
    Operator = Comparison::LessThan;
  else
  {
    iResult = iResult1;
    return PLASMA_SUCCESS;
  }

  plInt64 iResult2 = 0;
  if (ParseExpressionShift(Tokens, uiCurToken, iResult2).Failed())
    return PLASMA_FAILURE;

  switch (Operator)
  {
    case Comparison::Equal:
      iResult = (iResult1 == iResult2) ? 1 : 0;
      return PLASMA_SUCCESS;
    case Comparison::GreaterThan:
      iResult = (iResult1 > iResult2) ? 1 : 0;
      return PLASMA_SUCCESS;
    case Comparison::GreaterThanEqual:
      iResult = (iResult1 >= iResult2) ? 1 : 0;
      return PLASMA_SUCCESS;
    case Comparison::LessThan:
      iResult = (iResult1 < iResult2) ? 1 : 0;
      return PLASMA_SUCCESS;
    case Comparison::LessThanEqual:
      iResult = (iResult1 <= iResult2) ? 1 : 0;
      return PLASMA_SUCCESS;
    case Comparison::Unequal:
      iResult = (iResult1 != iResult2) ? 1 : 0;
      return PLASMA_SUCCESS;
    case Comparison::None:
      plLog::Error(m_pLog, "Unknown operator");
      return PLASMA_FAILURE;
  }

  return PLASMA_FAILURE;
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Implementation_Conditions);
