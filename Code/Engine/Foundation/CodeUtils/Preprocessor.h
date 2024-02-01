#pragma once

#include <Foundation/Basics.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Time/Timestamp.h>

/// \brief This object caches files in a tokenized state. It can be shared among plPreprocessor instances to improve performance when
/// they access the same files.
class PL_FOUNDATION_DLL plTokenizedFileCache
{
public:
  struct FileData
  {
    plTokenizer m_Tokens;
    plTimestamp m_Timestamp;
  };

  /// \brief Checks whether \a sFileName is already in the cache, returns an iterator to it. If the iterator is invalid, the file is not cached yet.
  plMap<plString, FileData>::ConstIterator Lookup(const plString& sFileName) const;

  /// \brief Removes the cached content for \a sFileName from the cache. Should be used when the file content has changed and needs to be re-read.
  void Remove(const plString& sFileName);

  /// \brief Removes all files from the cache to ensure that they will be re-read.
  void Clear();

  /// \brief Stores \a FileContent for the file \a sFileName as the new cached data.
  ///
  //// The file content is tokenized first and all #line directives are evaluated, to update the line number and file origin for each token.
  /// Any errors are written to the given log.
  const plTokenizer* Tokenize(const plString& sFileName, plArrayPtr<const plUInt8> fileContent, const plTimestamp& fileTimeStamp, plLogInterface* pLog);

private:
  void SkipWhitespace(plDeque<plToken>& Tokens, plUInt32& uiCurToken);

  mutable plMutex m_Mutex;
  plMap<plString, FileData> m_Cache;
};

/// \brief plPreprocessor implements a standard C preprocessor. It can be used to pre-process files to get the output after macro expansion and #ifdef
/// handling.
///
/// For a detailed documentation about the C preprocessor, see https://gcc.gnu.org/onlinedocs/cpp/
///
/// This class implements all standard features:
///   * object and function macros
///   * Full evaluation of #if, #ifdef etc. including mathematical operations such as #if A > 42
///   * Parameter stringification
///   * Parameter concatenation
///   * __LINE__ and __FILE__ macros
///   * Fully correct #line evaluation for error output
///   * Correct handling of __VA_ARGS__
///   * #include handling
///   * #pragma once
///   * #warning and #error for custom failure messages
class PL_FOUNDATION_DLL plPreprocessor
{
public:
  /// \brief Describes the type of #include that was encountered during preprocessing
  enum IncludeType
  {
    MainFile,        ///< This is used for the very first access to the main source file
    RelativeInclude, ///< An #include "file" has been encountered
    GlobalInclude    ///< An #include <file> has been encountered
  };

  /// \brief This type of callback is used to read an #include file. \a sAbsoluteFile is the path that the FileLocatorCB reported, the result needs
  /// to be stored in \a FileContent.
  using FileOpenCB = plDelegate<plResult(plStringView, plDynamicArray<plUInt8>&, plTimestamp&)>;

  /// \brief This type of callback is used to retrieve the absolute path of the \a sIncludeFile when #included inside \a sCurAbsoluteFile.
  ///
  /// Note that you should ensure that \a out_sAbsoluteFilePath is always identical (including casing and path slashes) when it is supposed to point
  /// to the same file, as this exact name is used for file lookup (and therefore also file caching).
  /// If it is not identical, file caching will not work, and on different OSes the file may be found or not.
  using FileLocatorCB = plDelegate<plResult(plStringView, plStringView, IncludeType, plStringBuilder&)>;

  /// \brief Every time an unknown command (e.g. '#version') is encountered, this callback is used to determine whether the command shall be passed
  /// through.
  ///
  /// If the callback returns false, an error is generated and parsing fails. The callback thus acts as a whitelist for all commands that shall be
  /// passed through.
  using PassThroughUnknownCmdCB = plDelegate<bool(plStringView)>;

  using MacroParameters = plDeque<plTokenParseUtils::TokenStream>;

