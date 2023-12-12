#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Document/DocumentUtils.h>
#include <ToolsFoundation/Project/ToolsProject.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDocumentManager, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, DocumentManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plPlugin::Events().AddEventHandler(plDocumentManager::OnPluginEvent);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plPlugin::Events().RemoveEventHandler(plDocumentManager::OnPluginEvent);
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

plSet<const plRTTI*> plDocumentManager::s_KnownManagers;
plHybridArray<plDocumentManager*, 16> plDocumentManager::s_AllDocumentManagers;
plMap<plString, const plDocumentTypeDescriptor*> plDocumentManager::s_AllDocumentDescriptors; // maps from "sDocumentTypeName" to descriptor
plCopyOnBroadcastEvent<const plDocumentManager::Event&> plDocumentManager::s_Events;
plEvent<plDocumentManager::Request&> plDocumentManager::s_Requests;
plMap<plString, plDocumentManager::CustomAction> plDocumentManager::s_CustomActions;

void plDocumentManager::OnPluginEvent(const plPluginEvent& e)
{
  switch (e.m_EventType)
  {
    case plPluginEvent::BeforeUnloading:
      UpdateBeforeUnloadingPlugins(e);
      break;
    case plPluginEvent::AfterPluginChanges:
      UpdatedAfterLoadingPlugins();
      break;

    default:
      break;
  }
}

void plDocumentManager::UpdateBeforeUnloadingPlugins(const plPluginEvent& e)
{
  bool bChanges = false;

  // triggers a reevaluation next time
  s_AllDocumentDescriptors.Clear();

  // remove all document managers that belong to this plugin
  for (plUInt32 i = 0; i < s_AllDocumentManagers.GetCount();)
  {
    const plRTTI* pRtti = s_AllDocumentManagers[i]->GetDynamicRTTI();

    if (pRtti->GetPluginName() == e.m_sPluginBinary)
    {
      s_KnownManagers.Remove(pRtti);

      pRtti->GetAllocator()->Deallocate(s_AllDocumentManagers[i]);
      s_AllDocumentManagers.RemoveAtAndSwap(i);

      bChanges = true;
    }
    else
      ++i;
  }

  if (bChanges)
  {
    Event e2;
    e2.m_Type = Event::Type::DocumentTypesRemoved;
    s_Events.Broadcast(e2);
  }
}

void plDocumentManager::UpdatedAfterLoadingPlugins()
{
  bool bChanges = false;

  plRTTI::ForEachDerivedType<plDocumentManager>(
  [&](const plRTTI* pRtti) {
    // add the ones that we don't know yet
    if (!s_KnownManagers.Find(pRtti).IsValid())
    {
      // add it as 'known' even if we cannot allocate it
      s_KnownManagers.Insert(pRtti);
      if (pRtti->GetAllocator()->CanAllocate())
      {
        // create one instance of each manager type
        plDocumentManager* pManager = pRtti->GetAllocator()->Allocate<plDocumentManager>();
        s_AllDocumentManagers.PushBack(pManager);

        bChanges = true;
      }
    }
  });

  // triggers a reevaluation next time
  s_AllDocumentDescriptors.Clear();
  GetAllDocumentDescriptors();


  if (bChanges)
  {
    Event e;
    e.m_Type = Event::Type::DocumentTypesAdded;
    s_Events.Broadcast(e);
  }
}

void plDocumentManager::GetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  InternalGetSupportedDocumentTypes(inout_DocumentTypes);

  for (auto& dt : inout_DocumentTypes)
  {
    PLASMA_ASSERT_DEBUG(dt->m_bCanCreate == false || dt->m_pDocumentType != nullptr, "No document type is set");
    PLASMA_ASSERT_DEBUG(!dt->m_sFileExtension.IsEmpty(), "File extension must be valid");
    PLASMA_ASSERT_DEBUG(dt->m_pManager != nullptr, "Document manager must be set");
  }
}

plStatus plDocumentManager::CanOpenDocument(const char* szFilePath) const
{
  plHybridArray<const plDocumentTypeDescriptor*, 4> DocumentTypes;
  GetSupportedDocumentTypes(DocumentTypes);

  plStringBuilder sPath = szFilePath;
  plStringBuilder sExt = sPath.GetFileExtension();

  // check whether the file extension is in the list of possible extensions
  // if not, we can definitely not open this file
  for (plUInt32 i = 0; i < DocumentTypes.GetCount(); ++i)
  {
    if (DocumentTypes[i]->m_sFileExtension.IsEqual_NoCase(sExt))
    {
      return plStatus(PLASMA_SUCCESS);
    }
  }

  return plStatus("File extension is not handled by any registered type");
}

