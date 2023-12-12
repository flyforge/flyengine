#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

class plWorld;

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plSceneExportModifier : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSceneExportModifier, plReflectedClass);

public:
  static void CreateModifiers(plHybridArray<plSceneExportModifier*, 8>& modifiers);
  static void DestroyModifiers(plHybridArray<plSceneExportModifier*, 8>& modifiers);

  static void ApplyAllModifiers(plWorld& world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport);

  virtual void ModifyWorld(plWorld& world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport) = 0;

  static void CleanUpWorld(plWorld& world);
};
