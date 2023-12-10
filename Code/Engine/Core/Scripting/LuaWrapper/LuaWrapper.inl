#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

#  pragma once

inline lua_State* plLuaWrapper::GetLuaState()
{
  return m_pState;
}

inline plInt32 plLuaWrapper::ReturnToScript() const
{
  return (m_States.m_iParametersPushed);
}

inline plUInt32 plLuaWrapper::GetNumberOfFunctionParameters() const
{
  return ((int)lua_gettop(m_pState));
}

inline bool plLuaWrapper::IsParameterBool(plUInt32 uiParameter) const
{
  return (lua_type(m_pState, uiParameter + s_iParamOffset) == LUA_TBOOLEAN);
}

inline bool plLuaWrapper::IsParameterFloat(plUInt32 uiParameter) const
{
  return (lua_type(m_pState, uiParameter + s_iParamOffset) == LUA_TNUMBER);
}

inline bool plLuaWrapper::IsParameterInt(plUInt32 uiParameter) const
{
  return (lua_type(m_pState, uiParameter + s_iParamOffset) == LUA_TNUMBER);
}

inline bool plLuaWrapper::IsParameterString(plUInt32 uiParameter) const
{
  return (lua_type(m_pState, uiParameter + s_iParamOffset) == LUA_TSTRING);
}

inline bool plLuaWrapper::IsParameterNil(plUInt32 uiParameter) const
{
  return (lua_type(m_pState, uiParameter + s_iParamOffset) == LUA_TNIL);
}

inline bool plLuaWrapper::IsParameterTable(plUInt32 uiParameter) const
{
  return (lua_type(m_pState, uiParameter + s_iParamOffset) == LUA_TTABLE);
}

inline void plLuaWrapper::PushParameter(plInt32 iParameter)
{
  lua_pushinteger(m_pState, iParameter);
  m_States.m_iParametersPushed++;
}

inline void plLuaWrapper::PushParameter(bool bParameter)
{
  lua_pushboolean(m_pState, bParameter);
  m_States.m_iParametersPushed++;
}

inline void plLuaWrapper::PushParameter(float fParameter)
{
  lua_pushnumber(m_pState, fParameter);
  m_States.m_iParametersPushed++;
}

inline void plLuaWrapper::PushParameter(const char* szParameter)
{
  lua_pushstring(m_pState, szParameter);
  m_States.m_iParametersPushed++;
}

inline void plLuaWrapper::PushParameter(const char* szParameter, plUInt32 uiLength)
{
  lua_pushlstring(m_pState, szParameter, uiLength);
  m_States.m_iParametersPushed++;
}

inline void plLuaWrapper::PushParameterNil()
{
  lua_pushnil(m_pState);
  m_States.m_iParametersPushed++;
}

inline void plLuaWrapper::PushReturnValue(plInt32 iParameter)
{
  lua_pushinteger(m_pState, iParameter);
  m_States.m_iParametersPushed++;
}

inline void plLuaWrapper::PushReturnValue(bool bParameter)
{
  lua_pushboolean(m_pState, bParameter);
  m_States.m_iParametersPushed++;
}

inline void plLuaWrapper::PushReturnValue(float fParameter)
{
  lua_pushnumber(m_pState, fParameter);
  m_States.m_iParametersPushed++;
}

inline void plLuaWrapper::PushReturnValue(const char* szParameter)
{
  lua_pushstring(m_pState, szParameter);
  m_States.m_iParametersPushed++;
}

inline void plLuaWrapper::PushReturnValue(const char* szParameter, plUInt32 uiLength)
{
  lua_pushlstring(m_pState, szParameter, uiLength);
  m_States.m_iParametersPushed++;
}

inline void plLuaWrapper::PushReturnValueNil()
{
  lua_pushnil(m_pState);
  m_States.m_iParametersPushed++;
}

inline void plLuaWrapper::SetVariableNil(const char* szName) const
{
  lua_pushnil(m_pState);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void plLuaWrapper::SetVariable(const char* szName, plInt32 iValue) const
{
  lua_pushinteger(m_pState, iValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void plLuaWrapper::SetVariable(const char* szName, float fValue) const
{
  lua_pushnumber(m_pState, fValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void plLuaWrapper::SetVariable(const char* szName, bool bValue) const
{
  lua_pushboolean(m_pState, bValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void plLuaWrapper::SetVariable(const char* szName, const char* szValue) const
{
  lua_pushstring(m_pState, szValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void plLuaWrapper::SetVariable(const char* szName, const char* szValue, plUInt32 uiLen) const
{
  lua_pushlstring(m_pState, szValue, uiLen);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void plLuaWrapper::PushTable(const char* szTableName, bool bGlobalTable)
{
  if (bGlobalTable || m_States.m_iOpenTables == 0)
    lua_getglobal(m_pState, szTableName);
  else
  {
    lua_pushstring(m_pState, szTableName);
    lua_gettable(m_pState, -2);
  }

  m_States.m_iParametersPushed++;
}

inline int plLuaWrapper::GetIntParameter(plUInt32 uiParameter) const
{
  return ((int)(lua_tointeger(m_pState, uiParameter + s_iParamOffset)));
}

inline bool plLuaWrapper::GetBoolParameter(plUInt32 uiParameter) const
{
  return (lua_toboolean(m_pState, uiParameter + s_iParamOffset) != 0);
}

inline float plLuaWrapper::GetFloatParameter(plUInt32 uiParameter) const
{
  return ((float)(lua_tonumber(m_pState, uiParameter + s_iParamOffset)));
}

inline const char* plLuaWrapper::GetStringParameter(plUInt32 uiParameter) const
{
  return (lua_tostring(m_pState, uiParameter + s_iParamOffset));
}

#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT
