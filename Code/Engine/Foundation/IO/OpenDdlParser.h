#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/Stream.h>

class plLogInterface;

/// \brief The primitive data types that OpenDDL supports
enum class plOpenDdlPrimitiveType
{
  Bool,
  Int8,
  Int16,
  Int32,
  Int64,
  UInt8,
  UInt16,
  UInt32,
  UInt64,
  // Half, // Currently not supported
  Float,
  Double,
  String,
  // Ref, // Currently not supported
  // Type // Currently not supported
  Custom
};

/// \brief A low level parser for the OpenDDL format. It can incrementally parse the structure, individual blocks can be skipped.
///
/// The document structure is returned through virtual functions that need to be overridden.
class PL_FOUNDATION_DLL plOpenDdlParser
{
public:
  plOpenDdlParser();
  virtual ~plOpenDdlParser() = default;

  /// \brief Whether an error occurred during parsing that resulted in cancellation of further parsing.
  bool HadFatalParsingError() const { return m_bHadFatalParsingError; } // [tested]

protected:
  /// \brief Sets an plLogInterface through which errors and warnings are reported.
  void SetLogInterface(plLogInterface* pLog) { m_pLogInterface = pLog; }

  /// \brief Data is returned in larger chunks, to reduce the number of function calls. The cache size determines the maximum chunk size per primitive
  /// type.
  ///
  /// Default cache size is 4 KB. That means up to 1000 integers may be returned in one chunk (or 500 doubles).
  /// It does not help to increase the chunk size, when the input data doesn't use such large data lists.
  void SetCacheSize(plUInt32 uiSizeInKB);

  /// \brief Configures the parser to read from the given stream. This can only be called once on a parser instance.
  void SetInputStream(plStreamReader& stream, plUInt32 uiFirstLineOffset = 0); // [tested]

  /// \brief Call this to parse the next piece of the document. This may trigger a callback through which data is returned.
  ///
  /// This function returns false when the end of the document has been reached, or a fatal parsing error has been reported.
  bool ContinueParsing(); // [tested]

  /// \brief Calls ContinueParsing() in a loop until that returns false.
  plResult ParseAll(); // [tested]

  /// \brief Skips the rest of the currently open object. No OnEndObject() call will be done for this object either.
  void SkipRestOfObject();

  /// \brief Can be used to prevent parsing the rest of the document.
  void StopParsing();

  /// \brief Outputs that a parsing error was detected (via OnParsingError) and stops further parsing, if bFatal is set to true.
  void ParsingError(plStringView sMessage, bool bFatal);

  plLogInterface* m_pLogInterface;

protected:
  /// \brief Called when something unexpected is encountered in the document.
  ///
  /// The error message describes what was expected and what was encountered.
  /// If bFatal is true, the error has left the parser in an unrecoverable state and thus it will not continue parsing.
  /// In that case client code will need to clean up it's open state, as no further callbacks will be called.
  /// If bFatal is false, the document is not entirely valid, but the parser is still able to continue.
  virtual void OnParsingError(plStringView sMessage, bool bFatal, plUInt32 uiLine, plUInt32 uiColumn) {}

  /// \brief Called when a new object is encountered.
  virtual void OnBeginObject(plStringView sType, plStringView sName, bool bGlobalName) = 0;

  /// \brief Called when the end of an object is encountered.
  virtual void OnEndObject() = 0;

  /// \brief Called when a new primitive object is encountered.
  virtual void OnBeginPrimitiveList(plOpenDdlPrimitiveType type, plStringView sName, bool bGlobalName) = 0;

  /// \brief Called when the end of a primitive object is encountered.
  virtual void OnEndPrimitiveList() = 0;

  /// \todo Currently not supported
  // virtual void OnBeginPrimitiveArrayList(plOpenDdlPrimitiveType type, plUInt32 uiGroupSize) = 0;
  // virtual void OnEndPrimitiveArrayList() = 0;

  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveBool(plUInt32 count, const bool* pData, bool bThisIsAll) = 0;

  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveInt8(plUInt32 count, const plInt8* pData, bool bThisIsAll) = 0;
  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveInt16(plUInt32 count, const plInt16* pData, bool bThisIsAll) = 0;
  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveInt32(plUInt32 count, const plInt32* pData, bool bThisIsAll) = 0;
  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveInt64(plUInt32 count, const plInt64* pData, bool bThisIsAll) = 0;

  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveUInt8(plUInt32 count, const plUInt8* pData, bool bThisIsAll) = 0;
  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveUInt16(plUInt32 count, const plUInt16* pData, bool bThisIsAll) = 0;
  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveUInt32(plUInt32 count, const plUInt32* pData, bool bThisIsAll) = 0;
  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveUInt64(plUInt32 count, const plUInt64* pData, bool bThisIsAll) = 0;

  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveFloat(plUInt32 count, const float* pData, bool bThisIsAll) = 0;
  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveDouble(plUInt32 count, const double* pData, bool bThisIsAll) = 0;

  /// \brief Called when data for a primitive type is available. More than one value may be reported at a time.
  virtual void OnPrimitiveString(plUInt32 count, const plStringView* pData, bool bThisIsAll) = 0;

private:
  enum State
  {
    Finished,
    Idle,
    ReadingBool,
    ReadingInt8,
    ReadingInt16,
    ReadingInt32,
    ReadingInt64,
    ReadingUInt8,
    ReadingUInt16,
    ReadingUInt32,
    ReadingUInt64,
    ReadingFloat,
    ReadingDouble,
    ReadingString,
  };

  struct DdlState
  {
    DdlState()
      : m_State(Idle)
    {
    }
    DdlState(State s)
      : m_State(s)
    {
    }

    State m_State;
  };

  void ReadNextByte();
  bool ReadCharacter();
  bool ReadCharacterSkipComments();
  void SkipWhitespace();
  void ContinueIdle();
  void ReadIdentifier(plUInt8* szString, plUInt32& count);
  void ReadString();
  void ReadWord();
  plUInt64 ReadDecimalLiteral();
  void PurgeCachedPrimitives(bool bThisIsAll);
  bool ContinuePrimitiveList();
  void ContinueString();
  void SkipString();
  void ContinueBool();
  void ContinueInt();
  void ContinueFloat();

  void ReadDecimalFloat();
  void ReadHexString();

  plHybridArray<DdlState, 32> m_StateStack;
  plStreamReader* m_pInput;
  plDynamicArray<plUInt8> m_Cache;

  static constexpr plUInt32 s_uiMaxIdentifierLength = 64;

  plUInt8 m_uiCurByte;
  plUInt8 m_uiNextByte;
  plUInt32 m_uiCurLine;
  plUInt32 m_uiCurColumn;
  bool m_bSkippingMode;
  bool m_bHadFatalParsingError;
  plUInt8 m_szIdentifierType[s_uiMaxIdentifierLength];
  plUInt8 m_szIdentifierName[s_uiMaxIdentifierLength];
  plDynamicArray<plUInt8> m_TempString;
  plUInt32 m_uiTempStringLength;

  plUInt32 m_uiNumCachedPrimitives;
  bool* m_pBoolCache;
  plInt8* m_pInt8Cache;
  plInt16* m_pInt16Cache;
  plInt32* m_pInt32Cache;
  plInt64* m_pInt64Cache;
  plUInt8* m_pUInt8Cache;
  plUInt16* m_pUInt16Cache;
  plUInt32* m_pUInt32Cache;
  plUInt64* m_pUInt64Cache;
  float* m_pFloatCache;
  double* m_pDoubleCache;
};
