#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>

/// \brief An object mirror that mirrors across IPC to the engine process.
///
/// One instance on the editor side needs to be initialized as sender and another
/// one on the engine side as receiver.
class PLASMA_EDITORFRAMEWORK_DLL plIPCObjectMirrorEditor : public plDocumentObjectMirror
{
public:
  plIPCObjectMirrorEditor();
  ~plIPCObjectMirrorEditor();

  void SetIPC(plEditorEngineConnection* pIPC);
  plEditorEngineConnection* GetIPC();
  virtual void ApplyOp(plObjectChange& ref_change) override;

private:
  void SendOp(plObjectChange& change);

  plEditorEngineConnection* m_pIPC;
};
