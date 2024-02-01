#pragma once

#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/DrawBoxGizmo.h>

struct plGameObjectEvent;
struct plManipulatorManagerEvent;

class PL_EDITORPLUGINSCENE_DLL plGreyBoxEditTool : public plGameObjectEditTool
{
  PL_ADD_DYNAMIC_REFLECTION(plGreyBoxEditTool, plGameObjectEditTool);

public:
  plGreyBoxEditTool();
  ~plGreyBoxEditTool();

  virtual plEditorInputContext* GetEditorInputContextOverride() override;
  virtual plEditToolSupportedSpaces GetSupportedSpaces() const override;
  virtual bool GetSupportsMoveParentOnly() const override;
  virtual void GetGridSettings(plGridSettingsMsgToEngine& out_gridSettings) override;

protected:
  virtual void OnConfigured() override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  void UpdateGizmoState();
  void GameObjectEventHandler(const plGameObjectEvent& e);
  void ManipulatorManagerEventHandler(const plManipulatorManagerEvent& e);
  void GizmoEventHandler(const plGizmoEvent& e);

  plDrawBoxGizmo m_DrawBoxGizmo;
};
