#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

class plWorld;

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plSceneExportModifier : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSceneExportModifier, plReflectedClass);

public:
  static void CreateModifiers(plHybridArray<plSceneExportModifier*, 8>& ref_modifiers);
  static void DestroyModifiers(plHybridArray<plSceneExportModifier*, 8>& ref_modifiers);

  static void ApplyAllModifiers(plWorld& ref_world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport);

  virtual void ModifyWorld(plWorld& ref_world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport) = 0;

  static void CleanUpWorld(plWorld& ref_world);
};
