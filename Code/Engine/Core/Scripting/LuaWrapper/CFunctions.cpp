#include <Core/CorePCH.h>

#include <Core/Scripting/LuaWrapper.h>

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

void plLuaWrapper::RegisterCFunction(const char* szFunctionName, lua_CFunction function, void* pLightUserData) const
{
  lua_pushlightuserdata(m_pState, pLightUserData);
  lua_pushcclosure(m_pState, function, 1);
  lua_setglobal(m_pState, szFunctionName);
}

void* plLuaWrapper::GetFunctionLightUserData() const
{
  return lua_touserdata(m_pState, lua_upvalueindex(1));
}

bool plLuaWrapper::PrepareFunctionCall(const char* szFunctionName)
{
  PL_ASSERT_DEV(m_States.m_iLuaReturnValues == 0,
    "plLuaWrapper::PrepareFunctionCall: You didn't discard the return-values of the previous script call. {0} Return-values "
    "were expected.",
    m_States.m_iLuaReturnValues);

  m_States.m_iParametersPushed = 0;

  if (m_States.m_iOpenTables == 0)
    lua_getglobal(m_pState, szFunctionName);
  else
  {
    lua_pushstring(m_pState, szFunctionName);
    lua_gettable(m_pState, -2);
  }

  if (lua_isfunction(m_pState, -1) == 0)
  {
    lua_pop(m_pState, 1);
    return false;
  }

  return true;
}

plResult plLuaWrapper::CallPreparedFunction(plUInt32 uiExpectedReturnValues, plLogInterface* pLogInterface)
{
  m_States.m_iLuaReturnValues = uiExpectedReturnValues;

  // save the current states on a cheap stack
  const plScriptStates StackedStates = m_States;
  m_States = plScriptStates();

  if (pLogInterface == nullptr)
    pLogInterface = plLog::GetThreadLocalLogSystem();

  if (lua_pcall(m_pState, StackedStates.m_iParametersPushed, uiExpectedReturnValues, 0) != 0)
  {
    // restore the states to their previous values
    m_States = StackedStates;

    m_States.m_iLuaReturnValues = 0;

    plLog::Error(pLogInterface, "Script-function Call: {0}", lua_tostring(m_pState, -1));

    lua_pop(m_pState, 1); /* pop error message from the stack */
    return PL_FAILURE;
  }

  // before resetting the state, make sure the returned state has no stuff left
  PL_ASSERT_DEV((m_States.m_iLuaReturnValues == 0) && (m_States.m_iOpenTables == 0),
    "After plLuaWrapper::CallPreparedFunction: Return values: {0}, Open Tables: {1}", m_States.m_iLuaReturnValues, m_States.m_iOpenTables);

  m_States = StackedStates;
  return PL_SUCCESS;
}

void plLuaWrapper::DiscardReturnValues()
{
  if (m_States.m_iLuaReturnValues == 0)
    return;

  lua_pop(m_pState, m_States.m_iLuaReturnValues);
  m_States.m_iLuaReturnValues = 0;
}

bool plLuaWrapper::IsReturnValueInt(plUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TNUMBER);
}

bool plLuaWrapper::IsReturnValueBool(plUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TBOOLEAN);
}

bool plLuaWrapper::IsReturnValueFloat(plUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TNUMBER);
}

bool plLuaWrapper::IsReturnValueString(plUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TSTRING);
}

bool plLuaWrapper::IsReturnValueNil(plUInt32 uiReturnValue) const
{
  return (lua_type(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) == LUA_TNIL);
}

plInt32 plLuaWrapper::GetIntReturnValue(plUInt32 uiReturnValue) const
{
  return ((int)(lua_tointeger(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1)));
}

bool plLuaWrapper::GetBoolReturnValue(plUInt32 uiReturnValue) const
{
  return (lua_toboolean(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1) != 0);
}

float plLuaWrapper::GetFloatReturnValue(plUInt32 uiReturnValue) const
{
  return ((float)(lua_tonumber(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1)));
}

const char* plLuaWrapper::GetStringReturnValue(plUInt32 uiReturnValue) const
{
  return (lua_tostring(m_pState, -m_States.m_iLuaReturnValues + (uiReturnValue + s_iParamOffset) - 1));
}


#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT


