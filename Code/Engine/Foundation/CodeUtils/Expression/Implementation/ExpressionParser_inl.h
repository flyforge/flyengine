
inline bool plExpressionParser::AcceptStatementTerminator()
{
  return plTokenParseUtils::Accept(m_TokenStream, m_uiCurrentToken, plTokenType::Newline) ||
         plTokenParseUtils::Accept(m_TokenStream, m_uiCurrentToken, ";");
}

inline plResult plExpressionParser::Expect(plStringView sToken, const plToken** pExpectedToken)
{
  plUInt32 uiAcceptedToken = 0;
  if (plTokenParseUtils::Accept(m_TokenStream, m_uiCurrentToken, sToken, &uiAcceptedToken) == false)
  {
    const plUInt32 uiErrorToken = plMath::Min(m_TokenStream.GetCount() - 1, m_uiCurrentToken);
    auto pToken = m_TokenStream[uiErrorToken];
    ReportError(pToken, plFmt("Syntax error, expected {} but got {}", sToken, pToken->m_DataView));
    return PL_FAILURE;
  }

  if (pExpectedToken != nullptr)
  {
    *pExpectedToken = m_TokenStream[uiAcceptedToken];
  }

  return PL_SUCCESS;
}

inline plResult plExpressionParser::Expect(plTokenType::Enum Type, const plToken** pExpectedToken /*= nullptr*/)
{
  plUInt32 uiAcceptedToken = 0;
  if (plTokenParseUtils::Accept(m_TokenStream, m_uiCurrentToken, Type, &uiAcceptedToken) == false)
  {
    const plUInt32 uiErrorToken = plMath::Min(m_TokenStream.GetCount() - 1, m_uiCurrentToken);
    auto pToken = m_TokenStream[uiErrorToken];
    ReportError(pToken, plFmt("Syntax error, expected token type {} but got {}", plTokenType::EnumNames[Type], plTokenType::EnumNames[pToken->m_iType]));
    return PL_FAILURE;
  }

  if (pExpectedToken != nullptr)
  {
    *pExpectedToken = m_TokenStream[uiAcceptedToken];
  }

  return PL_SUCCESS;
}

inline void plExpressionParser::ReportError(const plToken* pToken, const plFormatString& message0)
{
  plStringBuilder tmp;
  plStringView message = message0.GetText(tmp);
  plLog::Error("{}({},{}): {}", pToken->m_File, pToken->m_uiLine, pToken->m_uiColumn, message);
}
