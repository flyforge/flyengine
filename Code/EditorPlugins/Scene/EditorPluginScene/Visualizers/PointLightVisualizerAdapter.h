#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class plPointLightVisualizerAdapter : public plVisualizerAdapter
{
public:
  plPointLightVisualizerAdapter();
  ~plPointLightVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_fScale;
  plEngineGizmoHandle m_hGizmo;
};
