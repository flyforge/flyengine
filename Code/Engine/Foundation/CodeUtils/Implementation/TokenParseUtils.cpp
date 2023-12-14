#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>

namespace plTokenParseUtils
{
  void SkipWhitespace(const TokenStream& tokens, plUInt32& ref_uiCurToken)
  {
    while (ref_uiCurToken < tokens.GetCount() && ((tokens[ref_uiCurToken]->m_iType == plTokenType::Whitespace) || (tokens[ref_uiCurToken]->m_iType == plTokenType::BlockComment) || (tokens[ref_uiCurToken]->m_iType == plTokenType::LineComment)))
      ++ref_uiCurToken;
  }

  void SkipWhitespaceAndNewline(const TokenStream& tokens, plUInt32& ref_uiCurToken)
  {
    while (ref_uiCurToken < tokens.GetCount() && ((tokens[ref_uiCurToken]->m_iType == plTokenType::Whitespace) || (tokens[ref_uiCurToken]->m_iType == plTokenType::BlockComment) || (tokens[ref_uiCurToken]->m_iType == plTokenType::Newline) || (tokens[ref_uiCurToken]->m_iType == plTokenType::LineComment)))
      ++ref_uiCurToken;
  }

  bool IsEndOfLine(const TokenStream& tokens, plUInt32 uiCurToken, bool bIgnoreWhitespace)
  {
    if (bIgnoreWhitespace)
      SkipWhitespace(tokens, uiCurToken);

    if (uiCurToken >= tokens.GetCount())
      return true;

    return tokens[uiCurToken]->m_iType == plTokenType::Newline || tokens[uiCurToken]->m_iType == plTokenType::EndOfFile;
  }

  void CopyRelevantTokens(const TokenStream& source, plUInt32 uiFirstSourceToken, TokenStream& ref_destination, bool bPreserveNewLines)
  {
    ref_destination.Reserve(ref_destination.GetCount() + source.GetCount() - uiFirstSourceToken);

    {
      // skip all whitespace at the start of the replacement string
      plUInt32 i = uiFirstSourceToken;
      SkipWhitespace(source, i);

      // add all the relevant tokens to the definition
      for (; i < source.GetCount(); ++i)
      {
        if (source[i]->m_iType == plTokenType::BlockComment || source[i]->m_iType == plTokenType::LineComment || source[i]->m_iType == plTokenType::EndOfFile || (!bPreserveNewLines && source[i]->m_iType == plTokenType::Newline))
          continue;

        ref_destination.PushBack(source[i]);
      }
    }

    // remove whitespace at end of macro
    while (!ref_destination.IsEmpty() && ref_destination.PeekBack()->m_iType == plTokenType::Whitespace)
      ref_destination.PopBack();
  }

  bool Accept(const TokenStream& tokens, plUInt32& ref_uiCurToken, plStringView sToken, plUInt32* pAccepted)
  {
    SkipWhitespace(tokens, ref_uiCurToken);

    if (ref_uiCurToken >= tokens.GetCount())
      return false;

    if (tokens[ref_uiCurToken]->m_DataView == sToken)
    {
      if (pAccepted)
        *pAccepted = ref_uiCurToken;

      ref_uiCurToken++;
      return true;
    }

    return false;
  }

  bool Accept(const TokenStream& tokens, plUInt32& ref_uiCurToken, plTokenType::Enum type, plUInt32* pAccepted)
  {
    SkipWhitespace(tokens, ref_uiCurToken);

    if (ref_uiCurToken >= tokens.GetCount())
      return false;

    if (tokens[ref_uiCurToken]->m_iType == type)
    {
      if (pAccepted)
        *pAccepted = ref_uiCurToken;

      ref_uiCurToken++;
      return true;
    }

    return false;
  }

  bool Accept(const TokenStream& tokens, plUInt32& ref_uiCurToken, plStringView sToken1, plStringView sToken2, plUInt32* pAccepted)
  {
    SkipWhitespace(tokens, ref_uiCurToken);

    if (ref_uiCurToken + 1 >= tokens.GetCount())
      return false;

    if (tokens[ref_uiCurToken]->m_DataView == sToken1 && tokens[ref_uiCurToken + 1]->m_DataView == sToken2)
    {
      if (pAccepted)
        *pAccepted = ref_uiCurToken;

      ref_uiCurToken += 2;
      return true;
    }

    return false;
  }

  bool AcceptUnless(const TokenStream& tokens, plUInt32& ref_uiCurToken, plStringView sToken1, plStringView sToken2, plUInt32* pAccepted)
  {
    SkipWhitespace(tokens, ref_uiCurToken);

    if (ref_uiCurToken + 1 >= tokens.GetCount())
      return false;

    if (tokens[ref_uiCurToken]->m_DataView == sToken1 && tokens[ref_uiCurToken + 1]->m_DataView != sToken2)
    {
      if (pAccepted)
        *pAccepted = ref_uiCurToken;

      ref_uiCurToken += 1;
      return true;
    }

    return false;
  }

  bool Accept(const TokenStream& tokens, plUInt32& ref_uiCurToken, plArrayPtr<const TokenMatch> matches, plDynamicArray<plUInt32>* pAccepted)
  {
    if (pAccepted)
      pAccepted->Clear();

    plUInt32 uiCurToken = ref_uiCurToken;
    bool bAccepted = true;
    for (plUInt32 i = 0; i < matches.GetCount() && bAccepted; ++i)
    {
      plUInt32 uiAcceptedToken = uiCurToken;
      const TokenMatch& match = matches[i];
      if (match.m_Type == plTokenType::Unknown)
      {
        bAccepted = Accept(tokens, uiCurToken, match.m_sToken, &uiAcceptedToken);
      }
      else
      {
        bAccepted = Accept(tokens, uiCurToken, match.m_Type, &uiAcceptedToken);
      }

      if (pAccepted && bAccepted)
        pAccepted->PushBack(uiAcceptedToken);
    }

    if (bAccepted)
    {
      ref_uiCurToken = uiCurToken;
    }
    else
    {
      if (pAccepted)
        pAccepted->Clear();
    }
    return bAccepted;
  }

  void CombineRelevantTokensToString(const TokenStream& tokens, plUInt32 uiCurToken, plStringBuilder& ref_sResult)
  {
    ref_sResult.Clear();
    plStringBuilder sTemp;

    for (plUInt32 t = uiCurToken; t < tokens.GetCount(); ++t)
    {
      if ((tokens[t]->m_iType == plTokenType::LineComment) || (tokens[t]->m_iType == plTokenType::BlockComment) || (tokens[t]->m_iType == plTokenType::Newline) || (tokens[t]->m_iType == plTokenType::EndOfFile))
        continue;

      sTemp = tokens[t]->m_DataView;
      ref_sResult.Append(sTemp.GetView());
    }
  }

  void CreateCleanTokenStream(const TokenStream& tokens, plUInt32 uiCurToken, TokenStream& ref_destination, bool bKeepComments)
  {
    SkipWhitespace(tokens, uiCurToken);

    for (plUInt32 t = uiCurToken; t < tokens.GetCount(); ++t)
    {
      if (tokens[t]->m_iType == plTokenType::Newline)
      {
        // remove all whitespace before a newline
        while (!ref_destination.IsEmpty() && ref_destination.PeekBack()->m_iType == plTokenType::Whitespace)
          ref_destination.PopBack();

        // if there is already a newline stored, discard the new one
        if (!ref_destination.IsEmpty() && ref_destination.PeekBack()->m_iType == plTokenType::Newline)
          continue;
      }

      ref_destination.PushBack(tokens[t]);
    }
  }

  void CombineTokensToString(const TokenStream& tokens0, plUInt32 uiCurToken, plStringBuilder& ref_sResult, bool bKeepComments, bool bRemoveRedundantWhitespace, bool bInsertLine)
  {
    TokenStream Tokens;

    if (bRemoveRedundantWhitespace)
    {
      CreateCleanTokenStream(tokens0, uiCurToken, Tokens, bKeepComments);
      uiCurToken = 0;
    }
    else
      Tokens = tokens0;

    ref_sResult.Clear();
    plStringBuilder sTemp;

    plUInt32 uiCurLine = 0xFFFFFFFF;
    plHashedString sCurFile;

    for (plUInt32 t = uiCurToken; t < Tokens.GetCount(); ++t)
    {
      // skip all comments, if not desired
      if ((Tokens[t]->m_iType == plTokenType::BlockComment || Tokens[t]->m_iType == plTokenType::LineComment) && !bKeepComments)
        continue;

      if (Tokens[t]->m_iType == plTokenType::EndOfFile)
        return;

      if (bInsertLine)
      {
        if (ref_sResult.IsEmpty())
        {
          ref_sResult.AppendFormat("#line {0} \"{1}\"\n", Tokens[t]->m_uiLine, Tokens[t]->m_File);
          uiCurLine = Tokens[t]->m_uiLine;
          sCurFile = Tokens[t]->m_File;
        }

        if (Tokens[t]->m_iType == plTokenType::Newline)
        {
          ++uiCurLine;
        }

        if (t > 0 && Tokens[t - 1]->m_iType == plTokenType::Newline)
        {
          if (Tokens[t]->m_uiLine != uiCurLine || Tokens[t]->m_File != sCurFile)
          {
            ref_sResult.AppendFormat("\n#line {0} \"{1}\"\n", Tokens[t]->m_uiLine, Tokens[t]->m_File);
            uiCurLine = Tokens[t]->m_uiLine;
            sCurFile = Tokens[t]->m_File;
          }
        }
      }

      sTemp = Tokens[t]->m_DataView;
      ref_sResult.Append(sTemp.GetView());
    }
  }
} // namespace plTokenParseUtils

PLASMA_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Implementation_TokenParseUtils);
