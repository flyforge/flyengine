#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/NonUniformBoxGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct plGizmoEvent;

class plBoxManipulatorAdapter : public plManipulatorAdapter
{
public:
  plBoxManipulatorAdapter();
  ~plBoxManipulatorAdapter();

  virtual void QueryGridSettings(plGridSettingsMsgToEngine& outGridSettings) override;

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const plGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  plVec3 m_vPositionOffset;
  plQuat m_qRotation;
  plNonUniformBoxGizmo m_Gizmo;

  plVec3 m_vOldSize;
};
