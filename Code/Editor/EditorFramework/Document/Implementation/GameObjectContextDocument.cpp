#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Document/GameObjectContextDocument.h>
#include <EditorFramework/Preferences/GameObjectContextPreferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGameObjectContextDocument, 2, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plGameObjectContextDocument::plGameObjectContextDocument(
  const char* szDocumentPath, plDocumentObjectManager* pObjectManager, plAssetDocEngineConnection engineConnectionType)
  : plGameObjectDocument(szDocumentPath, pObjectManager, engineConnectionType)
{
}

plGameObjectContextDocument::~plGameObjectContextDocument() = default;

plStatus plGameObjectContextDocument::SetContext(plUuid documentGuid, plUuid objectGuid)
{
  if (!documentGuid.IsValid())
  {
    {
      plGameObjectContextEvent e;
      e.m_Type = plGameObjectContextEvent::Type::ContextAboutToBeChanged;
      m_GameObjectContextEvents.Broadcast(e);
    }
    ClearContext();
    {
      plGameObjectContextEvent e;
      e.m_Type = plGameObjectContextEvent::Type::ContextChanged;
      m_GameObjectContextEvents.Broadcast(e);
    }
    return plStatus(PLASMA_SUCCESS);
  }

  const plAbstractObjectGraph* pPrefab = plPrefabCache::GetSingleton()->GetCachedPrefabGraph(documentGuid);
  if (!pPrefab)
    return plStatus("Context document could not be loaded.");

  {
    plGameObjectContextEvent e;
    e.m_Type = plGameObjectContextEvent::Type::ContextAboutToBeChanged;
    m_GameObjectContextEvents.Broadcast(e);
  }
  ClearContext();
  plAbstractObjectGraph graph;
  pPrefab->Clone(graph);

  plRttiConverterContext context;
  plRttiConverterReader rttiConverter(&graph, &context);
  plDocumentObjectConverterReader objectConverter(&graph, GetObjectManager(), plDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
  {
    PLASMA_PROFILE_SCOPE("Restoring Objects");
    auto* pRootNode = graph.GetNodeByName("ObjectTree");
    PLASMA_ASSERT_DEV(pRootNode->FindProperty("TempObjects") == nullptr, "TempObjects should not be serialized.");
    pRootNode->RenameProperty("Children", "TempObjects");
    objectConverter.ApplyPropertiesToObject(pRootNode, GetObjectManager()->GetRootObject());
  }
  {
    PLASMA_PROFILE_SCOPE("Restoring Meta-Data");
    RestoreMetaDataAfterLoading(graph, false);
  }
  {
    plGameObjectContextPreferencesUser* pPreferences = plPreferences::QueryPreferences<plGameObjectContextPreferencesUser>(this);
    m_ContextDocument = documentGuid;
    pPreferences->SetContextDocument(m_ContextDocument);

    const plDocumentObject* pContextObject = GetObjectManager()->GetObject(objectGuid);
    m_ContextObject = pContextObject ? objectGuid : plUuid();
    pPreferences->SetContextObject(m_ContextObject);
  }
  {
    plGameObjectContextEvent e;
    e.m_Type = plGameObjectContextEvent::Type::ContextChanged;
    m_GameObjectContextEvents.Broadcast(e);
  }
  return plStatus(PLASMA_SUCCESS);
}

plUuid plGameObjectContextDocument::GetContextDocumentGuid() const
{
  return m_ContextDocument;
}

plUuid plGameObjectContextDocument::GetContextObjectGuid() const
{
  return m_ContextObject;
}

const plDocumentObject* plGameObjectContextDocument::GetContextObject() const
{
  if (m_ContextDocument.IsValid())
  {
    if (m_ContextObject.IsValid())
    {
      return GetObjectManager()->GetObject(m_ContextObject);
    }
    return GetObjectManager()->GetRootObject();
  }
  return nullptr;
}

void plGameObjectContextDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  plGameObjectContextPreferencesUser* pPreferences = plPreferences::QueryPreferences<plGameObjectContextPreferencesUser>(this);
  SetContext(pPreferences->GetContextDocument(), pPreferences->GetContextObject()).LogFailure();
  SUPER::InitializeAfterLoading(bFirstTimeCreation);
}

void plGameObjectContextDocument::ClearContext()
{
  m_ContextDocument = plUuid();
  m_ContextObject = plUuid();
  plDocumentObject* pRoot = GetObjectManager()->GetRootObject();
  plHybridArray<plVariant, 16> values;
  GetObjectAccessor()->GetValues(pRoot, "TempObjects", values).IgnoreResult();
  for (plInt32 i = (plInt32)values.GetCount() - 1; i >= 0; --i)
  {
    plDocumentObject* pChild = GetObjectManager()->GetObject(values[i].Get<plUuid>());
    GetObjectManager()->RemoveObject(pChild);
    GetObjectManager()->DestroyObject(pChild);
  }
}
