#include "CppStructure.h"
#include "DLangGenerator.h"
#include "RapidXML/rapidxml.hpp"

#include <Core/Scripting/LuaWrapper.h>
#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Utilities/CommandLineOptions.h>

using namespace rapidxml;

class DLangCodeGenTool : public plApplication
{
public:
  typedef plApplication SUPER;

  DLangCodeGenTool()
    : plApplication("DLangCodeGenTool")
  {
  }

  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;

  virtual Execution Run() override;

  plString m_sXmlFile;
  plUniquePtr<DLangGenerator> m_pGenerator;

private:
  plResult ParseArguments();

  CppStructure m_Structure;

private:
  void FindTargetFiles();
  void RemovePreviousAutogen();
  void CleanTargetFiles();
  void ReadFileContent(const char* szFile);
  void WriteFileContent(const char* szFile);
  void ProcessAllFiles();
  void ProcessFileCodeGen();
  void ProcessFileCommands();
  static int Lua_Struct(lua_State* context);
  static int Lua_Class(lua_State* context);
  static int Lua_Enum(lua_State* context);
  static int Lua_InterfaceStruct(lua_State* context);
  static int Lua_SynthesizeTemplate(lua_State* context);
  static int Lua_WhitelistType(lua_State* context);

  enum class Phase
  {
    GatherInfo,
    GenerateCode,
  };

  Phase m_Phase = Phase::GatherInfo;
  plStringBuilder m_sCurFileContent;
  const char* m_szCurFileInsertPos = nullptr;
  plSet<plString> m_TargetFiles;
  plLuaWrapper m_Lua;
};
