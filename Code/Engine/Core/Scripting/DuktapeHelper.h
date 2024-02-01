#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/String.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

struct duk_hthread;
using duk_context = duk_hthread;
using duk_c_function = int (*)(duk_context*);

struct plDuktapeTypeMask
{
  using StorageType = plUInt32;

  enum Enum
  {
    None = PL_BIT(0),      ///< no value, e.g. invalid index
    Undefined = PL_BIT(1), ///< ECMAScript undefined
    Null = PL_BIT(2),      ///< ECMAScript null
    Bool = PL_BIT(3),      ///< boolean, true or false
    Number = PL_BIT(4),    ///< any number, stored as a double
    String = PL_BIT(5),    ///< ECMAScript string: CESU-8 / extended UTF-8 encoded
    Object = PL_BIT(6),    ///< ECMAScript object: includes objects, arrays, functions, threads
    Buffer = PL_BIT(7),    ///< fixed or dynamic, garbage collected byte buffer
    Pointer = PL_BIT(8)    ///< raw void pointer

  };

  struct Bits
  {
    StorageType None : 1;
    StorageType Undefined : 1;
    StorageType Null : 1;
    StorageType Bool : 1;
    StorageType Number : 1;
    StorageType String : 1;
    StorageType Object : 1;
  };
};

PL_DECLARE_FLAGS_OPERATORS(plDuktapeTypeMask);

#  if PL_ENABLED(PL_COMPILE_FOR_DEBUG)

#    define PL_DUK_VERIFY_STACK(duk, ExpectedStackChange) \
      duk.EnableStackChangeVerification();                \
      duk.VerifyExpectedStackChange(ExpectedStackChange, PL_SOURCE_FILE, PL_SOURCE_LINE, PL_SOURCE_FUNCTION);

#    define PL_DUK_RETURN_AND_VERIFY_STACK(duk, ReturnCode, ExpectedStackChange) \
      {                                                                          \
        auto ret = ReturnCode;                                                   \
        PL_DUK_VERIFY_STACK(duk, ExpectedStackChange);                           \
        return ret;                                                              \
      }

#    define PL_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, ExpectedStackChange) \
      PL_DUK_VERIFY_STACK(duk, ExpectedStackChange);                      \
      return;


#  else

#    define PL_DUK_VERIFY_STACK(duk, ExpectedStackChange)

#    define PL_DUK_RETURN_AND_VERIFY_STACK(duk, ReturnCode, ExpectedStackChange) return ReturnCode;

#    define PL_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, ExpectedStackChange) return;

#  endif

class PL_CORE_DLL plDuktapeHelper
{
public:
  plDuktapeHelper(duk_context* pContext);
  plDuktapeHelper(const plDuktapeHelper& rhs);
  ~plDuktapeHelper();
  void operator=(const plDuktapeHelper& rhs);

  /// \name Basics
  ///@{

  /// \brief Returns the raw Duktape context for custom operations.
  PL_ALWAYS_INLINE duk_context* GetContext() const { return m_pContext; }

  /// \brief Implicit conversion to duk_context*
  PL_ALWAYS_INLINE operator duk_context*() const { return m_pContext; }

#  if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  void VerifyExpectedStackChange(plInt32 iExpectedStackChange, const char* szFile, plUInt32 uiLine, const char* szFunction) const;
#  endif

  ///@}
  /// \name Error Handling
  ///@{

  void Error(const plFormatString& text);

  void LogStackTrace(plInt32 iErrorObjIdx);


  ///@}
  /// \name Objects / Stash
  ///@{

  void PopStack(plUInt32 n = 1);

  void PushGlobalObject();

  void PushGlobalStash();

  plResult PushLocalObject(const char* szName, plInt32 iParentObjectIndex = -1);

  ///@}
  /// \name Object Properties
  ///@{

  bool HasProperty(const char* szPropertyName, plInt32 iParentObjectIndex = -1) const;

  bool GetBoolProperty(const char* szPropertyName, bool bFallback, plInt32 iParentObjectIndex = -1) const;
  plInt32 GetIntProperty(const char* szPropertyName, plInt32 iFallback, plInt32 iParentObjectIndex = -1) const;
  plUInt32 GetUIntProperty(const char* szPropertyName, plUInt32 uiFallback, plInt32 iParentObjectIndex = -1) const;
  float GetFloatProperty(const char* szPropertyName, float fFallback, plInt32 iParentObjectIndex = -1) const;
  double GetNumberProperty(const char* szPropertyName, double fFallback, plInt32 iParentObjectIndex = -1) const;
  const char* GetStringProperty(const char* szPropertyName, const char* szFallback, plInt32 iParentObjectIndex = -1) const;

