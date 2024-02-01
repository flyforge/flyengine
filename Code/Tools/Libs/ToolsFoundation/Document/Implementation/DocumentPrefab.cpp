#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

void plDocument::UpdatePrefabs()
{
  GetCommandHistory()->StartTransaction("Update Prefabs");

  UpdatePrefabsRecursive(GetObjectManager()->GetRootObject());

  GetCommandHistory()->FinishTransaction();

  ShowDocumentStatus("Prefabs have been updated");
  SetModified(true);
}

void plDocument::RevertPrefabs(const plDeque<const plDocumentObject*>& selection)
{
  if (selection.IsEmpty())
    return;

  auto pHistory = GetCommandHistory();

  pHistory->StartTransaction("Revert Prefab");

  for (auto pItem : selection)
  {
    RevertPrefab(pItem);
  }

  pHistory->FinishTransaction();
}

void plDocument::UnlinkPrefabs(const plDeque<const plDocumentObject*>& selection)
{
  if (selection.IsEmpty())
    return;

  auto pHistory = GetCommandHistory();
  pHistory->StartTransaction("Unlink Prefab");

  for (auto pObject : selection)
  {
    plUnlinkPrefabCommand cmd;
    cmd.m_Object = pObject->GetGuid();

    pHistory->AddCommand(cmd).AssertSuccess();
  }

  pHistory->FinishTransaction();
}

