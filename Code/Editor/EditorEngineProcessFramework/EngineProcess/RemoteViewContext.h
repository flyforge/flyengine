#pragma once

#include <Core/System/Window.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/Declarations.h>

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plRemoteEngineProcessViewContext : public PlasmaEngineProcessViewContext
{
public:
  plRemoteEngineProcessViewContext(PlasmaEngineProcessDocumentContext* pContext);
  ~plRemoteEngineProcessViewContext();

protected:
  virtual void HandleViewMessage(const PlasmaEditorEngineViewMsg* pMsg) override;
  virtual plViewHandle CreateView() override;

  static plUInt32 s_uiActiveViewID;
  static plRemoteEngineProcessViewContext* s_pActiveRemoteViewContext;
};
