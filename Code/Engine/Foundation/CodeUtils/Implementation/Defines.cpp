#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>

using namespace plTokenParseUtils;

bool plPreprocessor::RemoveDefine(plStringView sName)
{
  auto it = m_Macros.Find(sName);

  if (it.IsValid())
  {
    m_Macros.Remove(it);
    return true;
  }

  return false;
}


plResult plPreprocessor::StoreDefine(const plToken* pMacroNameToken, const TokenStream* pReplacementTokens, plUInt32 uiFirstReplacementToken, plInt32 iNumParameters, bool bUsesVarArgs)
{
  if ((pMacroNameToken->m_DataView.IsEqual("defined")) || (pMacroNameToken->m_DataView.IsEqual("__FILE__")) || (pMacroNameToken->m_DataView.IsEqual("__LINE__")))
  {
    PP_LOG(Error, "Macro name '{0}' is reserved", pMacroNameToken, pMacroNameToken->m_DataView);
    return PL_FAILURE;
  }

  MacroDefinition md;
  md.m_MacroIdentifier = pMacroNameToken;
  md.m_bIsFunction = iNumParameters >= 0;
  md.m_iNumParameters = plMath::Max(0, iNumParameters);
  md.m_bHasVarArgs = bUsesVarArgs;

  // removes whitespace at start and end, skips comments, newlines, etc.
  if (pReplacementTokens)
    CopyRelevantTokens(*pReplacementTokens, uiFirstReplacementToken, md.m_Replacement, false);

  if (!md.m_Replacement.IsEmpty() && md.m_Replacement.PeekBack()->m_DataView == "#")
  {
    PP_LOG(Error, "Macro '{0}' ends with invalid character '#'", md.m_Replacement.PeekBack(), pMacroNameToken->m_DataView);
    return PL_FAILURE;
  }

  /* make sure all replacements are not empty
  {
    plToken Whitespace;
    Whitespace.m_File = pMacroNameToken->m_File;
    Whitespace.m_iType = plTokenType::Whitespace;
    Whitespace.m_uiColumn = pMacroNameToken->m_uiColumn + sMacroName.GetCharacterCount() + 1;
    Whitespace.m_uiLine = pMacroNameToken->m_uiLine;

    md.m_Replacement.PushBack(AddCustomToken(&Whitespace, ""));
  }*/

  bool bExisted = false;
  auto it = m_Macros.FindOrAdd(pMacroNameToken->m_DataView, &bExisted);

  ProcessingEvent pe;
  pe.m_Type = bExisted ? ProcessingEvent::Redefine : ProcessingEvent::Define;
  pe.m_pToken = pMacroNameToken;
  m_ProcessingEvents.Broadcast(pe);

  if (bExisted)
  {
    PP_LOG(Warning, "Redefinition of macro '{0}'", pMacroNameToken, pMacroNameToken->m_DataView);
    // return PL_FAILURE;
  }

  it.Value() = md;
  return PL_SUCCESS;
}

plResult plPreprocessor::HandleDefine(const TokenStream& Tokens, plUInt32& uiCurToken)
{
  SkipWhitespace(Tokens, uiCurToken);

  plUInt32 uiNameToken = uiCurToken;

  if (Expect(Tokens, uiCurToken, plTokenType::Identifier, &uiNameToken).Failed())
    return PL_FAILURE;

  // check if we got an empty macro definition
  if (IsEndOfLine(Tokens, uiCurToken, true))
  {
    plStringBuilder sDefine = Tokens[uiNameToken]->m_DataView;

    return StoreDefine(Tokens[uiNameToken], nullptr, 0, -1, false);
  }

  // first determine whether this is a function macro (before skipping whitespace)
  if (Tokens[uiCurToken]->m_DataView != "(")
  {
    // add the rest of the macro definition as the replacement
    return StoreDefine(Tokens[uiNameToken], &Tokens, uiCurToken, -1, false);
  }

  // extract parameter names
  {
    bool bVarArgsFounds = false;

    // skip the opening parenthesis (
    if (Expect(Tokens, uiCurToken, "(").Failed())
      return PL_FAILURE;

    plHybridArray<plString, 16> parameters;

    while (!Accept(Tokens, uiCurToken, ")"))
    {
      if (uiCurToken >= Tokens.GetCount())
      {
        PP_LOG(Error, "Could not extract macro parameter {0}, reached end of token stream first", Tokens[Tokens.GetCount() - 1], parameters.GetCount());
        return PL_FAILURE;
      }

      const plUInt32 uiCurParamToken = uiCurToken;

      plString sParam;
      if (ExtractParameterName(Tokens, uiCurToken, sParam) == PL_FAILURE)
      {
        PP_LOG(Error, "Could not extract macro parameter {0}", Tokens[uiCurParamToken], parameters.GetCount());
        return PL_FAILURE;
      }

      if (bVarArgsFounds)
      {
        PP_LOG0(Error, "No additional parameters are allowed after '...'", Tokens[uiCurParamToken]);
        return PL_FAILURE;
      }

      /// \todo Make sure the same parameter name is not used twice

      if (sParam == "...")
      {
        bVarArgsFounds = true;
        sParam = "__VA_ARGS__";
      }

      parameters.PushBack(sParam);
    }

    TokenStream ReplacementTokens;
    CopyTokensReplaceParams(Tokens, uiCurToken, ReplacementTokens, parameters);

    PL_SUCCEED_OR_RETURN(StoreDefine(Tokens[uiNameToken], &ReplacementTokens, 0, parameters.GetCount(), bVarArgsFounds));
  }

  return PL_SUCCESS;
}

plResult plPreprocessor::AddCustomDefine(plStringView sDefinition)
{
  m_CustomDefines.PushBack();
  m_CustomDefines.PeekBack().m_Content.SetCountUninitialized(sDefinition.GetElementCount());
  plMemoryUtils::Copy(&m_CustomDefines.PeekBack().m_Content[0], (plUInt8*)sDefinition.GetStartPointer(), m_CustomDefines.PeekBack().m_Content.GetCount());
  m_CustomDefines.PeekBack().m_Tokenized.Tokenize(m_CustomDefines.PeekBack().m_Content, m_pLog);

  plUInt32 uiFirstToken = 0;
  plHybridArray<const plToken*, 32> Tokens;

  if (m_CustomDefines.PeekBack().m_Tokenized.GetNextLine(uiFirstToken, Tokens).Failed())
    return PL_FAILURE;

  plDeque<plToken>& NewTokens = m_CustomDefines.PeekBack().m_Tokenized.GetTokens();

  plHashedString sFile;
  sFile.Assign("<CustomDefines>");

  plUInt32 uiColumn = 1;
  for (plUInt32 t = 0; t < NewTokens.GetCount(); ++t)
  {
    NewTokens[t].m_File = sFile;
    NewTokens[t].m_uiLine = m_CustomDefines.GetCount();
    NewTokens[t].m_uiColumn = uiColumn;

    uiColumn += plStringUtils::GetCharacterCount(NewTokens[t].m_DataView.GetStartPointer(), NewTokens[t].m_DataView.GetEndPointer());
  }

  plUInt32 uiCurToken = 0;
  return HandleDefine(Tokens, uiCurToken);
}


