#pragma once

#include <EditorPluginScene/Scene/SceneDocument.h>

class plScene2Document;

class PLASMA_EDITORPLUGINSCENE_DLL plLayerDocument : public plSceneDocument
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLayerDocument, plSceneDocument);

public:
  plLayerDocument(const char* szDocumentPath, plScene2Document* pParentScene);
  ~plLayerDocument();

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void InitializeAfterLoadingAndSaving() override;

  virtual plVariant GetCreateEngineMetaData() const override;
};
