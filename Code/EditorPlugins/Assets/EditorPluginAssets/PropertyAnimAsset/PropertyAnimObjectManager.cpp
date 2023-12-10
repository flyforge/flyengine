#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>

plPropertyAnimObjectManager::plPropertyAnimObjectManager() = default;

plPropertyAnimObjectManager::~plPropertyAnimObjectManager() = default;

plStatus plPropertyAnimObjectManager::InternalCanAdd(
  const plRTTI* pRtti, const plDocumentObject* pParent, plStringView sParentProperty, const plVariant& index) const
{
  if (m_bAllowStructureChangeOnTemporaries)
    return plStatus(PLASMA_SUCCESS);

  if (IsTemporary(pParent, sParentProperty))
    return plStatus("The structure of the context cannot be animated.");
  return plStatus(PLASMA_SUCCESS);
}

plStatus plPropertyAnimObjectManager::InternalCanRemove(const plDocumentObject* pObject) const
{
  if (m_bAllowStructureChangeOnTemporaries)
    return plStatus(PLASMA_SUCCESS);

  if (IsTemporary(pObject))
    return plStatus("The structure of the context cannot be animated.");
  return plStatus(PLASMA_SUCCESS);
}

plStatus plPropertyAnimObjectManager::InternalCanMove(
  const plDocumentObject* pObject, const plDocumentObject* pNewParent, plStringView sParentProperty, const plVariant& index) const
{
  if (m_bAllowStructureChangeOnTemporaries)
    return plStatus(PLASMA_SUCCESS);

  if (IsTemporary(pObject))
    return plStatus("The structure of the context cannot be animated.");
  return plStatus(PLASMA_SUCCESS);
}
