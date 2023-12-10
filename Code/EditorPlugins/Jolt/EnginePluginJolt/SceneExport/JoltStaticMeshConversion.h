#pragma once

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>
#include <EnginePluginJolt/EnginePluginJoltDLL.h>

class PLASMA_ENGINEPLUGINJOLT_DLL plSceneExportModifier_JoltStaticMeshConversion : public plSceneExportModifier
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSceneExportModifier_JoltStaticMeshConversion, plSceneExportModifier);

public:
  virtual void ModifyWorld(plWorld& ref_world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport) override;
};
