#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/System/Window.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/Declarations.h>

class plActor;

using plRenderPipelineResourceHandle = plTypedResourceHandle<class plRenderPipelineResource>;

enum class plEditorEngineProcessMode
{
  Primary,
  Remote,
};

class PL_EDITORENGINEPROCESSFRAMEWORK_DLL plRemoteProcessWindow : public plWindow
{
public:
};

class PL_EDITORENGINEPROCESSFRAMEWORK_DLL plEditorEngineProcessApp
{
  PL_DECLARE_SINGLETON(plEditorEngineProcessApp);

public:
  plEditorEngineProcessApp();
  ~plEditorEngineProcessApp();

  void SetRemoteMode();

  bool IsRemoteMode() const { return m_Mode == plEditorEngineProcessMode::Remote; }

  virtual plViewHandle CreateRemoteWindowAndView(plCamera* pCamera);
  void DestroyRemoteWindow();

  virtual plRenderPipelineResourceHandle CreateDefaultMainRenderPipeline();
  virtual plRenderPipelineResourceHandle CreateDefaultDebugRenderPipeline();

protected:
  virtual void CreateRemoteWindow();

  plEditorEngineProcessMode m_Mode = plEditorEngineProcessMode::Primary;

  plActor* m_pActor = nullptr;
  plViewHandle m_hRemoteView;
};
