#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct plGizmoEvent;

class plConeVisualizerAdapter : public plVisualizerAdapter
{
public:
  plConeVisualizerAdapter();
  ~plConeVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float m_fFinalScale;
  float m_fAngleScale;
  plEngineGizmoHandle m_hGizmo;
};
