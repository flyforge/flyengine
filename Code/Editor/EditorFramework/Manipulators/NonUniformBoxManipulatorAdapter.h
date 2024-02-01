#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/NonUniformBoxGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct plGizmoEvent;

class plNonUniformBoxManipulatorAdapter : public plManipulatorAdapter
{
public:
  plNonUniformBoxManipulatorAdapter();
  ~plNonUniformBoxManipulatorAdapter();

  virtual void QueryGridSettings(plGridSettingsMsgToEngine& out_gridSettings) override;

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const plGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  plNonUniformBoxGizmo m_Gizmo;
};
