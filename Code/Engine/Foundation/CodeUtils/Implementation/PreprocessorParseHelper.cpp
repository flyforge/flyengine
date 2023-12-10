#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>

using namespace plTokenParseUtils;

plResult plPreprocessor::Expect(const TokenStream& Tokens, plUInt32& uiCurToken, plStringView sToken, plUInt32* pAccepted)
{
  if (Tokens.GetCount() < 1)
  {
    plLog::Error(m_pLog, "Expected token '{0}', got empty token stream", sToken);
    return PLASMA_FAILURE;
  }

  if (Accept(Tokens, uiCurToken, sToken, pAccepted))
    return PLASMA_SUCCESS;

  const plUInt32 uiErrorToken = plMath::Min(Tokens.GetCount() - 1, uiCurToken);
  plString sErrorToken = Tokens[uiErrorToken]->m_DataView;
  PP_LOG(Error, "Expected token '{0}' got '{1}'", Tokens[uiErrorToken], sToken, sErrorToken);

  return PLASMA_FAILURE;
}

plResult plPreprocessor::Expect(const TokenStream& Tokens, plUInt32& uiCurToken, plTokenType::Enum Type, plUInt32* pAccepted)
{
  if (Tokens.GetCount() < 1)
  {
    plLog::Error(m_pLog, "Expected token of type '{0}', got empty token stream", plTokenType::EnumNames[Type]);
    return PLASMA_FAILURE;
  }

  if (Accept(Tokens, uiCurToken, Type, pAccepted))
    return PLASMA_SUCCESS;

  const plUInt32 uiErrorToken = plMath::Min(Tokens.GetCount() - 1, uiCurToken);
  PP_LOG(Error, "Expected token of type '{0}' got type '{1}' instead", Tokens[uiErrorToken], plTokenType::EnumNames[Type], plTokenType::EnumNames[Tokens[uiErrorToken]->m_iType]);

  return PLASMA_FAILURE;
}

plResult plPreprocessor::Expect(const TokenStream& Tokens, plUInt32& uiCurToken, plStringView sToken1, plStringView sToken2, plUInt32* pAccepted)
{
  if (Tokens.GetCount() < 2)
  {
    plLog::Error(m_pLog, "Expected tokens '{0}{1}', got empty token stream", sToken1, sToken2);
    return PLASMA_FAILURE;
  }

  if (Accept(Tokens, uiCurToken, sToken1, sToken2, pAccepted))
    return PLASMA_SUCCESS;

  const plUInt32 uiErrorToken = plMath::Min(Tokens.GetCount() - 2, uiCurToken);
  plString sErrorToken1 = Tokens[uiErrorToken]->m_DataView;
  plString sErrorToken2 = Tokens[uiErrorToken + 1]->m_DataView;
  PP_LOG(Error, "Expected tokens '{0}{1}', got '{2}{3}'", Tokens[uiErrorToken], sToken1, sToken2, sErrorToken1, sErrorToken2);

  return PLASMA_FAILURE;
}

plResult plPreprocessor::ExpectEndOfLine(const TokenStream& Tokens, plUInt32 uiCurToken)
{
  if (!IsEndOfLine(Tokens, uiCurToken, true))
  {
    plString sToken = Tokens[uiCurToken]->m_DataView;
    PP_LOG(Warning, "Expected end-of-line, found token '{0}'", Tokens[uiCurToken], sToken);
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Implementation_PreprocessorParseHelper);
