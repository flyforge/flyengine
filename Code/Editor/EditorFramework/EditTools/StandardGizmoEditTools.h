#pragma once

#include <EditorFramework/EditTools/GizmoEditTool.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>

class plQtGameObjectDocumentWindow;
class plPreferences;

class PLASMA_EDITORFRAMEWORK_DLL plTranslateGizmoEditTool : public plGameObjectGizmoEditTool
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTranslateGizmoEditTool, plGameObjectGizmoEditTool);

public:
  plTranslateGizmoEditTool();
  ~plTranslateGizmoEditTool();

  virtual plEditToolSupportedSpaces GetSupportedSpaces() const override { return plEditToolSupportedSpaces::LocalAndWorldSpace; }
  virtual bool GetSupportsMoveParentOnly() const override { return true; }
  virtual void GetGridSettings(plGridSettingsMsgToEngine& out_gridSettings) override;

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const plTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const plGizmoEvent& e) override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  void OnPreferenceChange(plPreferences* pref);

  plTranslateGizmo m_TranslateGizmo;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_EDITORFRAMEWORK_DLL plRotateGizmoEditTool : public plGameObjectGizmoEditTool
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRotateGizmoEditTool, plGameObjectGizmoEditTool);

public:
  plRotateGizmoEditTool();
  ~plRotateGizmoEditTool();

  virtual plEditToolSupportedSpaces GetSupportedSpaces() const override { return plEditToolSupportedSpaces::LocalAndWorldSpace; }
  virtual bool GetSupportsMoveParentOnly() const override { return true; }

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const plTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const plGizmoEvent& e) override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  plRotateGizmo m_RotateGizmo;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_EDITORFRAMEWORK_DLL plScaleGizmoEditTool : public plGameObjectGizmoEditTool
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plScaleGizmoEditTool, plGameObjectGizmoEditTool);

public:
  plScaleGizmoEditTool();
  ~plScaleGizmoEditTool();

  virtual plEditToolSupportedSpaces GetSupportedSpaces() const override { return plEditToolSupportedSpaces::LocalSpaceOnly; }

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const plTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const plGizmoEvent& e) override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  plScaleGizmo m_ScaleGizmo;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_EDITORFRAMEWORK_DLL plDragToPositionGizmoEditTool : public plGameObjectGizmoEditTool
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDragToPositionGizmoEditTool, plGameObjectGizmoEditTool);

public:
  plDragToPositionGizmoEditTool();
  ~plDragToPositionGizmoEditTool();

  virtual plEditToolSupportedSpaces GetSupportedSpaces() const override { return plEditToolSupportedSpaces::LocalSpaceOnly; }
  virtual bool GetSupportsMoveParentOnly() const override { return true; }

protected:
  virtual void OnConfigured() override;
  virtual void ApplyGizmoVisibleState(bool visible) override;
  virtual void ApplyGizmoTransformation(const plTransform& transform) override;
  virtual void TransformationGizmoEventHandlerImpl(const plGizmoEvent& e) override;
  virtual void OnActiveChanged(bool bIsActive) override;

private:
  plDragToPositionGizmo m_DragToPosGizmo;
};