void plDocumentManager::EnsureWindowRequested(plDocument* pDocument, const plDocumentObject* pOpenContext /*= nullptr*/)
{
  if (pDocument->m_bWindowRequested)
    return;

  PLASMA_PROFILE_SCOPE("EnsureWindowRequested");
  pDocument->m_bWindowRequested = true;

  Event e;
  e.m_pDocument = pDocument;
  e.m_Type = Event::Type::DocumentWindowRequested;
  e.m_pOpenContext = pOpenContext;
  s_Events.Broadcast(e);

  e.m_pDocument = pDocument;
  e.m_Type = Event::Type::AfterDocumentWindowRequested;
  e.m_pOpenContext = pOpenContext;
  s_Events.Broadcast(e);
}

plStatus plDocumentManager::CreateOrOpenDocument(bool bCreate, const char* szDocumentTypeName, const char* szPath, plDocument*& out_pDocument,
  plBitflags<plDocumentFlags> flags, const plDocumentObject* pOpenContext /*= nullptr*/)
{
  plFileStats fs;
  plStringBuilder sPath = szPath;
  sPath.MakeCleanPath();
  if (!bCreate && plOSFile::GetFileStats(sPath, fs).Failed())
  {
    return plStatus("The file does not exist.");
  }

  Request r;
  r.m_Type = Request::Type::DocumentAllowedToOpen;
  r.m_RequestStatus.m_Result = PLASMA_SUCCESS;
  r.m_sDocumentType = szDocumentTypeName;
  r.m_sDocumentPath = sPath;
  s_Requests.Broadcast(r);

  // if for example no project is open, or not the correct one, then a document cannot be opened
  if (r.m_RequestStatus.m_Result.Failed())
    return r.m_RequestStatus;

  out_pDocument = nullptr;

  plStatus status;

  plHybridArray<const plDocumentTypeDescriptor*, 4> DocumentTypes;
  GetSupportedDocumentTypes(DocumentTypes);

  for (plUInt32 i = 0; i < DocumentTypes.GetCount(); ++i)
  {
    if (DocumentTypes[i]->m_sDocumentTypeName == szDocumentTypeName)
    {
      // See if there is a default asset document registered for the type, if so clone
      // it and use that as the new document instead of creating one from scratch.
      if (bCreate && !flags.IsSet(plDocumentFlags::EmptyDocument))
      {
        plStringBuilder sTemplateDoc = "Editor/DocumentTemplates/Default";
        sTemplateDoc.ChangeFileExtension(sPath.GetFileExtension());

        if (plFileSystem::ExistsFile(sTemplateDoc))
        {
          plUuid CloneUuid;
          if (CloneDocument(sTemplateDoc, sPath, CloneUuid).Succeeded())
          {
            if (OpenDocument(szDocumentTypeName, sPath, out_pDocument, flags, pOpenContext).Succeeded())
            {
              return plStatus(PLASMA_SUCCESS);
            }
          }

          plLog::Warning("Failed to create document from template '{}'", sTemplateDoc);
        }
      }

      PLASMA_ASSERT_DEV(DocumentTypes[i]->m_bCanCreate, "This document manager cannot create the document type '{0}'", szDocumentTypeName);

      {
        PLASMA_PROFILE_SCOPE(szDocumentTypeName);
        status = plStatus(PLASMA_SUCCESS);
        InternalCreateDocument(szDocumentTypeName, sPath, bCreate, out_pDocument, pOpenContext);
      }
      out_pDocument->SetAddToResetFilesList(flags.IsSet(plDocumentFlags::AddToRecentFilesList));

      if (status.m_Result.Succeeded())
      {
        out_pDocument->SetupDocumentInfo(DocumentTypes[i]);

        out_pDocument->m_pDocumentManager = this;
        m_AllOpenDocuments.PushBack(out_pDocument);

        if (!bCreate)
        {
          status = out_pDocument->LoadDocument();
        }

        {
          PLASMA_PROFILE_SCOPE("InitializeAfterLoading");
          out_pDocument->InitializeAfterLoading(bCreate);
        }

        if (bCreate)
        {
          out_pDocument->SetModified(true);
          if (flags.IsSet(plDocumentFlags::AsyncSave))
          {
            out_pDocument->SaveDocumentAsync({});
            status = plStatus(PLASMA_SUCCESS);
          }
          else
          {
            status = out_pDocument->SaveDocument();
          }
        }

        {
          PLASMA_PROFILE_SCOPE("InitializeAfterLoadingAndSaving");
          out_pDocument->InitializeAfterLoadingAndSaving();
        }

        Event e;
        e.m_pDocument = out_pDocument;
        e.m_Type = Event::Type::DocumentOpened;

        s_Events.Broadcast(e);

        if (flags.IsSet(plDocumentFlags::RequestWindow))
          EnsureWindowRequested(out_pDocument, pOpenContext);
      }

      return status;
    }
  }

  PLASMA_REPORT_FAILURE("This document manager does not support the document type '{0}'", szDocumentTypeName);
  return status;
}

