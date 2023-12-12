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

  void SetIPC(PlasmaEditorEngineConnection* pIPC);
  PlasmaEditorEngineConnection* GetIPC();
  virtual void ApplyOp(plObjectChange& change) override;

private:
  void SendOp(plObjectChange& change);

  PlasmaEditorEngineConnection* m_pIPC;
};
