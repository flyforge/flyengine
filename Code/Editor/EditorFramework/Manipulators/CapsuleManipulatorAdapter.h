#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/CapsuleGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct plGizmoEvent;

class plCapsuleManipulatorAdapter : public plManipulatorAdapter
{
public:
  plCapsuleManipulatorAdapter();
  ~plCapsuleManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const plGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  plCapsuleGizmo m_Gizmo;
};
