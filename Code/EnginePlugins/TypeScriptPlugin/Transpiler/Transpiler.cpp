#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <TypeScriptPlugin/Transpiler/Transpiler.h>

plTypeScriptTranspiler::plTypeScriptTranspiler()
  : m_Transpiler("TypeScript Transpiler")
{
}

plTypeScriptTranspiler::~plTypeScriptTranspiler() = default;

void plTypeScriptTranspiler::SetOutputFolder(const char* szFolder)
{
  m_sOutputFolder = szFolder;
}

void plTypeScriptTranspiler::StartLoadTranspiler()
{
  if (m_LoadTaskGroup.IsValid())
    return;

  plSharedPtr<plTask> pTask = PL_DEFAULT_NEW(plDelegateTask<void>, "Load TypeScript Transpiler", plTaskNesting::Never,
    [this]() //
    {
      PL_PROFILE_SCOPE("Load TypeScript Transpiler");

      if (m_Transpiler.ExecuteFile("typescriptServices.js").Failed())
      {
        plLog::Error("typescriptServices.js could not be loaded");
      }

      plLog::Success("Loaded TypeScript transpiler.");
    });

  pTask->ConfigureTask("Load TypeScript Transpiler", plTaskNesting::Never);
  m_LoadTaskGroup = plTaskSystem::StartSingleTask(pTask, plTaskPriority::LongRunning);
}

void plTypeScriptTranspiler::FinishLoadTranspiler()
{
  StartLoadTranspiler();

  plTaskSystem::WaitForGroup(m_LoadTaskGroup);
}

plResult plTypeScriptTranspiler::TranspileString(const char* szString, plStringBuilder& out_sResult)
{
  PL_LOG_BLOCK("TranspileString");

  FinishLoadTranspiler();

  PL_PROFILE_SCOPE("Transpile TypeScript");

  plDuktapeHelper duk(m_Transpiler);

  m_Transpiler.PushGlobalObject();                 // [ global ]
  if (m_Transpiler.PushLocalObject("ts").Failed()) // [ global ts ]
  {
    plLog::Error("'ts' object does not exist");
    duk.PopStack(2); // [ ]
    PL_DUK_RETURN_AND_VERIFY_STACK(duk, PL_FAILURE, 0);
  }

  if (m_Transpiler.PrepareObjectFunctionCall("transpile").Failed()) // [ global ts transpile ]
  {
    plLog::Error("'ts.transpile' function does not exist");
    duk.PopStack(3); // [ ]
    PL_DUK_RETURN_AND_VERIFY_STACK(duk, PL_FAILURE, 0);
  }

  m_Transpiler.PushString(szString);                // [ global ts transpile source ]
  if (m_Transpiler.CallPreparedFunction().Failed()) // [ global ts result ]
  {
    plLog::Error("String could not be transpiled");
    duk.PopStack(3); // [ ]
    PL_DUK_RETURN_AND_VERIFY_STACK(duk, PL_FAILURE, 0);
  }

  out_sResult = m_Transpiler.GetStringValue(-1); // [ global ts result ]
  m_Transpiler.PopStack(3);                     // [ ]

  PL_DUK_RETURN_AND_VERIFY_STACK(duk, PL_SUCCESS, 0);
}

plResult plTypeScriptTranspiler::TranspileFile(const char* szFile, plUInt64 uiSkipIfFileHash, plStringBuilder& out_sResult, plUInt64& out_uiFileHash)
{
  PL_LOG_BLOCK("TranspileFile", szFile);

  FinishLoadTranspiler();

  plFileReader file;
  if (file.Open(szFile).Failed())
  {
    plLog::Error("File does not exist: '{}'", szFile);
    return PL_FAILURE;
  }

  plStringBuilder source;
  source.ReadAll(file);

  if (m_ModifyTsBeforeTranspilationCB.IsValid())
  {
    m_ModifyTsBeforeTranspilationCB(source);
  }

  out_uiFileHash = plHashingUtils::xxHash64(source.GetData(), source.GetElementCount());

  if (uiSkipIfFileHash == out_uiFileHash)
    return PL_SUCCESS;

  return TranspileString(source, out_sResult);
}

plResult plTypeScriptTranspiler::TranspileFileAndStoreJS(const char* szFile, plStringBuilder& out_sResult)
{
  PL_LOG_BLOCK("TranspileFileAndStoreJS", szFile);

  PL_ASSERT_DEV(!m_sOutputFolder.IsEmpty(), "Output folder has not been set");

  plUInt64 uiExpectedFileHash = 0;
  plUInt64 uiActualFileHash = 0;

  plStringBuilder sOutFile = szFile;
  sOutFile.ChangeFileExtension("js");
  sOutFile.Prepend(m_sOutputFolder, "/");
  sOutFile.MakeCleanPath();

  {
    plFileReader fileIn;
    if (fileIn.Open(sOutFile).Succeeded())
    {
      out_sResult.ReadAll(fileIn);

      if (out_sResult.StartsWith_NoCase("/*SOURCE-HASH:"))
      {
        plStringView sHashView = out_sResult.GetView();
        sHashView.Shrink(14, 0);

        // try to extract the hash
        if (plConversionUtils::ConvertHexStringToUInt64(sHashView, uiExpectedFileHash).Failed())
        {
          uiExpectedFileHash = 0;
        }
      }
    }
  }

  PL_SUCCEED_OR_RETURN(TranspileFile(szFile, uiExpectedFileHash, out_sResult, uiActualFileHash));

  if (uiExpectedFileHash != uiActualFileHash)
  {
    plFileWriter fileOut;
    if (fileOut.Open(sOutFile).Failed())
    {
      plLog::Error("Could not write transpiled JS to file '{}'", sOutFile);
      return PL_FAILURE;
    }

    plStringBuilder sHashHeader;
    sHashHeader.SetFormat("/*SOURCE-HASH:{}*/\n", plArgU(uiActualFileHash, 16, true, 16, true));
    out_sResult.Prepend(sHashHeader);

    PL_SUCCEED_OR_RETURN(fileOut.WriteBytes(out_sResult.GetData(), out_sResult.GetElementCount()));
    plLog::Success("Transpiled '{}'", szFile);
  }

  return PL_SUCCESS;
}

void plTypeScriptTranspiler::SetModifyTsBeforeTranspilationCallback(plDelegate<void(plStringBuilder&)> callback)
{
  m_ModifyTsBeforeTranspilationCB = callback;
}
