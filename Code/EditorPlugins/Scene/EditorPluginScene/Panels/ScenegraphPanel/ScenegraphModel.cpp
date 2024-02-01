#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>

plQtScenegraphModel::plQtScenegraphModel(const plDocumentObjectManager* pObjectManager, const plUuid& root)
  : plQtGameObjectModel(pObjectManager, root)
{
}

plQtScenegraphModel::~plQtScenegraphModel() = default;
