#pragma once

#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>

class plScene2Document;

class plQtScene2DocumentWindow : public plQtSceneDocumentWindowBase
{
  Q_OBJECT

public:
  plQtScene2DocumentWindow(plScene2Document* pDocument);
  ~plQtScene2DocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "Scene2"; }
  virtual bool InternalCanCloseWindow() override;

  plStatus SaveAllLayers();
};