  /// \brief The event data that the processor broadcasts
  ///
  /// Please note that m_pToken contains a lot of interesting information, such as
  /// the current file and line number and of course the current piece of text.
  struct ProcessingEvent
  {
    /// \brief The event types that the processor broadcasts
    enum EventType
    {
      BeginExpansion,  ///< A macro is now going to be expanded
      EndExpansion,    ///< A macro is finished being expanded
      Error,           ///< An error was encountered
      Warning,         ///< A warning has been output.
      CheckDefined,    ///< A 'defined(X)' is being evaluated
      CheckIfdef,      ///< A '#ifdef X' is being evaluated
      CheckIfndef,     ///< A '#ifndef X' is being evaluated
      EvaluateUnknown, ///< Inside an #if an unknown identifier has been encountered, it will be evaluated as zero
      Define,          ///< A #define X has been stored
      Redefine,        ///< A #define for an already existing macro name (also logged as a warning)
    };

    EventType m_Type = EventType::Error;

    const plToken* m_pToken = nullptr;
    plStringView m_sInfo;
  };

  /// \brief Broadcasts events during the processing. This can be used to create detailed callstacks when an error is encountered.
  /// It also broadcasts errors and warnings with more detailed information than the log interface allows.
  plEvent<const ProcessingEvent&> m_ProcessingEvents;

  plPreprocessor();

  /// \brief All error output is sent to the given plLogInterface.
  ///
  /// Note that when the preprocessor encounters any error, it will stop immediately and usually no output is generated.
  /// However, there are also a few cases where only a warning is generated, in this case preprocessing will continue without problems.
  ///
  /// Additionally errors and warnings are also broadcast through m_ProcessingEvents. So if you want to output more detailed information,
  /// that method should be preferred, because the events carry more information about the current file and line number etc.
  void SetLogInterface(plLogInterface* pLog);

  /// \brief Allows to specify a custom cache object that should be used for storing the tokenized result of files.
  ///
  /// This allows to share one cache across multiple instances of plPreprocessor and across time. E.g. it makes it possible
  /// to prevent having to read and tokenize include files that are referenced often.
  void SetCustomFileCache(plTokenizedFileCache* pFileCache = nullptr);

  /// \brief If set to true, all #pragma commands are passed through to the output, otherwise they are removed.
  void SetPassThroughPragma(bool bPassThrough) { m_bPassThroughPragma = bPassThrough; }

  /// \brief If set to true, all #line commands are passed through to the output, otherwise they are removed.
  void SetPassThroughLine(bool bPassThrough) { m_bPassThroughLine = bPassThrough; }

  /// \brief Sets the callback that is used to determine whether an unknown command is passed through or triggers an error.
  void SetPassThroughUnknownCmdsCB(PassThroughUnknownCmdCB callback) { m_PassThroughUnknownCmdCB = callback; }

  /// \brief Sets the callback that is needed to read input data.
  ///
  /// The default file open function will just try to open files via plFileReader.
  void SetFileOpenFunction(FileOpenCB openAbsFileCB);

  /// \brief Sets the callback that is needed to locate an input file
  ///
  /// The default file locator will assume that the main source file and all files #included in angle brackets can be opened without modification.
  /// Files #included in "" will be appended as relative paths to the path of the file they appeared in.
  void SetFileLocatorFunction(FileLocatorCB locateAbsFileCB);

  /// \brief Adds a #define to the preprocessor, even before any file is processed.
  ///
  /// This allows to have global macros that are always defined for all processed files, such as the current platform etc.
  /// \a sDefinition must be in the form of the text that follows a #define statement. So to define the macro "WIN32", just
  /// pass that string. You can define any macro that could also be defined in the source files.
  ///
  /// If the definition is invalid, PL_FAILURE is returned. Also the preprocessor might end up in an invalid state, so using it any
  /// further might fail (including crashing).
  plResult AddCustomDefine(plStringView sDefinition);

  /// \brief Processes the given file and returns the result as a stream of tokens.
  ///
  /// This function is useful when you want to further process the output afterwards and thus need it in a tokenized form anyway.
  plResult Process(plStringView sMainFile, plTokenParseUtils::TokenStream& ref_tokenOutput);

  /// \brief Processes the given file and returns the result as a string.
  ///
  /// This function creates a string from the tokenized result. If \a bKeepComments is true, all block and line comments
  /// are included in the output string, otherwise they are removed.
  plResult Process(plStringView sMainFile, plStringBuilder& ref_sOutput, bool bKeepComments = true, bool bRemoveRedundantWhitespace = false, bool bInsertLine = false);


private:
  struct FileData
  {
    FileData()
    {
      m_iCurrentLine = 1;
      m_iExpandDepth = 0;
    }

