#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct plGizmoEvent;

class plTransformManipulatorAdapter : public plManipulatorAdapter
{
public:
  plTransformManipulatorAdapter();
  ~plTransformManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void GizmoEventHandler(const plGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  plVec3 GetTranslation();
  plQuat GetRotation();
  plVec3 GetScale();

  virtual plTransform GetOffsetTransform() const override;

  plTranslateGizmo m_TranslateGizmo;
  plRotateGizmo m_RotateGizmo;
  plManipulatorScaleGizmo m_ScaleGizmo;
  plVec3 m_vOldScale;

  bool m_bHideTranslate = true;
  bool m_bHideRotate = true;
  bool m_bHideScale = true;
};
