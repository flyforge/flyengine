#include <Core/CorePCH.h>

#include <Core/Scripting/LuaWrapper.h>

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

plLuaWrapper::plLuaWrapper()
{
  m_bReleaseOnExit = true;
  m_pState = nullptr;

  Clear();
}

plLuaWrapper::plLuaWrapper(lua_State* s)
{
  m_pState = s;
  m_bReleaseOnExit = false;
}

plLuaWrapper::~plLuaWrapper()
{
  if (m_bReleaseOnExit)
    lua_close(m_pState);
}

void plLuaWrapper::Clear()
{
  PLASMA_ASSERT_DEV(m_bReleaseOnExit, "Cannot clear a script that did not create the Lua state itself.");

  if (m_pState)
    lua_close(m_pState);

  m_pState = lua_newstate(lua_allocator, nullptr);

  luaL_openlibs(m_pState);
}

plResult plLuaWrapper::ExecuteString(const char* szString, const char* szDebugChunkName, plLogInterface* pLogInterface) const
{
  PLASMA_ASSERT_DEV(m_States.m_iLuaReturnValues == 0,
    "plLuaWrapper::ExecuteString: You didn't discard the return-values of the previous script call. {0} Return-values were expected.",
    m_States.m_iLuaReturnValues);

  if (!pLogInterface)
    pLogInterface = plLog::GetThreadLocalLogSystem();

  int error = luaL_loadbuffer(m_pState, szString, plStringUtils::GetStringElementCount(szString), szDebugChunkName);

  if (error != LUA_OK)
  {
    PLASMA_LOG_BLOCK("plLuaWrapper::ExecuteString");

    plLog::Error(pLogInterface, "[lua]Lua compile error: {0}", lua_tostring(m_pState, -1));
    plLog::Info(pLogInterface, "[luascript]Script: {0}", szString);

    return PLASMA_FAILURE;
  }

  error = lua_pcall(m_pState, 0, 0, 0);

  if (error != LUA_OK)
  {
    PLASMA_LOG_BLOCK("plLuaWrapper::ExecuteString");

    plLog::Error(pLogInterface, "[lua]Lua error: {0}", lua_tostring(m_pState, -1));
    plLog::Info(pLogInterface, "[luascript]Script: {0}", szString);

    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

void* plLuaWrapper::lua_allocator(void* ud, void* ptr, size_t osize, size_t nsize)
{
  /// \todo Create optimized allocator.

  if (nsize == 0)
  {
    delete[](plUInt8*) ptr;
    return (nullptr);
  }

  plUInt8* ucPtr = new plUInt8[nsize];

  if (ptr != nullptr)
  {
    plMemoryUtils::Copy(ucPtr, (plUInt8*)ptr, plUInt32(osize < nsize ? osize : nsize));

    delete[](plUInt8*) ptr;
  }

  return ((void*)ucPtr);
}


#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT


PLASMA_STATICLINK_FILE(Core, Core_Scripting_LuaWrapper_Initialize);
