#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>

using namespace plTokenParseUtils;

plPreprocessor::MacroDefinition::MacroDefinition()
{
  m_MacroIdentifier = nullptr;
  m_bIsFunction = false;
  m_bCurrentlyExpanding = false;
  m_iNumParameters = -1;
  m_bHasVarArgs = false;
}

void plPreprocessor::CopyTokensReplaceParams(const TokenStream& Source, plUInt32 uiFirstSourceToken, TokenStream& Destination, const plHybridArray<plString, 16>& parameters)
{
  Destination.Clear();
  Destination.Reserve(Source.GetCount() - uiFirstSourceToken);

  {
    // skip all whitespace at the start of the replacement string
    plUInt32 i = uiFirstSourceToken;
    SkipWhitespace(Source, i);

    // add all the relevant tokens to the definition
    for (; i < Source.GetCount(); ++i)
    {
      if (Source[i]->m_iType == plTokenType::BlockComment || Source[i]->m_iType == plTokenType::LineComment || Source[i]->m_iType == plTokenType::EndOfFile || Source[i]->m_iType == plTokenType::Newline)
        continue;

      if (Source[i]->m_iType == plTokenType::Identifier)
      {
        for (plUInt32 p = 0; p < parameters.GetCount(); ++p)
        {
          if (Source[i]->m_DataView == parameters[p])
          {
            // create a custom token for the parameter, for better error messages
            plToken* pParamToken = AddCustomToken(Source[i], parameters[p]);
            pParamToken->m_iType = s_iMacroParameter0 + p;

            Destination.PushBack(pParamToken);
            goto tokenfound;
          }
        }
      }

      Destination.PushBack(Source[i]);

    tokenfound:
      continue;
    }
  }

  // remove whitespace at end of macro
  while (!Destination.IsEmpty() && Destination.PeekBack()->m_iType == plTokenType::Whitespace)
    Destination.PopBack();
}

plResult plPreprocessor::ExtractParameterName(const TokenStream& Tokens, plUInt32& uiCurToken, plString& sIdentifierName)
{
  SkipWhitespace(Tokens, uiCurToken);

  if (uiCurToken + 2 < Tokens.GetCount() && Tokens[uiCurToken + 0]->m_DataView == "." && Tokens[uiCurToken + 1]->m_DataView == "." && Tokens[uiCurToken + 2]->m_DataView == ".")
  {
    sIdentifierName = "...";
    uiCurToken += 3;
  }
  else
  {
    plUInt32 uiParamToken = uiCurToken;

    if (Expect(Tokens, uiCurToken, plTokenType::Identifier, &uiParamToken).Failed())
      return PLASMA_FAILURE;

    sIdentifierName = Tokens[uiParamToken]->m_DataView;
  }

  // skip a trailing comma
  if (Accept(Tokens, uiCurToken, ","))
    SkipWhitespace(Tokens, uiCurToken);

  return PLASMA_SUCCESS;
}

plResult plPreprocessor::ExtractAllMacroParameters(const TokenStream& Tokens, plUInt32& uiCurToken, plDeque<TokenStream>& AllParameters)
{
  if (Expect(Tokens, uiCurToken, "(").Failed())
    return PLASMA_FAILURE;

  do
  {
    // add one parameter
    // note: we always add one extra parameter value, because MACRO() is actually a macro call with one empty parameter
    // the same for MACRO(a,) is a macro with two parameters, the second being empty
    AllParameters.SetCount(AllParameters.GetCount() + 1);

    if (ExtractParameterValue(Tokens, uiCurToken, AllParameters.PeekBack()).Failed())
      return PLASMA_FAILURE;

    // reached the end of the parameter list
    if (Accept(Tokens, uiCurToken, ")"))
      return PLASMA_SUCCESS;
  } while (Accept(Tokens, uiCurToken, ",")); // continue with the next parameter

  plString s = Tokens[uiCurToken]->m_DataView;
  PP_LOG(Error, "',' or ')' expected, got '{0}' instead", Tokens[uiCurToken], s);

  return PLASMA_FAILURE;
}