    plHashedString m_sVirtualFileName;
    plHashedString m_sFileName;
    plInt32 m_iCurrentLine;
    plInt32 m_iExpandDepth;
  };

  enum IfDefActivity
  {
    IsActive,
    IsInactive,
    WasActive,
  };

  struct CustomDefine
  {
    plHybridArray<plUInt8, 64> m_Content;
    plTokenizer m_Tokenized;
  };

  // This class-local allocator is used to get rid of some of the memory allocation
  // tracking that would otherwise occur for allocations made by the preprocessor.
  // If changing its position in the class, make sure it always comes before all
  // other members that depend on it to ensure deallocations in those members
  // happen before the allocator get destroyed.
  plAllocatorWithPolicy<plAllocPolicyHeap, plAllocatorTrackingMode::Nothing> m_ClassAllocator;

  bool m_bPassThroughPragma;
  bool m_bPassThroughLine;
  PassThroughUnknownCmdCB m_PassThroughUnknownCmdCB;

  // this file cache is used as long as the user does not provide his own
  plTokenizedFileCache m_InternalFileCache;

  // pointer to the file cache that is in use
  plTokenizedFileCache* m_pUsedFileCache;

  plDeque<FileData> m_CurrentFileStack;

  plLogInterface* m_pLog;

  plDeque<CustomDefine> m_CustomDefines;

  struct IfDefState
  {
    IfDefState(IfDefActivity activeState = IfDefActivity::IsActive)
      : m_ActiveState(activeState)

    {
    }

    IfDefActivity m_ActiveState;
    bool m_bIsInElseClause = false;
  };

  plDeque<IfDefState> m_IfdefActiveStack;

  plResult ProcessFile(plStringView sFile, plTokenParseUtils::TokenStream& TokenOutput);
  plResult ProcessCmd(const plTokenParseUtils::TokenStream& Tokens, plTokenParseUtils::TokenStream& TokenOutput);

public:
  static plResult DefaultFileLocator(plStringView sCurAbsoluteFile, plStringView sIncludeFile, plPreprocessor::IncludeType incType, plStringBuilder& out_sAbsoluteFilePath);
  static plResult DefaultFileOpen(plStringView sAbsoluteFile, plDynamicArray<plUInt8>& ref_fileContent, plTimestamp& out_fileModification);

private: // *** File Handling ***
  plResult OpenFile(plStringView sFile, const plTokenizer** pTokenizer);

  FileOpenCB m_FileOpenCallback;
  FileLocatorCB m_FileLocatorCallback;
  plSet<plTempHashedString> m_PragmaOnce;

private: // *** Macro Definition ***
  bool RemoveDefine(plStringView sName);
  plResult HandleDefine(const plTokenParseUtils::TokenStream& Tokens, plUInt32& uiCurToken);

  struct MacroDefinition
  {
    MacroDefinition();

    const plToken* m_MacroIdentifier;
    bool m_bIsFunction;
    bool m_bCurrentlyExpanding;
    bool m_bHasVarArgs;
    plInt32 m_iNumParameters;
    plTokenParseUtils::TokenStream m_Replacement;
  };

  plResult StoreDefine(const plToken* pMacroNameToken, const plTokenParseUtils::TokenStream* pReplacementTokens, plUInt32 uiFirstReplacementToken, plInt32 iNumParameters, bool bUsesVarArgs);
  plResult ExtractParameterName(const plTokenParseUtils::TokenStream& Tokens, plUInt32& uiCurToken, plString& sIdentifierName);

  plMap<plString256, MacroDefinition> m_Macros;

