#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/ConeLengthGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct plGizmoEvent;

class plConeLengthManipulatorAdapter : public plManipulatorAdapter
{
public:
  plConeLengthManipulatorAdapter();
  ~plConeLengthManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const plGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  plConeLengthGizmo m_Gizmo;
};
