#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct plGizmoEvent;

class plDirectionVisualizerAdapter : public plVisualizerAdapter
{
public:
  plDirectionVisualizerAdapter();
  ~plDirectionVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  PlasmaEngineGizmoHandle m_hGizmo;
};
