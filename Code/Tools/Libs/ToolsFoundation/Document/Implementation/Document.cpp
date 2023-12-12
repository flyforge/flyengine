#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/Progress.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Document/DocumentTasks.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDocumentObjectMetaData, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    //PLASMA_MEMBER_PROPERTY("MetaHidden", m_bHidden) // remove this property to disable serialization
    PLASMA_MEMBER_PROPERTY("MetaFromPrefab", m_CreateFromPrefab),
    PLASMA_MEMBER_PROPERTY("MetaPrefabSeed", m_PrefabSeedGuid),
    PLASMA_MEMBER_PROPERTY("MetaBasePrefab", m_sBasePrefab),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDocumentInfo, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("DocumentID", m_DocumentID),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plDocumentInfo::plDocumentInfo()
{
  m_DocumentID.CreateNewUuid();
}


PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDocument, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plEvent<const plDocumentEvent&> plDocument::s_EventsAny;

plDocument::plDocument(const char* szPath, plDocumentObjectManager* pDocumentObjectManagerImpl)
{
  using ObjectMetaData = plObjectMetaData<plUuid, plDocumentObjectMetaData>;
  m_DocumentObjectMetaData = PLASMA_DEFAULT_NEW(ObjectMetaData);
  m_pDocumentInfo = nullptr;
  m_sDocumentPath = szPath;
  m_pObjectManager = plUniquePtr<plDocumentObjectManager>(pDocumentObjectManagerImpl, plFoundation::GetDefaultAllocator());
  m_pObjectManager->SetDocument(this);
  m_pCommandHistory = PLASMA_DEFAULT_NEW(plCommandHistory, this);
  m_pSelectionManager = PLASMA_DEFAULT_NEW(plSelectionManager, m_pObjectManager.Borrow());
  
  if (m_pObjectAccessor == nullptr)
  {
    m_pObjectAccessor = PLASMA_DEFAULT_NEW(plObjectCommandAccessor, m_pCommandHistory.Borrow());
  }

  m_bWindowRequested = false;
  m_bModified = true;
  m_bReadOnly = false;
  m_bAddToRecentFilesList = true;

  m_uiUnknownObjectTypeInstances = 0;

  m_pHostDocument = this;
  m_pActiveSubDocument = this;
}

plDocument::~plDocument()
{
  m_pSelectionManager = nullptr;

  m_pObjectManager->DestroyAllObjects();

  m_pCommandHistory->ClearRedoHistory();
  m_pCommandHistory->ClearUndoHistory();

  PLASMA_DEFAULT_DELETE(m_pDocumentInfo);
}

void plDocument::SetupDocumentInfo(const plDocumentTypeDescriptor* pTypeDescriptor)
{
  m_pTypeDescriptor = pTypeDescriptor;
  m_pDocumentInfo = CreateDocumentInfo();

  PLASMA_ASSERT_DEV(m_pDocumentInfo != nullptr, "invalid document info");
}

