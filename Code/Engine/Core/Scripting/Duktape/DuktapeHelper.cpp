#include <Core/CorePCH.h>

#include <Core/Scripting/DuktapeHelper.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Duktape/duktape.h>
#  include <Foundation/IO/FileSystem/FileReader.h>

PLASMA_CHECK_AT_COMPILETIME(plDuktapeTypeMask::None == DUK_TYPE_MASK_NONE);
PLASMA_CHECK_AT_COMPILETIME(plDuktapeTypeMask::Undefined == DUK_TYPE_MASK_UNDEFINED);
PLASMA_CHECK_AT_COMPILETIME(plDuktapeTypeMask::Null == DUK_TYPE_MASK_NULL);
PLASMA_CHECK_AT_COMPILETIME(plDuktapeTypeMask::Bool == DUK_TYPE_MASK_BOOLEAN);
PLASMA_CHECK_AT_COMPILETIME(plDuktapeTypeMask::Number == DUK_TYPE_MASK_NUMBER);
PLASMA_CHECK_AT_COMPILETIME(plDuktapeTypeMask::String == DUK_TYPE_MASK_STRING);
PLASMA_CHECK_AT_COMPILETIME(plDuktapeTypeMask::Object == DUK_TYPE_MASK_OBJECT);
PLASMA_CHECK_AT_COMPILETIME(plDuktapeTypeMask::Buffer == DUK_TYPE_MASK_BUFFER);
PLASMA_CHECK_AT_COMPILETIME(plDuktapeTypeMask::Pointer == DUK_TYPE_MASK_POINTER);

#  if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)


void plDuktapeHelper::EnableStackChangeVerification() const
{
  m_bVerifyStackChange = true;
}

#  endif

plDuktapeHelper::plDuktapeHelper(duk_context* pContext)
  : m_pContext(pContext)
{
#  if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  if (m_pContext)
  {
    m_bVerifyStackChange = false;
    m_iStackTopAtStart = duk_get_top(m_pContext);
  }
#  endif
}

plDuktapeHelper::plDuktapeHelper(const plDuktapeHelper& rhs)
  : plDuktapeHelper(rhs.GetContext())
{
}

plDuktapeHelper::~plDuktapeHelper() = default;

void plDuktapeHelper::operator=(const plDuktapeHelper& rhs)
{
  if (this == &rhs)
    return;

  *this = plDuktapeHelper(rhs.GetContext());
}

#  if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
void plDuktapeHelper::VerifyExpectedStackChange(plInt32 iExpectedStackChange, const char* szFile, plUInt32 uiLine, const char* szFunction) const
{
  if (m_bVerifyStackChange && m_pContext)
  {
    const plInt32 iCurTop = duk_get_top(m_pContext);
    const plInt32 iStackChange = iCurTop - m_iStackTopAtStart;

    if (iStackChange != iExpectedStackChange)
    {
      plLog::Error("{}:{} ({}): Stack change {} != {}", szFile, uiLine, szFunction, iStackChange, iExpectedStackChange);
    }
  }
}
#  endif

void plDuktapeHelper::Error(const plFormatString& text)
{
  plStringBuilder tmp;
  duk_error(m_pContext, DUK_ERR_ERROR, text.GetTextCStr(tmp));
}

void plDuktapeHelper::LogStackTrace(plInt32 iErrorObjIdx)
{
  if (duk_is_error(m_pContext, iErrorObjIdx))
  {
    PLASMA_LOG_BLOCK("Stack Trace");

    duk_get_prop_string(m_pContext, iErrorObjIdx, "stack");

    const plStringBuilder stack = duk_safe_to_string(m_pContext, iErrorObjIdx);
    plHybridArray<plStringView, 32> lines;
    stack.Split(false, lines, "\n", "\r");

    for (plStringView line : lines)
    {
      plLog::Dev("{}", line);
    }

    duk_pop(m_pContext);
  }
}

void plDuktapeHelper::PopStack(plUInt32 n /*= 1*/)
{
  duk_pop_n(m_pContext, n);
}

void plDuktapeHelper::PushGlobalObject()
{
  duk_push_global_object(m_pContext); // [ global ]
}

void plDuktapeHelper::PushGlobalStash()
{
  duk_push_global_stash(m_pContext); // [ stash ]
}

