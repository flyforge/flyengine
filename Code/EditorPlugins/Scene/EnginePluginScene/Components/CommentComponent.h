#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

using plCommentComponentManager = plComponentManager<class plCommentComponent, plBlockStorageType::Compact>;

/// \brief This component is for adding notes to objects in a scene.
///
/// These comments are solely to explain things to other people that look at the scene or prefab structure.
/// They are not meant for use at runtime. Therefore, all instances of plCommentComponent are automatically stripped from a scene during export.
class PLASMA_ENGINEPLUGINSCENE_DLL plCommentComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plCommentComponent, plComponent, plCommentComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plCommentComponent

public:
  plCommentComponent();
  ~plCommentComponent();

  void SetComment(const char* text);
  const char* GetComment() const;

private:
  plHashedString m_sComment;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_ENGINEPLUGINSCENE_DLL plSceneExportModifier_RemoveCommentComponents : public plSceneExportModifier
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSceneExportModifier_RemoveCommentComponents, plSceneExportModifier);

public:
  virtual void ModifyWorld(plWorld& world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport) override;
};
