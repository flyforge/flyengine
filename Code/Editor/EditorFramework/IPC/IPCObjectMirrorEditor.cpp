#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/IPC/IPCObjectMirrorEditor.h>

plIPCObjectMirrorEditor::plIPCObjectMirrorEditor()
  : plDocumentObjectMirror()
{
  m_pIPC = nullptr;
}

plIPCObjectMirrorEditor::~plIPCObjectMirrorEditor() {}

void plIPCObjectMirrorEditor::SetIPC(PlasmaEditorEngineConnection* pIPC)
{
  PLASMA_ASSERT_DEBUG(m_pContext == nullptr, "Need to call SetIPC before SetReceiver");
  m_pIPC = pIPC;
}

PlasmaEditorEngineConnection* plIPCObjectMirrorEditor::GetIPC()
{
  return m_pIPC;
}

void plIPCObjectMirrorEditor::ApplyOp(plObjectChange& change)
{
  if (m_pManager)
  {
    SendOp(change);
  }
  else
  {
    PLASMA_REPORT_FAILURE("plIPCObjectMirrorEngine not set up for sender nor receiver!");
  }
}

void plIPCObjectMirrorEditor::SendOp(plObjectChange& change)
{
  PLASMA_ASSERT_DEBUG(m_pIPC != nullptr, "Need to call SetIPC before SetReceiver");

  plEntityMsgToEngine msg;
  msg.m_change = std::move(change);

  m_pIPC->SendMessage(&msg);
}
