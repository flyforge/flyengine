#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>

class plQtNodeScene;
class plQtNodeView;

class plQtRenderPipelineAssetScene : public plQtNodeScene
{
  Q_OBJECT

public:
  plQtRenderPipelineAssetScene(QObject* parent = nullptr);
  ~plQtRenderPipelineAssetScene();
};

