#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/IPC/IPCObjectMirrorEditor.h>

plIPCObjectMirrorEditor::plIPCObjectMirrorEditor()
  : plDocumentObjectMirror()
{
  m_pIPC = nullptr;
}

plIPCObjectMirrorEditor::~plIPCObjectMirrorEditor() = default;

void plIPCObjectMirrorEditor::SetIPC(plEditorEngineConnection* pIPC)
{
  PLASMA_ASSERT_DEBUG(m_pContext == nullptr, "Need to call SetIPC before SetReceiver");
  m_pIPC = pIPC;
}

plEditorEngineConnection* plIPCObjectMirrorEditor::GetIPC()
{
  return m_pIPC;
}

void plIPCObjectMirrorEditor::ApplyOp(plObjectChange& ref_change)
{
  if (m_pManager)
  {
    SendOp(ref_change);
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
