#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/IPC/IPCObjectMirrorEngine.h>

plIPCObjectMirrorEngine::plIPCObjectMirrorEngine()
  : plDocumentObjectMirror()
{
}

plIPCObjectMirrorEngine::~plIPCObjectMirrorEngine() {}

void plIPCObjectMirrorEngine::ApplyOp(plObjectChange& change)
{
  if (m_pContext)
  {
    plDocumentObjectMirror::ApplyOp(change);
  }
  else
  {
    PLASMA_REPORT_FAILURE("plIPCObjectMirrorEngine not set up for sender nor receiver!");
  }
}
