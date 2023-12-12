#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <ToolsFoundationTest/Object/TestObjectManager.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTestDocument, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plTestDocumentObjectManager::plTestDocumentObjectManager() = default;
plTestDocumentObjectManager::~plTestDocumentObjectManager() = default;

plTestDocument::plTestDocument(const char* szDocumentPath, bool bUseIPCObjectMirror /*= false*/)
  : plDocument(szDocumentPath, PLASMA_DEFAULT_NEW(plTestDocumentObjectManager))
  , m_bUseIPCObjectMirror(bUseIPCObjectMirror)
{
}

plTestDocument::~plTestDocument()
{
  if (m_bUseIPCObjectMirror)
  {
    m_ObjectMirror.Clear();
    m_ObjectMirror.DeInit();
  }
}

void plTestDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  if (m_bUseIPCObjectMirror)
  {
    m_ObjectMirror.InitSender(GetObjectManager());
    m_ObjectMirror.InitReceiver(&m_Context);
    m_ObjectMirror.SendDocument();
  }
}

void plTestDocument::ApplyNativePropertyChangesToObjectManager(plDocumentObject* pObject)
{
  // Create native object graph
  plAbstractObjectGraph graph;
  plAbstractObjectNode* pRootNode = nullptr;
  {
    plRttiConverterWriter rttiConverter(&graph, &m_Context, true, true);
    pRootNode = rttiConverter.AddObjectToGraph(pObject->GetType(), m_ObjectMirror.GetNativeObjectPointer(pObject), "Object");
  }

  // Create object manager graph
  plAbstractObjectGraph origGraph;
  plAbstractObjectNode* pOrigRootNode = nullptr;
  {
    plDocumentObjectConverterWriter writer(&origGraph, GetObjectManager());
    pOrigRootNode = writer.AddObjectToGraph(pObject);
  }

  // Remap native guids so they match the object manager (stuff like embedded classes will not have a guid on the native side).
  graph.ReMapNodeGuidsToMatchGraph(pRootNode, origGraph, pOrigRootNode);
  plDeque<plAbstractGraphDiffOperation> diffResult;

  graph.CreateDiffWithBaseGraph(origGraph, diffResult);

  // As we messed up the native side the object mirror is no longer synced and needs to be destroyed.
  m_ObjectMirror.Clear();
  m_ObjectMirror.DeInit();

  // Apply diff while object mirror is down.
  GetObjectAccessor()->StartTransaction("Apply Native Property Changes to Object");
  plDocumentObjectConverterReader::ApplyDiffToObject(GetObjectAccessor(), pObject, diffResult);
  GetObjectAccessor()->FinishTransaction();

  // Restart mirror from scratch.
  m_ObjectMirror.InitSender(GetObjectManager());
  m_ObjectMirror.InitReceiver(&m_Context);
  m_ObjectMirror.SendDocument();
}

plDocumentInfo* plTestDocument::CreateDocumentInfo()
{
  return PLASMA_DEFAULT_NEW(plDocumentInfo);
}
