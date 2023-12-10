#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

typedef plComponentManager<class plShapeIconComponent, plBlockStorageType::Compact> plShapeIconComponentManager;

/// \brief This is a dummy component that the editor creates on all 'empty' nodes for the sole purpose to render a shape icon and enable picking.
///
/// Though in the future one could potentially use them for other editor functionality, such as displaying the object name or some other useful text.
class PLASMA_ENGINEPLUGINSCENE_DLL plShapeIconComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plShapeIconComponent, plComponent, plShapeIconComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plShapeIconComponent

public:
  plShapeIconComponent();
  ~plShapeIconComponent();
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_ENGINEPLUGINSCENE_DLL plSceneExportModifier_RemoveShapeIconComponents : public plSceneExportModifier
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSceneExportModifier_RemoveShapeIconComponents, plSceneExportModifier);

public:
  virtual void ModifyWorld(plWorld& world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport) override;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_ENGINEPLUGINSCENE_DLL plSceneExportModifier_RemovePathNodeComponents : public plSceneExportModifier
{
	PLASMA_ADD_DYNAMIC_REFLECTION(plSceneExportModifier_RemovePathNodeComponents, plSceneExportModifier);

public:
  virtual void ModifyWorld(plWorld& world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport) override;
};
