#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class plBoxReflectionProbeVisualizerAdapter : public plVisualizerAdapter
{
public:
  plBoxReflectionProbeVisualizerAdapter();
  ~plBoxReflectionProbeVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  plVec3 m_vScale;
  plVec3 m_vPositionOffset;
  plQuat m_qRotation;

  PlasmaEngineGizmoHandle m_hGizmo;
};
