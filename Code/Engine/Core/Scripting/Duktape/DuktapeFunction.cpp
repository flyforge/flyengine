#include <Core/CorePCH.h>

#include <Core/Scripting/DuktapeFunction.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Duktape/duk_module_duktape.h>
#  include <Duktape/duktape.h>

plDuktapeFunction::plDuktapeFunction(duk_context* pExistingContext)
  : plDuktapeHelper(pExistingContext)
{
}

plDuktapeFunction::~plDuktapeFunction()
{
#  if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  if (m_bVerifyStackChange && !m_bDidReturnValue)
  {
    plLog::Error("You need to call one plDuktapeFunction::ReturnXY() and return its result from your C function.");
  }
#  endif
}

plUInt32 plDuktapeFunction::GetNumVarArgFunctionParameters() const
{
  return duk_get_top(GetContext());
}

plInt16 plDuktapeFunction::GetFunctionMagicValue() const
{
  return static_cast<plInt16>(duk_get_current_magic(GetContext()));
}

plInt32 plDuktapeFunction::ReturnVoid()
{
  PL_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  return 0;
}

plInt32 plDuktapeFunction::ReturnNull()
{
  PL_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_null(GetContext());
  return 1;
}

plInt32 plDuktapeFunction::ReturnUndefined()
{
  PL_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_undefined(GetContext());
  return 1;
}

plInt32 plDuktapeFunction::ReturnBool(bool value)
{
  PL_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_boolean(GetContext(), value);
  return 1;
}

plInt32 plDuktapeFunction::ReturnInt(plInt32 value)
{
  PL_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_int(GetContext(), value);
  return 1;
}

plInt32 plDuktapeFunction::ReturnUInt(plUInt32 value)
{
  PL_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_uint(GetContext(), value);
  return 1;
}

plInt32 plDuktapeFunction::ReturnFloat(float value)
{
  PL_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_number(GetContext(), value);
  return 1;
}

plInt32 plDuktapeFunction::ReturnNumber(double value)
{
  PL_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_number(GetContext(), value);
  return 1;
}

plInt32 plDuktapeFunction::ReturnString(plStringView value)
{
  PL_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  duk_push_lstring(GetContext(), value.GetStartPointer(), value.GetElementCount());
  return 1;
}

plInt32 plDuktapeFunction::ReturnCustom()
{
  PL_ASSERT_DEV(!m_bDidReturnValue, "Only one ReturnXYZ function may be called when exiting a C function");
  m_bDidReturnValue = true;
  // push nothing, the user calls this because he pushed something custom already
  return 1;
}

#endif


