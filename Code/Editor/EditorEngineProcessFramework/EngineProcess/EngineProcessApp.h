#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/System/Window.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/Declarations.h>

class plActor;

using plRenderPipelineResourceHandle = plTypedResourceHandle<class plRenderPipelineResource>;

enum class PlasmaEditorEngineProcessMode
{
  Primary,
  Remote,
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plRemoteProcessWindow : public plWindow
{
public:
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL PlasmaEditorEngineProcessApp
{
  PLASMA_DECLARE_SINGLETON(PlasmaEditorEngineProcessApp);

public:
  PlasmaEditorEngineProcessApp();
  ~PlasmaEditorEngineProcessApp();

  void SetRemoteMode();

  bool IsRemoteMode() const { return m_Mode == PlasmaEditorEngineProcessMode::Remote; }

  virtual plViewHandle CreateRemoteWindowAndView(plCamera* pCamera);
  virtual void DestroyRemoteWindow();

  virtual plRenderPipelineResourceHandle CreateDefaultMainRenderPipeline();
  virtual plRenderPipelineResourceHandle CreateDefaultDebugRenderPipeline();

protected:
  virtual void CreateRemoteWindow();

  PlasmaEditorEngineProcessMode m_Mode = PlasmaEditorEngineProcessMode::Primary;

  plActor* m_pActor = nullptr;
  plViewHandle m_hRemoteView;
};
