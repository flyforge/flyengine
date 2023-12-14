#pragma once

#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Containers/HybridArray.h>

namespace plTokenParseUtils
{
  using TokenStream = plHybridArray<const plToken*, 32>;

  PLASMA_FOUNDATION_DLL void SkipWhitespace(const TokenStream& tokens, plUInt32& ref_uiCurToken);
  PLASMA_FOUNDATION_DLL void SkipWhitespaceAndNewline(const TokenStream& tokens, plUInt32& ref_uiCurToken);
  PLASMA_FOUNDATION_DLL bool IsEndOfLine(const TokenStream& tokens, plUInt32 uiCurToken, bool bIgnoreWhitespace);
  PLASMA_FOUNDATION_DLL void CopyRelevantTokens(const TokenStream& source, plUInt32 uiFirstSourceToken, TokenStream& ref_destination, bool bPreserveNewLines);

  PLASMA_FOUNDATION_DLL bool Accept(const TokenStream& tokens, plUInt32& ref_uiCurToken, plStringView sToken, plUInt32* pAccepted = nullptr);
  PLASMA_FOUNDATION_DLL bool Accept(const TokenStream& tokens, plUInt32& ref_uiCurToken, plTokenType::Enum type, plUInt32* pAccepted = nullptr);
  PLASMA_FOUNDATION_DLL bool Accept(const TokenStream& tokens, plUInt32& ref_uiCurToken, plStringView sToken1, plStringView sToken2, plUInt32* pAccepted = nullptr);
  PLASMA_FOUNDATION_DLL bool AcceptUnless(const TokenStream& tokens, plUInt32& ref_uiCurToken, plStringView sToken1, plStringView sToken2, plUInt32* pAccepted = nullptr);

  struct TokenMatch
  {
    TokenMatch(plStringView sToken) : m_sToken(sToken) {}
    TokenMatch(plTokenType::Enum type) : m_Type(type) {}

    plStringView m_sToken;
    plTokenType::Enum m_Type = plTokenType::Unknown;
  };
  PLASMA_FOUNDATION_DLL bool Accept(const TokenStream& tokens, plUInt32& ref_uiCurToken, plArrayPtr<const TokenMatch> matches, plDynamicArray<plUInt32>* pAccepted = nullptr);

  PLASMA_FOUNDATION_DLL void CombineTokensToString(const TokenStream& tokens, plUInt32 uiCurToken, plStringBuilder& ref_sResult, bool bKeepComments = true, bool bRemoveRedundantWhitespace = false, bool bInsertLine = false);
  PLASMA_FOUNDATION_DLL void CombineRelevantTokensToString(const TokenStream& tokens, plUInt32 uiCurToken, plStringBuilder& ref_sResult);
  PLASMA_FOUNDATION_DLL void CreateCleanTokenStream(const TokenStream& tokens, plUInt32 uiCurToken, TokenStream& ref_destination, bool bKeepComments);
} // namespace plTokenParseUtils
