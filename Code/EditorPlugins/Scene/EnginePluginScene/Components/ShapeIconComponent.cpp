#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/Components/ShapeIconComponent.h>
#include <GameEngine/Animation/PathComponent.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plShapeIconComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Editing Utilities"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plShapeIconComponent::plShapeIconComponent() = default;
plShapeIconComponent::~plShapeIconComponent() = default;

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSceneExportModifier_RemoveShapeIconComponents, 1, plRTTIDefaultAllocator<plSceneExportModifier_RemoveShapeIconComponents>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plSceneExportModifier_RemoveShapeIconComponents::ModifyWorld(plWorld& world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport)
{
  PLASMA_LOCK(world.GetWriteMarker());

  if (plShapeIconComponentManager* pSiMan = world.GetComponentManager<plShapeIconComponentManager>())
  {
    for (auto it = pSiMan->GetComponents(); it.IsValid(); it.Next())
    {
      pSiMan->DeleteComponent(it->GetHandle());
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSceneExportModifier_RemovePathNodeComponents, 1, plRTTIDefaultAllocator<plSceneExportModifier_RemovePathNodeComponents>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plSceneExportModifier_RemovePathNodeComponents::ModifyWorld(plWorld& world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport)
{
	if (!bForExport)
		return;

	PLASMA_LOCK(world.GetWriteMarker());

	if (plPathComponentManager* pSiMan = world.GetComponentManager<plPathComponentManager>())
	{
		for (auto it = pSiMan->GetComponents(); it.IsValid(); it.Next())
		{
      it->EnsureControlPointRepresentationIsUpToDate();
      it->SetDisableControlPointUpdates(true);
		}
	}

	if (plPathNodeComponentManager* pSiMan = world.GetComponentManager<plPathNodeComponentManager>())
	{
		for (auto it = pSiMan->GetComponents(); it.IsValid(); it.Next())
		{
			if (it->GetOwner()->GetComponents().GetCount() == 1)
			{
				// if this is the only component on the object, clear it's name, so that the entire object may get cleaned up
        it->GetOwner()->SetName(plStringView());
			}

			pSiMan->DeleteComponent(it->GetHandle());
		}
	}
}
