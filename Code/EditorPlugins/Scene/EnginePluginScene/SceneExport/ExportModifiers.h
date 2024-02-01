#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

class PL_ENGINEPLUGINSCENE_DLL plSceneExportModifier_RemoveShapeIconComponents : public plSceneExportModifier
{
  PL_ADD_DYNAMIC_REFLECTION(plSceneExportModifier_RemoveShapeIconComponents, plSceneExportModifier);

public:
  virtual void ModifyWorld(plWorld& ref_world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport) override;
};

//////////////////////////////////////////////////////////////////////////

class PL_ENGINEPLUGINSCENE_DLL plSceneExportModifier_RemovePathNodeComponents : public plSceneExportModifier
{
  PL_ADD_DYNAMIC_REFLECTION(plSceneExportModifier_RemovePathNodeComponents, plSceneExportModifier);

public:
  virtual void ModifyWorld(plWorld& ref_world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport) override;
};