  static constexpr plInt32 s_iMacroParameter0 = plTokenType::ENUM_COUNT + 2;
  static plString s_ParamNames[32];
  plToken m_ParameterTokens[32];

private: // *** #if condition parsing ***
  plResult EvaluateCondition(const plTokenParseUtils::TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult);
  plResult ParseCondition(const plTokenParseUtils::TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult);
  plResult ParseFactor(const plTokenParseUtils::TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult);
  plResult ParseExpressionMul(const plTokenParseUtils::TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult);
  plResult ParseExpressionOr(const plTokenParseUtils::TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult);
  plResult ParseExpressionAnd(const plTokenParseUtils::TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult);
  plResult ParseExpressionPlus(const plTokenParseUtils::TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult);
  plResult ParseExpressionShift(const plTokenParseUtils::TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult);
  plResult ParseExpressionBitOr(const plTokenParseUtils::TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult);
  plResult ParseExpressionBitAnd(const plTokenParseUtils::TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult);
  plResult ParseExpressionBitXor(const plTokenParseUtils::TokenStream& Tokens, plUInt32& uiCurToken, plInt64& iResult);


private: // *** Parsing ***
  plResult CopyTokensAndEvaluateDefined(const plTokenParseUtils::TokenStream& Source, plUInt32 uiFirstSourceToken, plTokenParseUtils::TokenStream& Destination);
  void CopyTokensReplaceParams(const plTokenParseUtils::TokenStream& Source, plUInt32 uiFirstSourceToken, plTokenParseUtils::TokenStream& Destination, const plHybridArray<plString, 16>& parameters);

  plResult Expect(const plTokenParseUtils::TokenStream& Tokens, plUInt32& uiCurToken, plStringView sToken, plUInt32* pAccepted = nullptr);
  plResult Expect(const plTokenParseUtils::TokenStream& Tokens, plUInt32& uiCurToken, plTokenType::Enum Type, plUInt32* pAccepted = nullptr);
  plResult Expect(const plTokenParseUtils::TokenStream& Tokens, plUInt32& uiCurToken, plStringView sToken1, plStringView sToken2, plUInt32* pAccepted = nullptr);
  plResult ExpectEndOfLine(const plTokenParseUtils::TokenStream& Tokens, plUInt32 uiCurToken);

private: // *** Macro Expansion ***
  plResult Expand(const plTokenParseUtils::TokenStream& Tokens, plTokenParseUtils::TokenStream& Output);
  plResult ExpandOnce(const plTokenParseUtils::TokenStream& Tokens, plTokenParseUtils::TokenStream& Output);
  plResult ExpandObjectMacro(MacroDefinition& Macro, plTokenParseUtils::TokenStream& Output, const plToken* pMacroToken);
  plResult ExpandFunctionMacro(MacroDefinition& Macro, const MacroParameters& Parameters, plTokenParseUtils::TokenStream& Output, const plToken* pMacroToken);
  plResult ExpandMacroParam(const plToken& MacroToken, plUInt32 uiParam, plTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);
  void PassThroughFunctionMacro(MacroDefinition& Macro, const MacroParameters& Parameters, plTokenParseUtils::TokenStream& Output);
  plToken* AddCustomToken(const plToken* pPrevious, const plStringView& sNewText);
  void OutputNotExpandableMacro(MacroDefinition& Macro, plTokenParseUtils::TokenStream& Output);
  plResult ExtractAllMacroParameters(const plTokenParseUtils::TokenStream& Tokens, plUInt32& uiCurToken, plDeque<plTokenParseUtils::TokenStream>& AllParameters);
  plResult ExtractParameterValue(const plTokenParseUtils::TokenStream& Tokens, plUInt32& uiCurToken, plTokenParseUtils::TokenStream& ParamTokens);

  plResult InsertParameters(const plTokenParseUtils::TokenStream& Tokens, plTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);

  plResult InsertStringifiedParameters(const plTokenParseUtils::TokenStream& Tokens, plTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);
  plResult ConcatenateParameters(const plTokenParseUtils::TokenStream& Tokens, plTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);
  void MergeTokens(const plToken* pFirst, const plToken* pSecond, plTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);

  struct CustomToken
  {
    plToken m_Token;
    plString m_sIdentifierString;
  };

  enum TokenFlags : plUInt32
  {
    NoFurtherExpansion = PL_BIT(0),
  };

  plToken m_TokenFile;
  plToken m_TokenLine;
  const plToken* m_pTokenOpenParenthesis;
  const plToken* m_pTokenClosedParenthesis;
  const plToken* m_pTokenComma;

  plDeque<const MacroParameters*> m_MacroParamStack;
  plDeque<const MacroParameters*> m_MacroParamStackExpanded;
  plDeque<CustomToken> m_CustomTokens;

private: // *** Other ***
  static void StringifyTokens(const plTokenParseUtils::TokenStream& Tokens, plStringBuilder& sResult, bool bSurroundWithQuotes);
  plToken* CreateStringifiedParameter(plUInt32 uiParam, const plToken* pParamToken, const MacroDefinition& Macro);

