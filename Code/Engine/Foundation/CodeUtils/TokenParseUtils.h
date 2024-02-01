#pragma once

#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Containers/HybridArray.h>

namespace plTokenParseUtils
{
  using TokenStream = plHybridArray<const plToken*, 32>;

  /// \brief Moves ref_uiCurToken forward as long as it is a whitespace token (Whitespace, BlockComment, LineComment).
  /// \param tokens Token input.
  /// \param ref_uiCurToken Current location inside 'tokens'. On return, will either stay the same or move forward.
  PL_FOUNDATION_DLL void SkipWhitespace(const TokenStream& tokens, plUInt32& ref_uiCurToken); // [tested]

  /// \brief Moves ref_uiCurToken forward as long as it is a whitespace token (Whitespace, BlockComment, LineComment) or Newline.
  /// \param tokens Token input.
  /// \param ref_uiCurToken Current location inside 'tokens'. On return, will either stay the same or move forward.
  PL_FOUNDATION_DLL void SkipWhitespaceAndNewline(const TokenStream& tokens, plUInt32& ref_uiCurToken); // [tested]

  /// \brief Checks whether we are at the end of a line.
  /// \param tokens Token input.
  /// \param uiCurToken Current location inside 'tokens'.
  /// \param bIgnoreWhitespace If false, only uiCurToken is checked. If true, whitespace will be skipped in search for a Newline token.
  /// \return Whether we are at the end of a line.
  PL_FOUNDATION_DLL bool IsEndOfLine(const TokenStream& tokens, plUInt32 uiCurToken, bool bIgnoreWhitespace); // [tested]

  /// \brief Strips out BlockComment, LineComment, EndOfFile tokens and Newline as well if requested and copies the rest to ref_destination.
  /// \param source From where to copy the tokens.
  /// \param uiFirstSourceToken Start token in 'source'. Whitespace at the start will be stripped as well.
  /// \param ref_destination Copy target of the tokens.
  /// \param bPreserveNewLines Whether to preserve Newline tokens.
  PL_FOUNDATION_DLL void CopyRelevantTokens(const TokenStream& source, plUInt32 uiFirstSourceToken, TokenStream& ref_destination, bool bPreserveNewLines); // [tested]

  /// \brief Tries to move ref_uiCurToken forward by matching against 'sToken'. The function will skip whitespace tokens (Whitespace, BlockComment, LineComment) in search of 'sToken'.
  /// \param tokens Token input.
  /// \param ref_uiCurToken Current location inside 'tokens'. If false is returned it will stay the same. If true is returned, it will be the next index after matching 'sToken'.
  /// \param sToken The token to be matched.
  /// \param pAccepted If not null and true was returned, will be set to the index at which 'sToken' was matched.
  /// \return Whether the token was found.
  PL_FOUNDATION_DLL bool Accept(const TokenStream& tokens, plUInt32& ref_uiCurToken, plStringView sToken, plUInt32* pAccepted = nullptr); // [tested]

  /// \brief Overload that matches against a token type instead.
  PL_FOUNDATION_DLL bool Accept(const TokenStream& tokens, plUInt32& ref_uiCurToken, plTokenType::Enum type, plUInt32* pAccepted = nullptr); // [tested]

  /// \brief Tries to move ref_uiCurToken forward by matching against a token tuple. The function will skip whitespace tokens (Whitespace, BlockComment, LineComment) in search of the tokens.
  /// \param tokens Token input.
  /// \param ref_uiCurToken Current location inside 'tokens'. If false is returned it will stay the same. If true is returned, it will be the next index after matching after the matched 'sToken2'.
  /// \param sToken1 First token to match.
  /// \param sToken2 Second token to match. Must be followed right after 'sToken1'.
  /// \param pAccepted If not null and true was returned, will be set to the index at which 'sToken1' was matched.
  /// \return Whether the token tuple was found.
  PL_FOUNDATION_DLL bool Accept(const TokenStream& tokens, plUInt32& ref_uiCurToken, plStringView sToken1, plStringView sToken2, plUInt32* pAccepted = nullptr); // [tested]

