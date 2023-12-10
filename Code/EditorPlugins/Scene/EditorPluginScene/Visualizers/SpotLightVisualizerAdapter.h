#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class plSpotLightVisualizerAdapter : public plVisualizerAdapter
{
public:
  plSpotLightVisualizerAdapter();
  ~plSpotLightVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_fScale;
  float m_fAngleScale;
  plEngineGizmoHandle m_hGizmo;
};
