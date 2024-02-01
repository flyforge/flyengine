#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Log(duk_context* pDuk);

plResult plTypeScriptBinding::Init_Log()
{
  m_Duk.RegisterGlobalFunction("__CPP_Log_Error", __CPP_Log, 1, plLogMsgType::ErrorMsg);
  m_Duk.RegisterGlobalFunction("__CPP_Log_SeriousWarning", __CPP_Log, 1, plLogMsgType::SeriousWarningMsg);
  m_Duk.RegisterGlobalFunction("__CPP_Log_Warning", __CPP_Log, 1, plLogMsgType::WarningMsg);
  m_Duk.RegisterGlobalFunction("__CPP_Log_Success", __CPP_Log, 1, plLogMsgType::SuccessMsg);
  m_Duk.RegisterGlobalFunction("__CPP_Log_Info", __CPP_Log, 1, plLogMsgType::InfoMsg);
  m_Duk.RegisterGlobalFunction("__CPP_Log_Dev", __CPP_Log, 1, plLogMsgType::DevMsg);
  m_Duk.RegisterGlobalFunction("__CPP_Log_Debug", __CPP_Log, 1, plLogMsgType::DebugMsg);

  return PL_SUCCESS;
}

static int __CPP_Log(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);
  const plInt16 iMagic = duk.GetFunctionMagicValue();

  switch (iMagic)
  {
    case plLogMsgType::ErrorMsg:
      plLog::Error(duk.GetStringValue(0));
      break;
    case plLogMsgType::SeriousWarningMsg:
      plLog::SeriousWarning(duk.GetStringValue(0));
      break;
    case plLogMsgType::WarningMsg:
      plLog::Warning(duk.GetStringValue(0));
      break;
    case plLogMsgType::SuccessMsg:
      plLog::Success(duk.GetStringValue(0));
      break;
    case plLogMsgType::InfoMsg:
      plLog::Info(duk.GetStringValue(0));
      break;
    case plLogMsgType::DevMsg:
      plLog::Dev(duk.GetStringValue(0));
      break;
    case plLogMsgType::DebugMsg:
      plLog::Debug(duk.GetStringValue(0));
      break;
  }

  return duk.ReturnVoid();
}