plStatus plDocumentManager::CreateDocument(
  const char* szDocumentTypeName, const char* szPath, plDocument*& out_pDocument, plBitflags<plDocumentFlags> flags, const plDocumentObject* pOpenContext)
{
  return CreateOrOpenDocument(true, szDocumentTypeName, szPath, out_pDocument, flags, pOpenContext);
}

plStatus plDocumentManager::OpenDocument(const char* szDocumentTypeName, const char* szPath, plDocument*& out_pDocument,
  plBitflags<plDocumentFlags> flags, const plDocumentObject* pOpenContext)
{
  return CreateOrOpenDocument(false, szDocumentTypeName, szPath, out_pDocument, flags, pOpenContext);
}


plStatus plDocumentManager::CloneDocument(const char* szPath, const char* szClonePath, plUuid& inout_cloneGuid)
{
  const plDocumentTypeDescriptor* pTypeDesc = nullptr;
  plStatus res = plDocumentUtils::IsValidSaveLocationForDocument(szClonePath, &pTypeDesc);
  if (res.Failed())
    return res;

  plUniquePtr<plAbstractObjectGraph> header;
  plUniquePtr<plAbstractObjectGraph> objects;
  plUniquePtr<plAbstractObjectGraph> types;

  res = plDocument::ReadDocument(szPath, header, objects, types);
  if (res.Failed())
    return res;

  plUuid documentId;
  plAbstractObjectNode::Property* documentIdProp = nullptr;
  {
    auto* pHeaderNode = header->GetNodeByName("Header");
    PLASMA_ASSERT_DEV(pHeaderNode, "No header found, document '{0}' is corrupted.", szPath);
    documentIdProp = pHeaderNode->FindProperty("DocumentID");
    PLASMA_ASSERT_DEV(documentIdProp, "No document ID property found in header, document document '{0}' is corrupted.", szPath);
    documentId = documentIdProp->m_Value.Get<plUuid>();
  }

  plUuid seedGuid;
  if (inout_cloneGuid.IsValid())
  {
    seedGuid = inout_cloneGuid;
    seedGuid.RevertCombinationWithSeed(documentId);

    plUuid test = documentId;
    test.CombineWithSeed(seedGuid);
    PLASMA_ASSERT_DEV(test == inout_cloneGuid, "");
  }
  else
  {
    seedGuid.CreateNewUuid();
    inout_cloneGuid = documentId;
    inout_cloneGuid.CombineWithSeed(seedGuid);
  }

  InternalCloneDocument(szPath, szClonePath, documentId, seedGuid, inout_cloneGuid, header.Borrow(), objects.Borrow(), types.Borrow());

  {
    plDeferredFileWriter file;
    file.SetOutput(szClonePath);
    plAbstractGraphDdlSerializer::WriteDocument(file, header.Borrow(), objects.Borrow(), types.Borrow(), false);
    if (file.Close() == PLASMA_FAILURE)
    {
      return plStatus(plFmt("Unable to open file '{0}' for writing!", szClonePath));
    }
  }
  return plStatus(PLASMA_SUCCESS);
}

void plDocumentManager::InternalCloneDocument(const char* szPath, const char* szClonePath, const plUuid& documentId, const plUuid& seedGuid, const plUuid& cloneGuid, plAbstractObjectGraph* header, plAbstractObjectGraph* objects, plAbstractObjectGraph* types)
{
  // Remap
  header->ReMapNodeGuids(seedGuid);
  objects->ReMapNodeGuids(seedGuid);

  auto* pHeaderNode = header->GetNodeByName("Header");
  auto* documentIdProp = pHeaderNode->FindProperty("DocumentID");
  documentIdProp->m_Value = cloneGuid;

  // Fix cloning of docs containing prefabs.
  // TODO: generalize this for other doc features?
  auto& AllNodes = objects->GetAllNodes();
  for (auto it = AllNodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    plAbstractObjectNode::Property* pProp = pNode->FindProperty("MetaPrefabSeed");
    if (pProp && pProp->m_Value.IsA<plUuid>())
    {
      plUuid prefabSeed = pProp->m_Value.Get<plUuid>();
      prefabSeed.CombineWithSeed(seedGuid);
      pProp->m_Value = prefabSeed;
    }
  }
}

