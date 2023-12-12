#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class plScene2Document;
struct plScene2LayerEvent;

///
class PLASMA_EDITORPLUGINSCENE_DLL plLayerActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapContextMenuActions(const char* szMapping, const char* szPath);

  static plActionDescriptorHandle s_hLayerCategory;
  static plActionDescriptorHandle s_hCreateLayer;
  static plActionDescriptorHandle s_hDeleteLayer;
  static plActionDescriptorHandle s_hSaveLayer;
  static plActionDescriptorHandle s_hSaveActiveLayer;
  static plActionDescriptorHandle s_hLayerLoaded;
  static plActionDescriptorHandle s_hLayerVisible;
};

///
class PLASMA_EDITORPLUGINSCENE_DLL plLayerAction : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLayerAction, plButtonAction);

public:
  enum class ActionType
  {
    CreateLayer,
    DeleteLayer,
    SaveLayer,
    SaveActiveLayer,
    LayerLoaded,
    LayerVisible,
  };

  plLayerAction(const plActionContext& context, const char* szName, ActionType type);
  ~plLayerAction();

  static void ToggleLayerLoaded(plScene2Document* m_pSceneDocument, plUuid layerGuid);
  virtual void Execute(const plVariant& value) override;

private:
  void LayerEventHandler(const plScene2LayerEvent& e);
  void DocumentEventHandler(const plDocumentEvent& e);
  void UpdateEnableState();
  plUuid GetCurrentSelectedLayer() const;

private:
  plScene2Document* m_pSceneDocument;
  ActionType m_Type;
};
