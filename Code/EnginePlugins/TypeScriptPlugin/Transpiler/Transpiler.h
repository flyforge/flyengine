#pragma once

#include <TypeScriptPlugin/TypeScriptPluginDLL.h>

#include <Core/Scripting/DuktapeContext.h>
#include <Foundation/Basics.h>
#include <Foundation/Threading/TaskSystem.h>

class PL_TYPESCRIPTPLUGIN_DLL plTypeScriptTranspiler
{
public:
  plTypeScriptTranspiler();
  ~plTypeScriptTranspiler();

  void SetOutputFolder(const char* szFolder);
  void StartLoadTranspiler();
  void FinishLoadTranspiler();
  plResult TranspileString(const char* szString, plStringBuilder& out_sResult);
  plResult TranspileFile(const char* szFile, plUInt64 uiSkipIfFileHash, plStringBuilder& out_sResult, plUInt64& out_uiFileHash);
  plResult TranspileFileAndStoreJS(const char* szFile, plStringBuilder& out_sResult);
  void SetModifyTsBeforeTranspilationCallback(plDelegate<void(plStringBuilder&)> callback);

private:
  plDelegate<void(plStringBuilder&)> m_ModifyTsBeforeTranspilationCB;
  plString m_sOutputFolder;
  plTaskGroupID m_LoadTaskGroup;
  plDuktapeContext m_Transpiler;
};
