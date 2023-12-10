#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/IPC/IPCObjectMirrorEngine.h>

plIPCObjectMirrorEngine::plIPCObjectMirrorEngine()
  : plDocumentObjectMirror()
{
}

plIPCObjectMirrorEngine::~plIPCObjectMirrorEngine() = default;

void plIPCObjectMirrorEngine::ApplyOp(plObjectChange& inout_change)
{
  if (m_pContext)
  {
    plDocumentObjectMirror::ApplyOp(inout_change);
  }
  else
  {
    PLASMA_REPORT_FAILURE("plIPCObjectMirrorEngine not set up for sender nor receiver!");
  }
}
