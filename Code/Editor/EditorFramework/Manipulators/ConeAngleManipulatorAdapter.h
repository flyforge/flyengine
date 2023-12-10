#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/ConeAngleGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct plGizmoEvent;

class plConeAngleManipulatorAdapter : public plManipulatorAdapter
{
public:
  plConeAngleManipulatorAdapter();
  ~plConeAngleManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const plGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  plConeAngleGizmo m_Gizmo;
};
