#pragma once

#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/EditorFrameworkDLL.h>

struct PlasmaEngineWindowEvent;
struct plGameObjectEvent;
struct plDocumentObjectStructureEvent;
struct plManipulatorManagerEvent;
struct plSelectionManagerEvent;
struct plCommandHistoryEvent;
struct plGizmoEvent;

class PLASMA_EDITORFRAMEWORK_DLL plGameObjectGizmoEditTool : public plGameObjectEditTool
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGameObjectGizmoEditTool, plGameObjectEditTool);

public:
  plGameObjectGizmoEditTool();
  ~plGameObjectGizmoEditTool();

  void TransformationGizmoEventHandler(const plGizmoEvent& e);

protected:
  virtual void OnConfigured() override;

  void UpdateGizmoSelectionList();

  void UpdateGizmoVisibleState();
  virtual void ApplyGizmoVisibleState(bool visible) = 0;

  void UpdateGizmoTransformation();
  virtual void ApplyGizmoTransformation(const plTransform& transform) = 0;

  virtual void TransformationGizmoEventHandlerImpl(const plGizmoEvent& e) = 0;

  plDeque<plSelectedGameObject> m_GizmoSelection;
  bool m_bInGizmoInteraction = false;
  bool m_bMergeTransactions = false;

private:
  void DocumentWindowEventHandler(const plQtDocumentWindowEvent& e);
  void UpdateManipulatorVisibility();
  void GameObjectEventHandler(const plGameObjectEvent& e);
  void CommandHistoryEventHandler(const plCommandHistoryEvent& e);
  void SelectionManagerEventHandler(const plSelectionManagerEvent& e);
  void ManipulatorManagerEventHandler(const plManipulatorManagerEvent& e);
  void EngineWindowEventHandler(const PlasmaEngineWindowEvent& e);
  void ObjectStructureEventHandler(const plDocumentObjectStructureEvent& e);
};