plResult plPreprocessor::ExtractParameterValue(const TokenStream& Tokens, plUInt32& uiCurToken, TokenStream& ParamTokens)
{
  SkipWhitespaceAndNewline(Tokens, uiCurToken);
  const plUInt32 uiFirstToken = plMath::Min(uiCurToken, Tokens.GetCount() - 1);

  plInt32 iParenthesis = 0;

  // get all tokens up until a comma or the last closing parenthesis
  // ignore commas etc. as long as they are surrounded with parenthesis
  for (; uiCurToken < Tokens.GetCount(); ++uiCurToken)
  {
    if (Tokens[uiCurToken]->m_iType == plTokenType::BlockComment || Tokens[uiCurToken]->m_iType == plTokenType::LineComment || Tokens[uiCurToken]->m_iType == plTokenType::Newline)
      continue;

    if (Tokens[uiCurToken]->m_iType == plTokenType::EndOfFile)
      break; // outputs an error

    if (iParenthesis == 0)
    {
      if (Tokens[uiCurToken]->m_DataView == "," || Tokens[uiCurToken]->m_DataView == ")")
      {
        if (!ParamTokens.IsEmpty() && ParamTokens.PeekBack()->m_iType == plTokenType::Whitespace)
        {
          ParamTokens.PopBack();
        }
        return PLASMA_SUCCESS;
      }
    }

    if (Tokens[uiCurToken]->m_DataView == "(")
      ++iParenthesis;
    else if (Tokens[uiCurToken]->m_DataView == ")")
      --iParenthesis;

    ParamTokens.PushBack(Tokens[uiCurToken]);
  }

  // reached the end of the stream without encountering the closing parenthesis first
  PP_LOG0(Error, "Unexpected end of file during macro parameter extraction", Tokens[uiFirstToken]);
  return PLASMA_FAILURE;
}

void plPreprocessor::StringifyTokens(const TokenStream& Tokens, plStringBuilder& sResult, bool bSurroundWithQuotes)
{
  plUInt32 uiCurToken = 0;

  sResult.Clear();

  if (bSurroundWithQuotes)
    sResult = "\"";

  plStringBuilder sTemp;

  SkipWhitespace(Tokens, uiCurToken);

  plUInt32 uiLastNonWhitespace = Tokens.GetCount();

  while (uiLastNonWhitespace > 0)
  {
    if (Tokens[uiLastNonWhitespace - 1]->m_iType != plTokenType::Whitespace && Tokens[uiLastNonWhitespace - 1]->m_iType != plTokenType::Newline && Tokens[uiLastNonWhitespace - 1]->m_iType != plTokenType::BlockComment && Tokens[uiLastNonWhitespace - 1]->m_iType != plTokenType::LineComment)
      break;

    --uiLastNonWhitespace;
  }

  for (plUInt32 t = uiCurToken; t < uiLastNonWhitespace; ++t)
  {
    // comments, newlines etc. are stripped out
    if ((Tokens[t]->m_iType == plTokenType::LineComment) || (Tokens[t]->m_iType == plTokenType::BlockComment) || (Tokens[t]->m_iType == plTokenType::Newline) || (Tokens[t]->m_iType == plTokenType::EndOfFile))
      continue;

    sTemp = Tokens[t]->m_DataView;

    // all whitespace becomes a single white space
    if (Tokens[t]->m_iType == plTokenType::Whitespace)
      sTemp = " ";

    // inside strings, all backslashes and double quotes are escaped
    if ((Tokens[t]->m_iType == plTokenType::String1) || (Tokens[t]->m_iType == plTokenType::String2))
    {
      sTemp.ReplaceAll("\\", "\\\\");
      sTemp.ReplaceAll("\"", "\\\"");
    }

    sResult.Append(sTemp.GetView());
  }

  if (bSurroundWithQuotes)
    sResult.Append("\"");
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Implementation_Macros);