plResult plDuktapeHelper::PushLocalObject(const char* szName, plInt32 iParentObjectIndex /* = -1*/)
{
  duk_require_top_index(m_pContext);

  if (duk_get_prop_string(m_pContext, iParentObjectIndex, szName) == false) // [ obj/undef ]
  {
    duk_pop(m_pContext); // [ ]
    return PLASMA_FAILURE;
  }

  // [ object ]
  return PLASMA_SUCCESS;
}

bool plDuktapeHelper::HasProperty(const char* szPropertyName, plInt32 iParentObjectIndex /*= -1*/) const
{
  return duk_is_object(m_pContext, iParentObjectIndex) && duk_has_prop_string(m_pContext, iParentObjectIndex, szPropertyName);
}

bool plDuktapeHelper::GetBoolProperty(const char* szPropertyName, bool bFallback, plInt32 iParentObjectIndex /*= -1*/) const
{
  bool result = bFallback;

  if (duk_is_object(m_pContext, iParentObjectIndex))
  {
    if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
    {
      result = duk_get_boolean_default(m_pContext, -1, bFallback); // [ value ]
    }

    duk_pop(m_pContext); // [ ]
  }

  return result;
}

plInt32 plDuktapeHelper::GetIntProperty(const char* szPropertyName, plInt32 iFallback, plInt32 iParentObjectIndex /*= -1*/) const
{
  plInt32 result = iFallback;

  if (duk_is_object(m_pContext, iParentObjectIndex))
  {
    if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
    {
      result = duk_get_int_default(m_pContext, -1, iFallback); // [ value ]
    }

    duk_pop(m_pContext); // [ ]
  }

  return result;
}

plUInt32 plDuktapeHelper::GetUIntProperty(const char* szPropertyName, plUInt32 uiFallback, plInt32 iParentObjectIndex /*= -1*/) const
{
  plUInt32 result = uiFallback;

  if (duk_is_object(m_pContext, iParentObjectIndex))
  {
    if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
    {
      result = duk_get_uint_default(m_pContext, -1, uiFallback); // [ value ]
    }

    duk_pop(m_pContext); // [ ]
  }

  return result;
}

float plDuktapeHelper::GetFloatProperty(const char* szPropertyName, float fFallback, plInt32 iParentObjectIndex /*= -1*/) const
{
  return static_cast<float>(GetNumberProperty(szPropertyName, fFallback, iParentObjectIndex));
}

double plDuktapeHelper::GetNumberProperty(const char* szPropertyName, double fFallback, plInt32 iParentObjectIndex /*= -1*/) const
{
  double result = fFallback;

  if (duk_is_object(m_pContext, iParentObjectIndex))
  {
    if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
    {
      result = duk_get_number_default(m_pContext, -1, fFallback); // [ value ]
    }

    duk_pop(m_pContext); // [ ]
  }

  return result;
}

const char* plDuktapeHelper::GetStringProperty(const char* szPropertyName, const char* szFallback, plInt32 iParentObjectIndex /*= -1*/) const
{
  const char* result = szFallback;

  if (duk_is_object(m_pContext, iParentObjectIndex))
  {
    if (duk_get_prop_string(m_pContext, iParentObjectIndex, szPropertyName)) // [ value/undef ]
    {
      result = duk_get_string_default(m_pContext, -1, szFallback); // [ value ]
    }

    duk_pop(m_pContext); // [ ]
  }

  return result;
}

