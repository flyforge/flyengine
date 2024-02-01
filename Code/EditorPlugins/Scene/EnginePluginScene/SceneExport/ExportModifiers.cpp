#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/Components/ShapeIconComponent.h>
#include <EnginePluginScene/SceneExport/ExportModifiers.h>
#include <GameEngine/Animation/PathComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSceneExportModifier_RemoveShapeIconComponents, 1, plRTTIDefaultAllocator<plSceneExportModifier_RemoveShapeIconComponents>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plSceneExportModifier_RemoveShapeIconComponents::ModifyWorld(plWorld& ref_world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport)
{
  PL_LOCK(ref_world.GetWriteMarker());

  if (plShapeIconComponentManager* pSiMan = ref_world.GetComponentManager<plShapeIconComponentManager>())
  {
    for (auto it = pSiMan->GetComponents(); it.IsValid(); it.Next())
    {
      pSiMan->DeleteComponent(it->GetHandle());
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSceneExportModifier_RemovePathNodeComponents, 1, plRTTIDefaultAllocator<plSceneExportModifier_RemovePathNodeComponents>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plSceneExportModifier_RemovePathNodeComponents::ModifyWorld(plWorld& ref_world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport)
{
  if (!bForExport)
    return;

  PL_LOCK(ref_world.GetWriteMarker());

  if (plPathComponentManager* pSiMan = ref_world.GetComponentManager<plPathComponentManager>())
  {
    for (auto it = pSiMan->GetComponents(); it.IsValid(); it.Next())
    {
      it->EnsureControlPointRepresentationIsUpToDate();
      it->SetDisableControlPointUpdates(true);
    }
  }

  if (plPathNodeComponentManager* pSiMan = ref_world.GetComponentManager<plPathNodeComponentManager>())
  {
    for (auto it = pSiMan->GetComponents(); it.IsValid(); it.Next())
    {
      if (it->GetOwner()->GetComponents().GetCount() == 1 && it->GetOwner()->GetChildCount() == 0)
      {
        // if this is the only component on the object, clear it's name, so that the entire object may get cleaned up
        it->GetOwner()->SetName(plStringView());
      }

      pSiMan->DeleteComponent(it->GetHandle());
    }
  }
}

//////////////////////////////////////////////////////////////////////////
