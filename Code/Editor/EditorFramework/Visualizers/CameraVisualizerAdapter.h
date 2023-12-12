#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class plCameraVisualizerAdapter : public plVisualizerAdapter
{
public:
  plCameraVisualizerAdapter();
  ~plCameraVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  plTransform m_LocalTransformFrustum;
  plTransform m_LocalTransformNearPlane;
  plTransform m_LocalTransformFarPlane;
  PlasmaEngineGizmoHandle m_hBoxGizmo;
  PlasmaEngineGizmoHandle m_hFrustumGizmo;
  PlasmaEngineGizmoHandle m_hNearPlaneGizmo;
  PlasmaEngineGizmoHandle m_hFarPlaneGizmo;
};
