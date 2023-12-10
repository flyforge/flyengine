#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>


class plLUTAssetProperties : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLUTAssetProperties, plReflectedClass);

public:
  static void PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e);

  const char* GetInputFile() const { return m_sInput; }
  void SetInputFile(const char* szFile) { m_sInput = szFile; }

  plString GetAbsoluteInputFilePath() const;

private:
  plString m_sInput;
};