void plDocument::SetModified(bool b)
{
  if (m_bModified == b)
    return;

  m_bModified = b;

  plDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = plDocumentEvent::Type::ModifiedChanged;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

void plDocument::SetReadOnly(bool b)
{
  if (m_bReadOnly == b)
    return;

  m_bReadOnly = b;

  plDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = plDocumentEvent::Type::ReadOnlyChanged;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

plStatus plDocument::SaveDocument(bool bForce)
{
  if (!IsModified() && !bForce)
    return plStatus(PLASMA_SUCCESS);

  // In the unlikely event that we manage to edit a doc and call save again while
  // an async save is already in progress we block on the first save to ensure
  // the correct chronological state on disk after both save ops are done.
  if (m_ActiveSaveTask.IsValid())
  {
    plTaskSystem::WaitForGroup(m_ActiveSaveTask);
    m_ActiveSaveTask.Invalidate();
  }
  plStatus result;
  m_ActiveSaveTask = InternalSaveDocument([&result](plDocument* doc, plStatus res) { result = res; });
  plTaskSystem::WaitForGroup(m_ActiveSaveTask);
  m_ActiveSaveTask.Invalidate();
  return result;
}


plTaskGroupID plDocument::SaveDocumentAsync(AfterSaveCallback callback, bool bForce)
{
  if (!IsModified() && !bForce)
    return plTaskGroupID();

  m_ActiveSaveTask = InternalSaveDocument(callback);
  return m_ActiveSaveTask;
}

void plDocument::EnsureVisible()
{
  plDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = plDocumentEvent::Type::EnsureVisible;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

plTaskGroupID plDocument::InternalSaveDocument(AfterSaveCallback callback)
{
  PLASMA_PROFILE_SCOPE("InternalSaveDocument");
  plTaskGroupID saveID = plTaskSystem::CreateTaskGroup(plTaskPriority::LongRunningHighPriority);
  auto saveTask = PLASMA_DEFAULT_NEW(plSaveDocumentTask);

  {
    saveTask->m_document = this;
    saveTask->file.SetOutput(m_sDocumentPath);
    plTaskSystem::AddTaskToGroup(saveID, saveTask);

    {
      plRttiConverterContext context;
      plRttiConverterWriter rttiConverter(&saveTask->headerGraph, &context, true, true);
      context.RegisterObject(GetGuid(), m_pDocumentInfo->GetDynamicRTTI(), m_pDocumentInfo);
      rttiConverter.AddObjectToGraph(m_pDocumentInfo, "Header");
    }
    {
      // Do not serialize any temporary properties into the document.
      auto filter = [](const plDocumentObject*, const plAbstractProperty* pProp) -> bool
      {
        if (pProp->GetAttributeByType<plTemporaryAttribute>() != nullptr)
          return false;
        return true;
      };
      plDocumentObjectConverterWriter objectConverter(&saveTask->objectGraph, GetObjectManager(), filter);
      objectConverter.AddObjectToGraph(GetObjectManager()->GetRootObject(), "ObjectTree");

      AttachMetaDataBeforeSaving(saveTask->objectGraph);
    }
    {
      plSet<const plRTTI*> types;
      plToolsReflectionUtils::GatherObjectTypes(GetObjectManager()->GetRootObject(), types);
      plToolsSerializationUtils::SerializeTypes(types, saveTask->typesGraph);
    }
  }

  plTaskGroupID afterSaveID = plTaskSystem::CreateTaskGroup(plTaskPriority::SomeFrameMainThread);
  {
    auto afterSaveTask = PLASMA_DEFAULT_NEW(plAfterSaveDocumentTask);
    afterSaveTask->m_document = this;
    afterSaveTask->m_callback = callback;
    plTaskSystem::AddTaskToGroup(afterSaveID, afterSaveTask);
  }
  plTaskSystem::AddTaskGroupDependency(afterSaveID, saveID);
  if (!plTaskSystem::IsTaskGroupFinished(m_ActiveSaveTask))
  {
    plTaskSystem::AddTaskGroupDependency(saveID, m_ActiveSaveTask);
  }

  plTaskSystem::StartTaskGroup(saveID);
  plTaskSystem::StartTaskGroup(afterSaveID);
  return afterSaveID;
}

plStatus plDocument::ReadDocument(const char* sDocumentPath, plUniquePtr<plAbstractObjectGraph>& header, plUniquePtr<plAbstractObjectGraph>& objects,
  plUniquePtr<plAbstractObjectGraph>& types)
{
  plDefaultMemoryStreamStorage storage;
  plMemoryStreamReader memreader(&storage);

  {
    PLASMA_PROFILE_SCOPE("Read File");
    plFileReader file;
    if (file.Open(sDocumentPath) == PLASMA_FAILURE)
    {
      return plStatus("Unable to open file for reading!");
    }

    // range.BeginNextStep("Reading File");
    storage.ReadAll(file);

    // range.BeginNextStep("Parsing Graph");
    {
      PLASMA_PROFILE_SCOPE("parse DDL graph");
      plStopwatch sw;
      if (plAbstractGraphDdlSerializer::ReadDocument(memreader, header, objects, types, true).Failed())
        return plStatus("Failed to parse DDL graph");

      plTime t = sw.GetRunningTotal();
      plLog::Debug("DDL parsing time: {0} msec", plArgF(t.GetMilliseconds(), 1));
    }
  }
  return plStatus(PLASMA_SUCCESS);
}

plStatus plDocument::ReadAndRegisterTypes(const plAbstractObjectGraph& types)
{
  PLASMA_PROFILE_SCOPE("Deserializing Types");
  // range.BeginNextStep("Deserializing Types");

  // Deserialize and register serialized phantom types.
  plString sDescTypeName = plGetStaticRTTI<plReflectedTypeDescriptor>()->GetTypeName();
  plDynamicArray<plReflectedTypeDescriptor*> descriptors;
  auto& nodes = types.GetAllNodes();
  descriptors.Reserve(nodes.GetCount()); // Overkill but doesn't matter much as it's just temporary.
  plRttiConverterContext context;
  plRttiConverterReader rttiConverter(&types, &context);

  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->GetType() == sDescTypeName)
    {
      plReflectedTypeDescriptor* pDesc = rttiConverter.CreateObjectFromNode(it.Value()).Cast<plReflectedTypeDescriptor>();
      if (pDesc->m_Flags.IsSet(plTypeFlags::Minimal))
      {
        plGetStaticRTTI<plReflectedTypeDescriptor>()->GetAllocator()->Deallocate(pDesc);
      }
      else
      {
        descriptors.PushBack(pDesc);
      }
    }
  }
  plToolsReflectionUtils::DependencySortTypeDescriptorArray(descriptors);
  for (plReflectedTypeDescriptor* desc : descriptors)
  {
    if (!plRTTI::FindTypeByName(desc->m_sTypeName))
    {
      plPhantomRttiManager::RegisterType(*desc);
    }
    plGetStaticRTTI<plReflectedTypeDescriptor>()->GetAllocator()->Deallocate(desc);
  }
  return plStatus(PLASMA_SUCCESS);
}

plStatus plDocument::InternalLoadDocument()
{
  PLASMA_PROFILE_SCOPE("InternalLoadDocument");
  // this would currently crash in Qt, due to the processEvents in the QtProgressBar
  // plProgressRange range("Loading Document", 5, false);

  plUniquePtr<plAbstractObjectGraph> header;
  plUniquePtr<plAbstractObjectGraph> objects;
  plUniquePtr<plAbstractObjectGraph> types;

  plStatus res = ReadDocument(m_sDocumentPath, header, objects, types);
  if (res.Failed())
    return res;

  res = ReadAndRegisterTypes(*types.Borrow());
  if (res.Failed())
    return res;

  {
    PLASMA_PROFILE_SCOPE("Restoring Header");
    plRttiConverterContext context;
    plRttiConverterReader rttiConverter(header.Borrow(), &context);
    auto* pHeaderNode = header->GetNodeByName("Header");
    rttiConverter.ApplyPropertiesToObject(pHeaderNode, m_pDocumentInfo->GetDynamicRTTI(), m_pDocumentInfo);
  }

  {
    PLASMA_PROFILE_SCOPE("Restoring Objects");
    plDocumentObjectConverterReader objectConverter(
      objects.Borrow(), GetObjectManager(), plDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
    // range.BeginNextStep("Restoring Objects");
    auto* pRootNode = objects->GetNodeByName("ObjectTree");
    objectConverter.ApplyPropertiesToObject(pRootNode, GetObjectManager()->GetRootObject());

    SetUnknownObjectTypes(objectConverter.GetUnknownObjectTypes(), objectConverter.GetNumUnknownObjectCreations());
  }

  {
    PLASMA_PROFILE_SCOPE("Restoring Meta-Data");
    // range.BeginNextStep("Restoring Meta-Data");
    RestoreMetaDataAfterLoading(*objects.Borrow(), false);
  }

  SetModified(false);
  return plStatus(PLASMA_SUCCESS);
}

void plDocument::AttachMetaDataBeforeSaving(plAbstractObjectGraph& graph) const
{
  m_DocumentObjectMetaData->AttachMetaDataToAbstractGraph(graph);
}

void plDocument::RestoreMetaDataAfterLoading(const plAbstractObjectGraph& graph, bool bUndoable)
{
  m_DocumentObjectMetaData->RestoreMetaDataFromAbstractGraph(graph);
}

void plDocument::BeforeClosing()
{
  // This can't be done in the dtor as the task uses virtual functions on this object.
  if (m_ActiveSaveTask.IsValid())
  {
    plTaskSystem::WaitForGroup(m_ActiveSaveTask);
    m_ActiveSaveTask.Invalidate();
  }
}

void plDocument::SetUnknownObjectTypes(const plSet<plString>& Types, plUInt32 uiInstances)
{
  m_UnknownObjectTypes = Types;
  m_uiUnknownObjectTypeInstances = uiInstances;
}


void plDocument::BroadcastInterDocumentMessage(plReflectedClass* pMessage, plDocument* pSender)
{
  for (auto& man : plDocumentManager::GetAllDocumentManagers())
  {
    for (auto pDoc : man->GetAllOpenDocuments())
    {
      if (pDoc == pSender)
        continue;

      pDoc->OnInterDocumentMessage(pMessage, pSender);
    }
  }
}

void plDocument::DeleteSelectedObjects() const
{
  auto objects = GetSelectionManager()->GetTopLevelSelection();

  // make sure the whole selection is cleared, otherwise each delete command would reduce the selection one by one
  GetSelectionManager()->Clear();

  auto history = GetCommandHistory();
  history->StartTransaction("Delete Object");

  plRemoveObjectCommand cmd;

  for (const plDocumentObject* pObject : objects)
  {
    cmd.m_Object = pObject->GetGuid();

    if (history->AddCommand(cmd).m_Result.Failed())
    {
      history->CancelTransaction();
      return;
    }
  }

  history->FinishTransaction();
}

void plDocument::ShowDocumentStatus(const plFormatString& msg) const
{
  plStringBuilder tmp;

  plDocumentEvent e;
  e.m_pDocument = this;
  e.m_szStatusMsg = msg.GetTextCStr(tmp);
  e.m_Type = plDocumentEvent::Type::DocumentStatusMsg;

  m_EventsOne.Broadcast(e);
}


plResult plDocument::ComputeObjectTransformation(const plDocumentObject* pObject, plTransform& out_Result) const
{
  out_Result.SetIdentity();
  return PLASMA_FAILURE;
}

plObjectAccessorBase* plDocument::GetObjectAccessor() const
{
  return m_pObjectAccessor.Borrow();
}