  /// \brief Tries to move ref_uiCurToken forward by matching against 'sToken1' unless it is followed right after by 'sToken2'. The function will skip whitespace tokens (Whitespace, BlockComment, LineComment) in search of 'sToken1'.
  /// \param tokens Token input.
  /// \param ref_uiCurToken Current location inside 'tokens'. If false is returned it will stay the same. If true is returned, it will be the next index after matching 'sToken1'.
  /// \param sToken1 The token to be matched.
  /// \param sToken2 The token that must not follow right after 'sToken1'.
  /// \param pAccepted If not null and true was returned, will be set to the index at which 'sToken1' was matched.
  /// \return Whether the token was found.
  PL_FOUNDATION_DLL bool AcceptUnless(const TokenStream& tokens, plUInt32& ref_uiCurToken, plStringView sToken1, plStringView sToken2, plUInt32* pAccepted = nullptr); // [tested]

  /// \brief A token to be matched by the plTokenParseUtils::Accept overload for arrays of tokens.
  /// Can either match a token string or type. If type is plTokenType::Unknown, the token string will be matched.
  struct TokenMatch
  {
    /// \brief This matches a token string of any type.
    TokenMatch(plStringView sToken) : m_sToken(sToken) {}
    /// \brief This matches a token type of any string value.
    TokenMatch(plTokenType::Enum type) : m_Type(type) {}

    /// \brief For internal use. Use one of the other constructors instead.
    TokenMatch(plTokenType::Enum type, plStringView sToken) : m_Type(type), m_sToken(sToken) {}

    plTokenType::Enum m_Type = plTokenType::Unknown;
    plStringView m_sToken;
  };

  /// \brief Tries to move ref_uiCurToken forward by matching against an array of tokens. The function will skip whitespace tokens (Whitespace, BlockComment, LineComment) between the tokens it tries to match within 'matches'.
  ///
  /// Here is an example how to parse a vector declaration:
  /// \code{.cpp}
  ///   plTokenParseUtils::TokenMatch pattern[] = {"Vec2"_plsv, "("_plsv, plTokenType::Float, ","_plsv, plTokenType::Float, ")"_plsv};
  ///   plHybridArray<plUInt32, 6> matchedTokens;
  ///   PL_TEST_BOOL(plTokenParseUtils::Accept(tokens, uiCurToken, pattern, &matchedTokens));
  /// \endcode
  ///
  /// \param tokens Token input.
  /// \param ref_uiCurToken Current location inside 'tokens'. If false is returned it will stay the same. If true is returned, it will be the next index after matching all of 'matches'.
  /// \param matches The tokens to be matched. These can be separated by whitespace tokens.
  /// \param pAccepted If not null and true was returned, will be filled with the indices at which each token inside 'matches' was matched.
  /// \return Whether the array of tokens was found.
  PL_FOUNDATION_DLL bool Accept(const TokenStream& tokens, plUInt32& ref_uiCurToken, plArrayPtr<const TokenMatch> matches, plDynamicArray<plUInt32>* pAccepted = nullptr); // [tested]

  /// \brief Combines tokens to a string.
  /// \param tokens The token stream to combine.
  /// \param uiCurToken The start location inside 'tokens' from which point the tokens should be combined.
  /// \param ref_sResult Holds the resulting string after the function call.
  /// \param bKeepComments Whether comments should be written into the string.
  /// \param bRemoveRedundantWhitespace Whether redundant whitespace should be removed.
  /// \param bInsertLine If set, will insert macros in the form of '#line <LINE> "<FILE>"' where appropriate. This is used as a hint by debuggers and other tools to reconstruct in which line/file a failure in a script occurred.
  PL_FOUNDATION_DLL void CombineTokensToString(const TokenStream& tokens, plUInt32 uiCurToken, plStringBuilder& ref_sResult, bool bKeepComments = true, bool bRemoveRedundantWhitespace = false, bool bInsertLine = false);

  /// \brief Strips out BlockComment, LineComment, EndOfFile and NewLine tokens and combines the rest into a string.
  /// \param tokens The token stream to combine.
  /// \param uiCurToken The start location inside 'tokens' from which point the tokens should be combined.
  /// \param ref_sResult Holds the resulting string after the function call.
  PL_FOUNDATION_DLL void CombineRelevantTokensToString(const TokenStream& tokens, plUInt32 uiCurToken, plStringBuilder& ref_sResult);

  /// \brief Removes whitespace at the end of each line and removes NewLine that follow each other.
  /// \param tokens The token stream to cleaned up.
  /// \param uiCurToken The start location inside 'tokens' from which point the tokens should be copied to 'ref_destination'.
  /// \param ref_destination Target stream to store the cleaned up tokens into.
  PL_FOUNDATION_DLL void CreateCleanTokenStream(const TokenStream& tokens, plUInt32 uiCurToken, TokenStream& ref_destination);
} // namespace plTokenParseUtils