void plDuktapeHelper::SetBoolProperty(const char* szPropertyName, bool value, plInt32 iParentObjectIndex /*= -1*/) const
{
  plDuktapeHelper duk(m_pContext);

  duk_push_boolean(m_pContext, value); // [ value ]

  if (iParentObjectIndex >= 0)
    duk_put_prop_string(m_pContext, iParentObjectIndex, szPropertyName); // [ ]
  else
    duk_put_prop_string(m_pContext, iParentObjectIndex - 1, szPropertyName); // [ ]

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void plDuktapeHelper::SetNumberProperty(const char* szPropertyName, double value, plInt32 iParentObjectIndex /*= -1*/) const
{
  plDuktapeHelper duk(m_pContext);

  duk_push_number(m_pContext, value); // [ value ]

  if (iParentObjectIndex >= 0)
    duk_put_prop_string(m_pContext, iParentObjectIndex, szPropertyName); // [ ]
  else
    duk_put_prop_string(m_pContext, iParentObjectIndex - 1, szPropertyName); // [ ]

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void plDuktapeHelper::SetStringProperty(const char* szPropertyName, const char* value, plInt32 iParentObjectIndex /*= -1*/) const
{
  plDuktapeHelper duk(m_pContext);

  duk_push_string(m_pContext, value); // [ value ]

  if (iParentObjectIndex >= 0)
    duk_put_prop_string(m_pContext, iParentObjectIndex, szPropertyName); // [ ]
  else
    duk_put_prop_string(m_pContext, iParentObjectIndex - 1, szPropertyName); // [ ]

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void plDuktapeHelper::SetCustomProperty(const char* szPropertyName, plInt32 iParentObjectIndex /*= -1*/) const
{
  plDuktapeHelper duk(m_pContext); // [ value ]

  if (iParentObjectIndex >= 0)
    duk_put_prop_string(m_pContext, iParentObjectIndex, szPropertyName); // [ ]
  else
    duk_put_prop_string(m_pContext, iParentObjectIndex - 1, szPropertyName); // [ ]

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, -1);
}

void plDuktapeHelper::StorePointerInStash(const char* szKey, void* pPointer)
{
  duk_push_global_stash(m_pContext);                                                      // [ stash ]
  *reinterpret_cast<void**>(duk_push_fixed_buffer(m_pContext, sizeof(void*))) = pPointer; // [ stash buffer ]
  duk_put_prop_string(m_pContext, -2, szKey);                                             // [ stash ]
  duk_pop(m_pContext);                                                                    // [ ]
}

void* plDuktapeHelper::RetrievePointerFromStash(const char* szKey) const
{
  void* pPointer = nullptr;

  duk_push_global_stash(m_pContext); // [ stash ]

  if (duk_get_prop_string(m_pContext, -1, szKey)) // [ stash obj/undef ]
  {
    PLASMA_ASSERT_DEBUG(duk_is_buffer(m_pContext, -1), "Object '{}' in stash is not a buffer", szKey);

    pPointer = *reinterpret_cast<void**>(duk_get_buffer(m_pContext, -1, nullptr)); // [ stash obj/undef ]
  }

  duk_pop_2(m_pContext); // [ ]

  return pPointer;
}

void plDuktapeHelper::StoreStringInStash(const char* szKey, const char* value)
{
  duk_push_global_stash(m_pContext);          // [ stash ]
  duk_push_string(m_pContext, value);         // [ stash value ]
  duk_put_prop_string(m_pContext, -2, szKey); // [ stash ]
  duk_pop(m_pContext);                        // [ ]
}

const char* plDuktapeHelper::RetrieveStringFromStash(const char* szKey, const char* szFallback /*= nullptr*/) const
{
  duk_push_global_stash(m_pContext); // [ stash ]

  if (!duk_get_prop_string(m_pContext, -1, szKey)) // [ stash string/undef ]
  {
    duk_pop_2(m_pContext); // [ ]
    return szFallback;
  }

  szFallback = duk_get_string_default(m_pContext, -1, szFallback); // [ stash string ]
  duk_pop_2(m_pContext);                                           // [ ]

  return szFallback;
}

bool plDuktapeHelper::IsOfType(plBitflags<plDuktapeTypeMask> mask, plInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, mask.GetValue());
}

bool plDuktapeHelper::IsBool(plInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_BOOLEAN);
}

bool plDuktapeHelper::IsNumber(plInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_NUMBER);
}

bool plDuktapeHelper::IsString(plInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_STRING);
}

bool plDuktapeHelper::IsNull(plInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_NULL);
}

bool plDuktapeHelper::IsUndefined(plInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_UNDEFINED);
}

bool plDuktapeHelper::IsObject(plInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_OBJECT);
}

bool plDuktapeHelper::IsBuffer(plInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_BUFFER);
}

bool plDuktapeHelper::IsPointer(plInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_POINTER);
}

bool plDuktapeHelper::IsNullOrUndefined(plInt32 iStackElement /*= -1*/) const
{
  return duk_check_type_mask(m_pContext, iStackElement, DUK_TYPE_MASK_NULL | DUK_TYPE_MASK_UNDEFINED);
}

void plDuktapeHelper::RegisterGlobalFunction(
  const char* szFunctionName, duk_c_function function, plUInt8 uiNumArguments, plInt16 iMagicValue /*= 0*/)
{
  // TODO: could store iFuncIdx for faster function calls

  duk_push_global_object(m_pContext);                                                 // [ global ]
  /*const int iFuncIdx =*/duk_push_c_function(m_pContext, function, uiNumArguments);  // [ global func ]
  duk_set_magic(m_pContext, -1, iMagicValue);                                         // [ global func ]
  duk_put_prop_string(m_pContext, -2, szFunctionName);                                // [ global ]
  duk_pop(m_pContext);                                                                // [ ]
}

