#pragma once

#include <EditorFramework/Panels/GameObjectPanel/GameObjectModel.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Basics.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>

class plSceneDocument;

class plQtScenegraphModel : public plQtGameObjectModel
{
  Q_OBJECT

public:
  plQtScenegraphModel(const plDocumentObjectManager* pObjectManager, const plUuid& root = plUuid());
  ~plQtScenegraphModel();
};

