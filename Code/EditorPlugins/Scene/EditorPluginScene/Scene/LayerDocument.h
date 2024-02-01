#pragma once

#include <EditorPluginScene/Scene/SceneDocument.h>

class plScene2Document;

class PL_EDITORPLUGINSCENE_DLL plLayerDocument : public plSceneDocument
{
  PL_ADD_DYNAMIC_REFLECTION(plLayerDocument, plSceneDocument);

public:
  plLayerDocument(plStringView sDocumentPath, plScene2Document* pParentScene);
  ~plLayerDocument();

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void InitializeAfterLoadingAndSaving() override;

  virtual plVariant GetCreateEngineMetaData() const override;
};
