#include <EditorPluginScene/EditorPluginScenePCH.h>

#include "Foundation/Serialization/GraphPatch.h"
#include <Core/World/GameObject.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSceneDocumentSettingsBase, 1, plRTTINoAllocator)
{
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plPrefabDocumentSettings, 1, plRTTIDefaultAllocator<plPrefabDocumentSettings>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY("ExposedProperties", m_ExposedProperties),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLayerDocumentSettings, 1, plRTTIDefaultAllocator<plLayerDocumentSettings>)
{
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSceneDocumentRoot, 2, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Settings", m_pSettings)->AddFlags(plPropertyFlags::PointerOwner),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

plSceneObjectManager::plSceneObjectManager()
  : plDocumentObjectManager(plGetStaticRTTI<plSceneDocumentRoot>())
{
}

void plSceneObjectManager::GetCreateableTypes(plHybridArray<const plRTTI*, 32>& Types) const
{
  Types.PushBack(plGetStaticRTTI<plGameObject>());

  plRTTI::ForEachDerivedType<plComponent>(
    [&](const plRTTI* pRtti) { Types.PushBack(pRtti); },
    plRTTI::ForEachOptions::ExcludeAbstract);
}

plStatus plSceneObjectManager::InternalCanAdd(
  const plRTTI* pRtti, const plDocumentObject* pParent, const char* szParentProperty, const plVariant& index) const
{
  if (IsUnderRootProperty("Children", pParent, szParentProperty))
  {
    if (pParent == nullptr)
    {
      bool bIsDerived = pRtti->IsDerivedFrom<plGameObject>();
      if (!bIsDerived)
      {
        return plStatus("Only plGameObject can be added to the root of the world!");
      }
    }
    else
    {
      // only prevent adding game objects (as children) to objects that already have a prefab component
      // do allow to attach components to objects with prefab components
      // if (pRtti->IsDerivedFrom<plGameObject>())
      //{
      //  if (pParent->GetTypeAccessor().GetType()->IsDerivedFrom<plGameObject>())
      //  {
      //    auto children = pParent->GetChildren();
      //    for (auto pChild : children)
      //    {
      //      if (pChild->GetType()->IsDerivedFrom<plPrefabReferenceComponent>())
      //        return plStatus("Cannot add objects to a prefab node.");
      //    }
      //  }
      //}

      // in case prefab component should be the only component on a node
      // if (pRtti->IsDerivedFrom<plPrefabReferenceComponent>())
      //{
      //  if (!pParent->GetChildren().IsEmpty())
      //    return plStatus("Prefab components can only be added to empty nodes.");
      //}
    }
  }
  return plStatus(PLASMA_SUCCESS);
}

plStatus plSceneObjectManager::InternalCanMove(
  const plDocumentObject* pObject, const plDocumentObject* pNewParent, const char* szParentProperty, const plVariant& index) const
{
  // code to disallow attaching nodes to a prefab node
  // if (pNewParent != nullptr)
  //{
  //  if (pNewParent->GetTypeAccessor().GetType()->IsDerivedFrom<plGameObject>())
  //  {
  //    auto children = pNewParent->GetChildren();
  //    for (auto pChild : children)
  //    {
  //      if (pChild->GetType()->IsDerivedFrom<plPrefabReferenceComponent>())
  //        return plStatus("Cannot move objects into a prefab node.");
  //    }
  //  }
  //}

  return plStatus(PLASMA_SUCCESS);
}

plStatus plSceneObjectManager::InternalCanSelect(const plDocumentObject* pObject) const
{
  /*if (pObject->GetTypeAccessor().GetType() != plGetStaticRTTI<plGameObject>())
  {
    return plStatus(
      plFmt("Object of type '{0}' is not a 'plGameObject' and can't be selected.", pObject->GetTypeAccessor().GetType()->GetTypeName()));
  }*/
  return plStatus(PLASMA_SUCCESS);
}

namespace
{
  /// Patch class
  class plSceneDocumentSettings_1_2 : public plGraphPatch
  {
  public:
    plSceneDocumentSettings_1_2()
      : plGraphPatch("plSceneDocumentSettings", 2)
    {
    }
    virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
    {
      // Previously, plSceneDocumentSettings only contained prefab settings. As these only apply to prefab documents, we switch the old version to prefab.
      context.RenameClass("plPrefabDocumentSettings", 1);
      plVersionKey bases[] = {{"plSceneDocumentSettingsBase", 1}, {"plReflectedClass", 1}};
      context.ChangeBaseClass(bases);
    }
  };
  plSceneDocumentSettings_1_2 g_plSceneDocumentSettings_1_2;
} // namespace
