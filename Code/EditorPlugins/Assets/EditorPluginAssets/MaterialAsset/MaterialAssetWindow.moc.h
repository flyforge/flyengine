#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plMaterialAssetDocument;
class plQtOrbitCamViewWidget;
class plQtVisualShaderScene;
class plQtNodeView;
struct plSelectionManagerEvent;
class plDirectoryWatcher;
enum class plDirectoryWatcherAction;
enum class plDirectoryWatcherType;
class plQtDocumentPanel;
class QTextEdit;
struct plMaterialVisualShaderEvent;

class plQtMaterialAssetDocumentWindow : public plQtEngineDocumentWindow
{
  Q_OBJECT

public:
  plQtMaterialAssetDocumentWindow(plMaterialAssetDocument* pDocument);
  ~plQtMaterialAssetDocumentWindow();

  plMaterialAssetDocument* GetMaterialDocument();
  virtual const char* GetWindowLayoutGroupName() const override { return "MaterialAsset"; }

protected:
  virtual void InternalRedraw() override;


  virtual void showEvent(QShowEvent* event) override;

private Q_SLOTS:
  void OnOpenShaderClicked(bool);

private:
  void UpdatePreview();
  void PropertyEventHandler(const plDocumentObjectPropertyEvent& e);
  void SelectionEventHandler(const plSelectionManagerEvent& e);
  void SendRedrawMsg();
  void RestoreResource();
  void UpdateNodeEditorVisibility();
  void OnVseConfigChanged(plStringView sFilename, plDirectoryWatcherAction action, plDirectoryWatcherType type);
  void VisualShaderEventHandler(const plMaterialVisualShaderEvent& e);
  void SetupDirectoryWatcher(bool needIt);

  plEngineViewConfig m_ViewConfig;
  plQtOrbitCamViewWidget* m_pViewWidget = nullptr;
  plQtVisualShaderScene* m_pScene = nullptr;
  plQtNodeView* m_pNodeView = nullptr;
  plQtDocumentPanel* m_pVsePanel = nullptr;
  QTextEdit* m_pOutputLine = nullptr;
  QPushButton* m_pOpenShaderButton = nullptr;
  bool m_bVisualShaderEnabled;

  static plInt32 s_iNodeConfigWatchers;
  static plDirectoryWatcher* s_pNodeConfigWatcher;
};

class plMaterialModelAction : public plEnumerationMenuAction
{
  PL_ADD_DYNAMIC_REFLECTION(plMaterialModelAction, plEnumerationMenuAction);

public:
  plMaterialModelAction(const plActionContext& context, const char* szName, const char* szIconPath);
  virtual plInt64 GetValue() const override;
  virtual void Execute(const plVariant& value) override;
};

class plMaterialAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(plStringView sMapping);

  static plActionDescriptorHandle s_hMaterialModelAction;
};
