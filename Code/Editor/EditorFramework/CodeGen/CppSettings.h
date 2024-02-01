#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Strings/String.h>

class PL_EDITORFRAMEWORK_DLL plCppSettings
{
public:
  plResult Save(plStringView sFile = ":project/Editor/CppProject.ddl");
  plResult Load(plStringView sFile = ":project/Editor/CppProject.ddl");

  plString m_sPluginName;
};
