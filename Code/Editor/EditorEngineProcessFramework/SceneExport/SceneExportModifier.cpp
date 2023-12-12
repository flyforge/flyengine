#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSceneExportModifier, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plSceneExportModifier::CreateModifiers(plHybridArray<plSceneExportModifier*, 8>& ref_modifiers)
{
  plRTTI::ForEachDerivedType<plSceneExportModifier>(
    [&](const plRTTI* pRtti) {
      plSceneExportModifier* pMod = pRtti->GetAllocator()->Allocate<plSceneExportModifier>();
      ref_modifiers.PushBack(pMod);
    },
    plRTTI::ForEachOptions::ExcludeNonAllocatable);
}

void plSceneExportModifier::DestroyModifiers(plHybridArray<plSceneExportModifier*, 8>& modifiers)
{
  for (auto pMod : modifiers)
  {
    pMod->GetDynamicRTTI()->GetAllocator()->Deallocate(pMod);
  }

  modifiers.Clear();
}

void plSceneExportModifier::ApplyAllModifiers(plWorld& world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport)
{
  plHybridArray<plSceneExportModifier*, 8> modifiers;
  CreateModifiers(modifiers);

  for (auto pMod : modifiers)
  {
    pMod->ModifyWorld(world, sDocumentType, documentGuid, bForExport);
  }

  DestroyModifiers(modifiers);

  CleanUpWorld(world);
}

void VisitObject(plWorld& world, plGameObject* pObject)
{
  for (auto it = pObject->GetChildren(); it.IsValid(); it.Next())
  {
    VisitObject(world, it);
  }

  if (pObject->GetChildCount() > 0)
    return;

  if (!pObject->GetComponents().IsEmpty())
    return;

  if (!pObject->GetName().IsEmpty())
    return;

  if (!pObject->GetGlobalKey().IsEmpty())
    return;

  world.DeleteObjectDelayed(pObject->GetHandle(), false);
}

void plSceneExportModifier::CleanUpWorld(plWorld& world)
{
  PLASMA_LOCK(world.GetWriteMarker());

//  for (auto it = world.GetObjects(); it.IsValid(); it.Next())
//  {
//    // only visit objects without parents, those are the root objects
//    if (it->GetParent() != nullptr)
//      continue;
//
//    VisitObject(world, it);
//  }

  const bool bSim = world.GetWorldSimulationEnabled();
  world.SetWorldSimulationEnabled(false);
  world.Update();
  world.SetWorldSimulationEnabled(bSim);
}