void plDocumentManager::CloseDocument(plDocument* pDocument)
{
  PLASMA_ASSERT_DEV(pDocument != nullptr, "Invalid document pointer");

  if (!m_AllOpenDocuments.RemoveAndCopy(pDocument))
    return;

  Event e;
  e.m_pDocument = pDocument;
  e.m_Type = Event::Type::DocumentClosing;
  s_Events.Broadcast(e);

  e.m_pDocument = pDocument;
  e.m_Type = Event::Type::DocumentClosing2;
  s_Events.Broadcast(e);

  pDocument->BeforeClosing();
  delete pDocument;

  e.m_pDocument = pDocument;
  e.m_Type = Event::Type::DocumentClosed;
  s_Events.Broadcast(e);
}

void plDocumentManager::CloseAllDocumentsOfManager()
{
  while (!m_AllOpenDocuments.IsEmpty())
  {
    CloseDocument(m_AllOpenDocuments[0]);
  }
}

void plDocumentManager::CloseAllDocuments()
{
  for (plDocumentManager* pMan : s_AllDocumentManagers)
  {
    pMan->CloseAllDocumentsOfManager();
  }
}

plDocument* plDocumentManager::GetDocumentByPath(const char* szPath) const
{
  plStringBuilder sPath = szPath;
  sPath.MakeCleanPath();

  for (plDocument* pDoc : m_AllOpenDocuments)
  {
    if (sPath.IsEqual_NoCase(pDoc->GetDocumentPath()))
      return pDoc;
  }

  return nullptr;
}


plDocument* plDocumentManager::GetDocumentByGuid(const plUuid& guid)
{
  for (auto man : s_AllDocumentManagers)
  {
    for (auto doc : man->m_AllOpenDocuments)
    {
      if (doc->GetGuid() == guid)
        return doc;
    }
  }

  return nullptr;
}


bool plDocumentManager::EnsureDocumentIsClosedInAllManagers(const char* szPath)
{
  bool bClosedAny = false;
  for (auto man : s_AllDocumentManagers)
  {
    if (man->EnsureDocumentIsClosed(szPath))
      bClosedAny = true;
  }

  return bClosedAny;
}

bool plDocumentManager::EnsureDocumentIsClosed(const char* szPath)
{
  auto pDoc = GetDocumentByPath(szPath);

  if (pDoc == nullptr)
    return false;

  CloseDocument(pDoc);

  return true;
}

plResult plDocumentManager::FindDocumentTypeFromPath(const char* szPath, bool bForCreation, const plDocumentTypeDescriptor*& out_pTypeDesc)
{
  const plString sFileExt = plPathUtils::GetFileExtension(szPath);

  const auto& allDesc = GetAllDocumentDescriptors();

  for (auto it : allDesc)
  {
    const auto* desc = it.Value();

    if (bForCreation && !desc->m_bCanCreate)
      continue;

    if (desc->m_sFileExtension.IsEqual_NoCase(sFileExt))
    {
      out_pTypeDesc = desc;
      return PLASMA_SUCCESS;
    }
  }

  return PLASMA_FAILURE;
}

const plMap<plString, const plDocumentTypeDescriptor*>& plDocumentManager::GetAllDocumentDescriptors()
{
  if (s_AllDocumentDescriptors.IsEmpty())
  {
    for (plDocumentManager* pMan : plDocumentManager::GetAllDocumentManagers())
    {
      plHybridArray<const plDocumentTypeDescriptor*, 4> descriptors;
      pMan->GetSupportedDocumentTypes(descriptors);

      for (auto pDesc : descriptors)
      {
        s_AllDocumentDescriptors[pDesc->m_sDocumentTypeName] = pDesc;
      }
    }
  }

  return s_AllDocumentDescriptors;
}

const plDocumentTypeDescriptor* plDocumentManager::GetDescriptorForDocumentType(const char* szDocumentType)
{
  return GetAllDocumentDescriptors().GetValueOrDefault(szDocumentType, nullptr);
}

/// \todo on close doc: remove from m_AllDocuments
