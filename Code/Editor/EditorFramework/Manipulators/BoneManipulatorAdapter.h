#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/ClickGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <Foundation/Containers/DynamicArray.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>

struct plGizmoEvent;

/// \brief Makes an array of plExposedBone properties editable in the viewport
///
/// Enabled by attaching the plBoneManipulatorAttribute.
class plBoneManipulatorAdapter : public plManipulatorAdapter
{
public:
  plBoneManipulatorAdapter();
  ~plBoneManipulatorAdapter();

protected:
  virtual void Finalize() override;

  void MigrateSelection();

  virtual void Update() override;
  void RotateGizmoEventHandler(const plGizmoEvent& e);
  void ClickGizmoEventHandler(const plGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  struct ElementGizmo
  {
    plMat4 m_Offset;
    plMat4 m_InverseOffset;
    plRotateGizmo m_RotateGizmo;
    plClickGizmo m_ClickGizmo;
  };

  plVariantArray m_Keys;
  plDynamicArray<plExposedBone> m_Bones;
  plDeque<ElementGizmo> m_Gizmos;
  plTransform m_RootTransform = plTransform::MakeIdentity();

  void RetrieveBones();
  void ConfigureGizmos();
  void SetTransform(plUInt32 uiBone, const plTransform& value);
  plMat4 ComputeFullTransform(plUInt32 uiBone) const;
  plMat4 ComputeParentTransform(plUInt32 uiBone) const;

  static plString s_sLastSelectedBone;
};