  void SetBoolProperty(const char* szPropertyName, bool value, plInt32 iParentObjectIndex = -1) const;
  void SetNumberProperty(const char* szPropertyName, double value, plInt32 iParentObjectIndex = -1) const;
  void SetStringProperty(const char* szPropertyName, const char* value, plInt32 iParentObjectIndex = -1) const;

  /// \note If a negative parent index is given, the parent object taken is actually ParentIdx - 1 (obj at idx -1 is the custom object to use)
  void SetCustomProperty(const char* szPropertyName, plInt32 iParentObjectIndex = -1) const;


  ///@}
  /// \name Global State
  ///@{

  void StorePointerInStash(const char* szKey, void* pPointer);
  void* RetrievePointerFromStash(const char* szKey) const;

  void StoreStringInStash(const char* szKey, const char* value);
  const char* RetrieveStringFromStash(const char* szKey, const char* szFallback = nullptr) const;

  ///@}
  /// \name Type Checks
  ///@{

  bool IsOfType(plBitflags<plDuktapeTypeMask> mask, plInt32 iStackElement = -1) const;
  bool IsBool(plInt32 iStackElement = -1) const;
  bool IsNumber(plInt32 iStackElement = -1) const;
  bool IsString(plInt32 iStackElement = -1) const;
  bool IsNull(plInt32 iStackElement = -1) const;
  bool IsUndefined(plInt32 iStackElement = -1) const;
  bool IsObject(plInt32 iStackElement = -1) const;
  bool IsBuffer(plInt32 iStackElement = -1) const;
  bool IsPointer(plInt32 iStackElement = -1) const;
  bool IsNullOrUndefined(plInt32 iStackElement = -1) const;

  ///@}
  /// \name C Functions
  ///@{

  void RegisterGlobalFunction(const char* szFunctionName, duk_c_function function, plUInt8 uiNumArguments, plInt16 iMagicValue = 0);
  void RegisterGlobalFunctionWithVarArgs(const char* szFunctionName, duk_c_function function, plInt16 iMagicValue = 0);

  void RegisterObjectFunction(
    const char* szFunctionName, duk_c_function function, plUInt8 uiNumArguments, plInt32 iParentObjectIndex = -1, plInt16 iMagicValue = 0);

  plResult PrepareGlobalFunctionCall(const char* szFunctionName);
  plResult PrepareObjectFunctionCall(const char* szFunctionName, plInt32 iParentObjectIndex = -1);
  plResult CallPreparedFunction();

  plResult PrepareMethodCall(const char* szMethodName, plInt32 iParentObjectIndex = -1);
  plResult CallPreparedMethod();


  ///@}
  /// \name Values / Parameters
  ///@{

  void PushInt(plInt32 iParam);
  void PushUInt(plUInt32 uiParam);
  void PushBool(bool bParam);
  void PushNumber(double fParam);
  void PushString(const plStringView& sParam);
  void PushNull();
  void PushUndefined();
  void PushCustom(plUInt32 uiNum = 1);

  bool GetBoolValue(plInt32 iStackElement, bool bFallback = false) const;
  plInt32 GetIntValue(plInt32 iStackElement, plInt32 iFallback = 0) const;
  plUInt32 GetUIntValue(plInt32 iStackElement, plUInt32 uiFallback = 0) const;
  float GetFloatValue(plInt32 iStackElement, float fFallback = 0) const;
  double GetNumberValue(plInt32 iStackElement, double fFallback = 0) const;
  const char* GetStringValue(plInt32 iStackElement, const char* szFallback = "") const;

  ///@}
  /// \name Executing Scripts
  ///@{

  plResult ExecuteString(const char* szString, const char* szDebugName = "eval");

  plResult ExecuteStream(plStreamReader& inout_stream, const char* szDebugName);

  plResult ExecuteFile(const char* szFile);

  ///@}

public:
#  if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  void EnableStackChangeVerification() const;
#  endif


protected:
  duk_context* m_pContext = nullptr;
  plInt32 m_iPushedValues = 0;

#  if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  plInt32 m_iStackTopAtStart = -1000;
  mutable bool m_bVerifyStackChange = false;

#  endif
};

#endif