plStatus plDocument::CreatePrefabDocumentFromSelection(plStringView sFile, const plRTTI* pRootType, plDelegate<void(plAbstractObjectNode*)> adjustGraphNodeCB, plDelegate<void(plDocumentObject*)> adjustNewNodesCB, plDelegate<void(plAbstractObjectGraph& graph, plDynamicArray<plAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB)
{
  auto Selection = GetSelectionManager()->GetTopLevelSelection(pRootType);

  if (Selection.IsEmpty())
    return plStatus("To create a prefab, the selection must not be empty");

  plHybridArray<const plDocumentObject*, 32> nodes;
  nodes.Reserve(Selection.GetCount());
  for (auto pNode : Selection)
  {
    nodes.PushBack(pNode);
  }

  plUuid PrefabGuid, SeedGuid;
  SeedGuid = plUuid::MakeUuid();
  plStatus res = CreatePrefabDocument(sFile, nodes, SeedGuid, PrefabGuid, adjustGraphNodeCB, true, finalizeGraphCB);

  if (res.m_Result.Succeeded())
  {
    GetCommandHistory()->StartTransaction("Replace all by Prefab");

    // this replaces ONE object by the new prefab (we pick the last one in the selection)
    plUuid newObj = ReplaceByPrefab(nodes.PeekBack(), sFile, PrefabGuid, SeedGuid, true);

    // if we had more than one selected objects, remove the others as well
    if (nodes.GetCount() > 1)
    {
      nodes.PopBack();

      for (auto pNode : nodes)
      {
        plRemoveObjectCommand remCmd;
        remCmd.m_Object = pNode->GetGuid();

        GetCommandHistory()->AddCommand(remCmd).AssertSuccess();
      }
    }

    auto pObject = GetObjectManager()->GetObject(newObj);

    if (adjustNewNodesCB.IsValid())
    {
      adjustNewNodesCB(pObject);
    }

    GetCommandHistory()->FinishTransaction();
    GetSelectionManager()->SetSelection(pObject);
  }

  return res;
}

plStatus plDocument::CreatePrefabDocument(plStringView sFile, plArrayPtr<const plDocumentObject*> rootObjects, const plUuid& invPrefabSeed,
  plUuid& out_newDocumentGuid, plDelegate<void(plAbstractObjectNode*)> adjustGraphNodeCB, bool bKeepOpen, plDelegate<void(plAbstractObjectGraph& graph, plDynamicArray<plAbstractObjectNode*>& graphRootNodes)> finalizeGraphCB)
{
  const plDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (plDocumentManager::FindDocumentTypeFromPath(sFile, true, pTypeDesc).Failed())
    return plStatus(plFmt("Document type is unknown: '{0}'", sFile));

  pTypeDesc->m_pManager->EnsureDocumentIsClosed(sFile);

  // prepare the current state as a graph
  plAbstractObjectGraph PrefabGraph;
  plDocumentObjectConverterWriter writer(&PrefabGraph, GetObjectManager());

  plHybridArray<plAbstractObjectNode*, 32> graphRootNodes;
  graphRootNodes.Reserve(rootObjects.GetCount() + 1);

  for (plUInt32 i = 0; i < rootObjects.GetCount(); ++i)
  {
    auto pSaveAsPrefab = rootObjects[i];

    PL_ASSERT_DEV(pSaveAsPrefab != nullptr, "CreatePrefabDocument: pSaveAsPrefab must be a valid object!");

    auto pPrefabGraphMainNode = writer.AddObjectToGraph(pSaveAsPrefab);
    graphRootNodes.PushBack(pPrefabGraphMainNode);

    // allow external adjustments
    if (adjustGraphNodeCB.IsValid())
    {
      adjustGraphNodeCB(pPrefabGraphMainNode);
    }
  }

  if (finalizeGraphCB.IsValid())
  {
    finalizeGraphCB(PrefabGraph, graphRootNodes);
  }

  PrefabGraph.ReMapNodeGuids(invPrefabSeed, true);

  plDocument* pSceneDocument = nullptr;

  PL_SUCCEED_OR_RETURN(pTypeDesc->m_pManager->CreateDocument("Prefab", sFile, pSceneDocument, plDocumentFlags::RequestWindow | plDocumentFlags::AddToRecentFilesList | plDocumentFlags::EmptyDocument));

  out_newDocumentGuid = pSceneDocument->GetGuid();
  auto pPrefabSceneRoot = pSceneDocument->GetObjectManager()->GetRootObject();

  plDocumentObjectConverterReader reader(&PrefabGraph, pSceneDocument->GetObjectManager(), plDocumentObjectConverterReader::Mode::CreateAndAddToDocument);

  for (plUInt32 i = 0; i < graphRootNodes.GetCount(); ++i)
  {
    const plRTTI* pRootType = plRTTI::FindTypeByName(graphRootNodes[i]->GetType());

    plUuid rootGuid = graphRootNodes[i]->GetGuid();
    rootGuid.RevertCombinationWithSeed(invPrefabSeed);

    plDocumentObject* pPrefabSceneMainObject = pSceneDocument->GetObjectManager()->CreateObject(pRootType, rootGuid);
    pSceneDocument->GetObjectManager()->AddObject(pPrefabSceneMainObject, pPrefabSceneRoot, "Children", -1);

    reader.ApplyPropertiesToObject(graphRootNodes[i], pPrefabSceneMainObject);
  }

  pSceneDocument->SetModified(true);
  auto res = pSceneDocument->SaveDocument();

  if (!bKeepOpen)
  {
    pTypeDesc->m_pManager->CloseDocument(pSceneDocument);
  }

  return res;
}


plUuid plDocument::ReplaceByPrefab(const plDocumentObject* pRootObject, plStringView sPrefabFile, const plUuid& prefabAsset, const plUuid& prefabSeed, bool bEnginePrefab)
{
  GetCommandHistory()->StartTransaction("Replace by Prefab");

  plUuid instantiatedRoot;

  if (!bEnginePrefab) // create editor prefab
  {
    plInstantiatePrefabCommand instCmd;
    instCmd.m_Index = pRootObject->GetPropertyIndex().ConvertTo<plInt32>();
    instCmd.m_bAllowPickedPosition = false;
    instCmd.m_CreateFromPrefab = prefabAsset;
    instCmd.m_Parent = pRootObject->GetParent() == GetObjectManager()->GetRootObject() ? plUuid() : pRootObject->GetParent()->GetGuid();
    instCmd.m_sBasePrefabGraph = plPrefabUtils::ReadDocumentAsString(
      sPrefabFile); // since the prefab might have been created just now, going through the cache (via GUID) will most likely fail
    instCmd.m_RemapGuid = prefabSeed;

    GetCommandHistory()->AddCommand(instCmd).AssertSuccess();

    instantiatedRoot = instCmd.m_CreatedRootObject;
  }
  else // create an object with the reference prefab component
  {
    auto pHistory = GetCommandHistory();

    plStringBuilder tmp;
    plUuid CmpGuid = plUuid::MakeUuid();
    instantiatedRoot = plUuid::MakeUuid();

    plAddObjectCommand cmd;
    cmd.m_Parent = (pRootObject->GetParent() == GetObjectManager()->GetRootObject()) ? plUuid() : pRootObject->GetParent()->GetGuid();
    cmd.m_Index = pRootObject->GetPropertyIndex();
    cmd.SetType("plGameObject");
    cmd.m_NewObjectGuid = instantiatedRoot;
    cmd.m_sParentProperty = "Children";

    PL_VERIFY(pHistory->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    cmd.SetType("plPrefabReferenceComponent");
    cmd.m_sParentProperty = "Components";
    cmd.m_Index = -1;
    cmd.m_NewObjectGuid = CmpGuid;
    cmd.m_Parent = instantiatedRoot;
    PL_VERIFY(pHistory->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    plSetObjectPropertyCommand cmd2;
    cmd2.m_Object = CmpGuid;
    cmd2.m_sProperty = "Prefab";
    cmd2.m_NewValue = plConversionUtils::ToString(prefabAsset, tmp).GetData();
    PL_VERIFY(pHistory->AddCommand(cmd2).m_Result.Succeeded(), "AddCommand failed");
  }

  {
    plRemoveObjectCommand remCmd;
    remCmd.m_Object = pRootObject->GetGuid();

    GetCommandHistory()->AddCommand(remCmd).AssertSuccess();
  }

  GetCommandHistory()->FinishTransaction();

  return instantiatedRoot;
}

plUuid plDocument::RevertPrefab(const plDocumentObject* pObject)
{
  auto pHistory = GetCommandHistory();
  auto pMeta = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid());

  const plUuid PrefabAsset = pMeta->m_CreateFromPrefab;

  if (!PrefabAsset.IsValid())
  {
    m_DocumentObjectMetaData->EndReadMetaData();
    return plUuid();
  }

  plRemoveObjectCommand remCmd;
  remCmd.m_Object = pObject->GetGuid();

  plInstantiatePrefabCommand instCmd;
  instCmd.m_Index = pObject->GetPropertyIndex().ConvertTo<plInt32>();
  instCmd.m_bAllowPickedPosition = false;
  instCmd.m_CreateFromPrefab = PrefabAsset;
  instCmd.m_Parent = pObject->GetParent() == GetObjectManager()->GetRootObject() ? plUuid() : pObject->GetParent()->GetGuid();
  instCmd.m_RemapGuid = pMeta->m_PrefabSeedGuid;
  instCmd.m_sBasePrefabGraph = plPrefabCache::GetSingleton()->GetCachedPrefabDocument(pMeta->m_CreateFromPrefab);

  m_DocumentObjectMetaData->EndReadMetaData();

  pHistory->AddCommand(remCmd).AssertSuccess();
  pHistory->AddCommand(instCmd).AssertSuccess();

  return instCmd.m_CreatedRootObject;
}


void plDocument::UpdatePrefabsRecursive(plDocumentObject* pObject)
{
  // Deliberately copy the array as the UpdatePrefabObject function will add / remove elements from the array.
  auto ChildArray = pObject->GetChildren();

  plStringBuilder sPrefabBase;

  for (auto pChild : ChildArray)
  {
    auto pMeta = m_DocumentObjectMetaData->BeginReadMetaData(pChild->GetGuid());
    const plUuid PrefabAsset = pMeta->m_CreateFromPrefab;
    const plUuid PrefabSeed = pMeta->m_PrefabSeedGuid;
    sPrefabBase = pMeta->m_sBasePrefab;

    m_DocumentObjectMetaData->EndReadMetaData();

    // if this is a prefab instance, update it
    if (PrefabAsset.IsValid())
    {
      UpdatePrefabObject(pChild, PrefabAsset, PrefabSeed, sPrefabBase);
    }
    else
    {
      // only recurse if no prefab was found
      // nested prefabs are not allowed
      UpdatePrefabsRecursive(pChild);
    }
  }
}

void plDocument::UpdatePrefabObject(plDocumentObject* pObject, const plUuid& PrefabAsset, const plUuid& PrefabSeed, plStringView sBasePrefab)
{
  const plStringBuilder& sNewBasePrefab = plPrefabCache::GetSingleton()->GetCachedPrefabDocument(PrefabAsset);

  plStringBuilder sNewMergedGraph;
  plPrefabUtils::Merge(sBasePrefab, sNewBasePrefab, pObject, true, PrefabSeed, sNewMergedGraph);

  // remove current object
  plRemoveObjectCommand rm;
  rm.m_Object = pObject->GetGuid();

  // instantiate prefab again
  plInstantiatePrefabCommand inst;
  inst.m_Index = pObject->GetPropertyIndex().ConvertTo<plInt32>();
  inst.m_bAllowPickedPosition = false;
  inst.m_CreateFromPrefab = PrefabAsset;
  inst.m_Parent = pObject->GetParent() == GetObjectManager()->GetRootObject() ? plUuid() : pObject->GetParent()->GetGuid();
  inst.m_RemapGuid = PrefabSeed;
  inst.m_sBasePrefabGraph = sNewBasePrefab;
  inst.m_sObjectGraph = sNewMergedGraph;

  GetCommandHistory()->AddCommand(rm).AssertSuccess();
  GetCommandHistory()->AddCommand(inst).AssertSuccess();
}
