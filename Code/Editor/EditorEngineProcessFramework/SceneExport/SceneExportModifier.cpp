#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSceneExportModifier, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
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

void plSceneExportModifier::DestroyModifiers(plHybridArray<plSceneExportModifier*, 8>& ref_modifiers)
{
  for (auto pMod : ref_modifiers)
  {
    pMod->GetDynamicRTTI()->GetAllocator()->Deallocate(pMod);
  }

  ref_modifiers.Clear();
}

void plSceneExportModifier::ApplyAllModifiers(plWorld& ref_world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport)
{
  plHybridArray<plSceneExportModifier*, 8> modifiers;
  CreateModifiers(modifiers);

  for (auto pMod : modifiers)
  {
    pMod->ModifyWorld(ref_world, sDocumentType, documentGuid, bForExport);
  }

  DestroyModifiers(modifiers);

  CleanUpWorld(ref_world);
}

void VisitObject(plWorld& ref_world, plGameObject* pObject)
{
  for (auto it = pObject->GetChildren(); it.IsValid(); it.Next())
  {
    VisitObject(ref_world, it);
  }

  if (pObject->GetChildCount() > 0)
    return;

  if (!pObject->GetComponents().IsEmpty())
    return;

  if (!pObject->GetName().IsEmpty())
    return;

  if (!pObject->GetGlobalKey().IsEmpty())
    return;

  ref_world.DeleteObjectDelayed(pObject->GetHandle(), false);
}

void plSceneExportModifier::CleanUpWorld(plWorld& ref_world)
{
  PL_LOCK(ref_world.GetWriteMarker());

  // Don't do this (for now), as we would also delete objects that are referenced by other components,
  // and currently we can't know which ones are important to keep.

  // for (auto it = world.GetObjects(); it.IsValid(); it.Next())
  //{
  //   // only visit objects without parents, those are the root objects
  //   if (it->GetParent() != nullptr)
  //     continue;

  //  VisitObject(world, it);
  //}

  const bool bSim = ref_world.GetWorldSimulationEnabled();
  ref_world.SetWorldSimulationEnabled(false);
  ref_world.Update();
  ref_world.SetWorldSimulationEnabled(bSim);
}