  plResult HandleErrorDirective(const plTokenParseUtils::TokenStream& Tokens, plUInt32 uiCurToken, plUInt32 uiDirectiveToken);
  plResult HandleWarningDirective(const plTokenParseUtils::TokenStream& Tokens, plUInt32 uiCurToken, plUInt32 uiDirectiveToken);
  plResult HandleUndef(const plTokenParseUtils::TokenStream& Tokens, plUInt32 uiCurToken, plUInt32 uiDirectiveToken);

  plResult HandleEndif(const plTokenParseUtils::TokenStream& Tokens, plUInt32 uiCurToken, plUInt32 uiDirectiveToken);
  plResult HandleElif(const plTokenParseUtils::TokenStream& Tokens, plUInt32 uiCurToken, plUInt32 uiDirectiveToken);
  plResult HandleIf(const plTokenParseUtils::TokenStream& Tokens, plUInt32 uiCurToken, plUInt32 uiDirectiveToken);
  plResult HandleElse(const plTokenParseUtils::TokenStream& Tokens, plUInt32 uiCurToken, plUInt32 uiDirectiveToken);
  plResult HandleIfdef(const plTokenParseUtils::TokenStream& Tokens, plUInt32 uiCurToken, plUInt32 uiDirectiveToken, bool bIsIfdef);
  plResult HandleInclude(const plTokenParseUtils::TokenStream& Tokens, plUInt32 uiCurToken, plUInt32 uiDirectiveToken, plTokenParseUtils::TokenStream& TokenOutput);
  plResult HandleLine(const plTokenParseUtils::TokenStream& Tokens, plUInt32 uiCurToken, plUInt32 uiDirectiveToken, plTokenParseUtils::TokenStream& TokenOutput);
};

#define PP_LOG0(Type, FormatStr, ErrorToken)                                                                                                        \
  {                                                                                                                                                 \
    ProcessingEvent pe;                                                                                                                             \
    pe.m_Type = ProcessingEvent::Type;                                                                                                              \
    pe.m_pToken = ErrorToken;                                                                                                                       \
    pe.m_sInfo = FormatStr;                                                                                                                         \
    if (pe.m_pToken->m_uiLine == 0 && pe.m_pToken->m_uiColumn == 0)                                                                                 \
    {                                                                                                                                               \
      const_cast<plToken*>(pe.m_pToken)->m_uiLine = m_CurrentFileStack.PeekBack().m_iCurrentLine;                                                   \
      const_cast<plToken*>(pe.m_pToken)->m_File = m_CurrentFileStack.PeekBack().m_sVirtualFileName;                                                 \
    }                                                                                                                                               \
    m_ProcessingEvents.Broadcast(pe);                                                                                                               \
    plLog::Type(m_pLog, "File '{0}', Line {1} ({2}): " FormatStr, pe.m_pToken->m_File.GetString(), pe.m_pToken->m_uiLine, pe.m_pToken->m_uiColumn); \
  }

#define PP_LOG(Type, FormatStr, ErrorToken, ...)                                                                                                       \
  {                                                                                                                                                    \
    ProcessingEvent _pe;                                                                                                                               \
    _pe.m_Type = ProcessingEvent::Type;                                                                                                                \
    _pe.m_pToken = ErrorToken;                                                                                                                         \
    if (_pe.m_pToken->m_uiLine == 0 && _pe.m_pToken->m_uiColumn == 0)                                                                                  \
    {                                                                                                                                                  \
      const_cast<plToken*>(_pe.m_pToken)->m_uiLine = m_CurrentFileStack.PeekBack().m_iCurrentLine;                                                     \
      const_cast<plToken*>(_pe.m_pToken)->m_File = m_CurrentFileStack.PeekBack().m_sVirtualFileName;                                                   \
    }                                                                                                                                                  \
    plStringBuilder sInfo;                                                                                                                             \
    sInfo.SetFormat(FormatStr, ##__VA_ARGS__);                                                                                                            \
    _pe.m_sInfo = sInfo;                                                                                                                               \
    m_ProcessingEvents.Broadcast(_pe);                                                                                                                 \
    plLog::Type(m_pLog, "File '{0}', Line {1} ({2}): {3}", _pe.m_pToken->m_File.GetString(), _pe.m_pToken->m_uiLine, _pe.m_pToken->m_uiColumn, sInfo); \
  }