void plDuktapeHelper::RegisterGlobalFunctionWithVarArgs(const char* szFunctionName, duk_c_function function, plInt16 iMagicValue /*= 0*/)
{
  // TODO: could store iFuncIdx for faster function calls

  duk_push_global_object(m_pContext);                                              // [ global ]
  /*const int iFuncIdx =*/duk_push_c_function(m_pContext, function, DUK_VARARGS);  // [ global func ]
  duk_set_magic(m_pContext, -1, iMagicValue);                                      // [ global func ]
  duk_put_prop_string(m_pContext, -2, szFunctionName);                             // [ global ]
  duk_pop(m_pContext);                                                             // [ ]
}

void plDuktapeHelper::RegisterObjectFunction(
  const char* szFunctionName, duk_c_function function, plUInt8 uiNumArguments, plInt32 iParentObjectIndex /*= -1*/, plInt16 iMagicValue /*= 0*/)
{
  /*const int iFuncIdx =*/duk_push_c_function(m_pContext, function, uiNumArguments);  // [ func ]
  duk_set_magic(m_pContext, -1, iMagicValue);                                         // [ func ]

  if (iParentObjectIndex < 0)
  {
    duk_put_prop_string(m_pContext, iParentObjectIndex - 1, szFunctionName); // [ ]
  }
  else
  {
    duk_put_prop_string(m_pContext, iParentObjectIndex, szFunctionName); // [ ]
  }
}

plResult plDuktapeHelper::PrepareGlobalFunctionCall(const char* szFunctionName)
{
  if (!duk_get_global_string(m_pContext, szFunctionName)) // [ func/undef ]
    goto failure;

  if (!duk_is_function(m_pContext, -1)) // [ func ]
    goto failure;

  m_iPushedValues = 0;
  return PLASMA_SUCCESS; // [ func ]

failure:
  duk_pop(m_pContext); // [ ]
  return PLASMA_FAILURE;
}

plResult plDuktapeHelper::PrepareObjectFunctionCall(const char* szFunctionName, plInt32 iParentObjectIndex /*= -1*/)
{
  duk_require_top_index(m_pContext);

  if (!duk_get_prop_string(m_pContext, iParentObjectIndex, szFunctionName)) // [ func/undef ]
    goto failure;

  if (!duk_is_function(m_pContext, -1)) // [ func ]
    goto failure;

  m_iPushedValues = 0;
  return PLASMA_SUCCESS; // [ func ]

failure:
  duk_pop(m_pContext); // [ ]
  return PLASMA_FAILURE;
}

plResult plDuktapeHelper::CallPreparedFunction()
{
  if (duk_pcall(m_pContext, m_iPushedValues) == DUK_EXEC_SUCCESS) // [ func n-args ] -> [ result/error ]
  {
    return PLASMA_SUCCESS; // [ result ]
  }
  else
  {
    plLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1));

    LogStackTrace(-1);

    return PLASMA_FAILURE; // [ error ]
  }
}

plResult plDuktapeHelper::PrepareMethodCall(const char* szMethodName, plInt32 iParentObjectIndex /*= -1*/)
{
  if (!duk_get_prop_string(m_pContext, iParentObjectIndex, szMethodName)) // [ func/undef ]
    goto failure;

  if (!duk_is_function(m_pContext, -1)) // [ func ]
    goto failure;

  if (iParentObjectIndex < 0)
  {
    duk_dup(m_pContext, iParentObjectIndex - 1); // [ func this ]
  }
  else
  {
    duk_dup(m_pContext, iParentObjectIndex); // [ func this ]
  }

  m_iPushedValues = 0;
  return PLASMA_SUCCESS; // [ func this ]

failure:
  duk_pop(m_pContext); // [ ]
  return PLASMA_FAILURE;
}

plResult plDuktapeHelper::CallPreparedMethod()
{
  if (duk_pcall_method(m_pContext, m_iPushedValues) == DUK_EXEC_SUCCESS) // [ func this n-args ] -> [ result/error ]
  {
    return PLASMA_SUCCESS; // [ result ]
  }
  else
  {
    plLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1));

    LogStackTrace(-1);

    return PLASMA_FAILURE; // [ error ]
  }
}

