#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Strings/String.h>

class PLASMA_EDITORFRAMEWORK_DLL plCppSettings
{
public:
  plResult Save(plStringView sFile = ":project/Editor/CppProject.ddl");
  plResult Load(plStringView sFile = ":project/Editor/CppProject.ddl");

  enum class Compiler
  {
    None,
    Vs2022,
  };

  plString m_sPluginName;
  Compiler m_Compiler = Compiler::None;
  mutable plString m_sMsBuildPath;
};