void plDuktapeHelper::PushInt(plInt32 iParam)
{
  duk_push_int(m_pContext, iParam); // [ value ]
  ++m_iPushedValues;
}

void plDuktapeHelper::PushUInt(plUInt32 uiParam)
{
  duk_push_uint(m_pContext, uiParam); // [ value ]
  ++m_iPushedValues;
}

void plDuktapeHelper::PushBool(bool bParam)
{
  duk_push_boolean(m_pContext, bParam); // [ value ]
  ++m_iPushedValues;
}

void plDuktapeHelper::PushNumber(double fParam)
{
  duk_push_number(m_pContext, fParam); // [ value ]
  ++m_iPushedValues;
}

void plDuktapeHelper::PushString(const plStringView& sParam)
{
  duk_push_lstring(m_pContext, sParam.GetStartPointer(), sParam.GetElementCount()); // [ value ]
  ++m_iPushedValues;
}

void plDuktapeHelper::PushNull()
{
  duk_push_null(m_pContext); // [ null ]
  ++m_iPushedValues;
}

void plDuktapeHelper::PushUndefined()
{
  duk_push_undefined(m_pContext); // [ undefined ]
  ++m_iPushedValues;
}

void plDuktapeHelper::PushCustom(plUInt32 uiNum)
{
  m_iPushedValues += uiNum;
}

bool plDuktapeHelper::GetBoolValue(plInt32 iStackElement, bool bFallback /*= false*/) const
{
  return duk_get_boolean_default(m_pContext, iStackElement, bFallback);
}

plInt32 plDuktapeHelper::GetIntValue(plInt32 iStackElement, plInt32 iFallback /*= 0*/) const
{
  return duk_get_int_default(m_pContext, iStackElement, iFallback);
}

plUInt32 plDuktapeHelper::GetUIntValue(plInt32 iStackElement, plUInt32 uiFallback /*= 0*/) const
{
  return duk_get_uint_default(m_pContext, iStackElement, uiFallback);
}

float plDuktapeHelper::GetFloatValue(plInt32 iStackElement, float fFallback /*= 0*/) const
{
  return static_cast<float>(duk_get_number_default(m_pContext, iStackElement, fFallback));
}

double plDuktapeHelper::GetNumberValue(plInt32 iStackElement, double fFallback /*= 0*/) const
{
  return duk_get_number_default(m_pContext, iStackElement, fFallback);
}

const char* plDuktapeHelper::GetStringValue(plInt32 iStackElement, const char* szFallback /*= ""*/) const
{
  return duk_get_string_default(m_pContext, iStackElement, szFallback);
}

plResult plDuktapeHelper::ExecuteString(const char* szString, const char* szDebugName /*= "eval"*/)
{
  duk_push_string(m_pContext, szDebugName);                       // [ filename ]
  if (duk_pcompile_string_filename(m_pContext, 0, szString) != 0) // [ function/error ]
  {
    PLASMA_LOG_BLOCK("DukTape::ExecuteString", "Compilation failed");

    plLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1)); // [ error ]

    LogStackTrace(-1);

    // TODO: print out line by line
    plLog::Info("[duktape]Source: {0}", szString);

    duk_pop(m_pContext); // [ ]
    return PLASMA_FAILURE;
  }

  // [ function ]

  if (duk_pcall(m_pContext, 0) != DUK_EXEC_SUCCESS) // [ result/error ]
  {
    PLASMA_LOG_BLOCK("DukTape::ExecuteString", "Execution failed");

    plLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1)); // [ error ]

    LogStackTrace(-1);

    // TODO: print out line by line
    plLog::Info("[duktape]Source: {0}", szString);

    duk_pop(m_pContext); // [ ]
    return PLASMA_FAILURE;
  }

  duk_pop(m_pContext); // [ ]
  return PLASMA_SUCCESS;
}

plResult plDuktapeHelper::ExecuteStream(plStreamReader& inout_stream, const char* szDebugName)
{
  plStringBuilder source;
  source.ReadAll(inout_stream);

  return ExecuteString(source, szDebugName);
}

plResult plDuktapeHelper::ExecuteFile(const char* szFile)
{
  plFileReader file;
  PLASMA_SUCCEED_OR_RETURN(file.Open(szFile));

  return ExecuteStream(file, szFile);
}

#endif


PLASMA_STATICLINK_FILE(Core, Core_Scripting_Duktape_DuktapeHelper);
