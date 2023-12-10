#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <ToolsFoundation/Command/TreeCommands.h>


void plSceneDocument::UnlinkPrefabs(const plDeque<const plDocumentObject*>& selection)
{
  SUPER::UnlinkPrefabs(selection);

  // Clear cached names.
  for (auto pObject : selection)
  {
    auto pMetaScene = m_GameObjectMetaData->BeginModifyMetaData(pObject->GetGuid());
    pMetaScene->m_CachedNodeName.Clear();
    m_GameObjectMetaData->EndModifyMetaData(plGameObjectMetaData::CachedName);
  }
}


bool plSceneDocument::IsObjectEditorPrefab(const plUuid& object, plUuid* out_pPrefabAssetGuid) const
{
  auto pMeta = m_DocumentObjectMetaData->BeginReadMetaData(object);
  const bool bIsPrefab = pMeta->m_CreateFromPrefab.IsValid();

  if (out_pPrefabAssetGuid)
  {
    *out_pPrefabAssetGuid = pMeta->m_CreateFromPrefab;
  }

  m_DocumentObjectMetaData->EndReadMetaData();

  return bIsPrefab;
}


bool plSceneDocument::IsObjectEnginePrefab(const plUuid& object, plUuid* out_pPrefabAssetGuid) const
{
  const plDocumentObject* pObject = GetObjectManager()->GetObject(object);

  plHybridArray<plVariant, 16> values;
  pObject->GetTypeAccessor().GetValues("Components", values);

  for (plVariant& value : values)
  {
    auto pChild = GetObjectManager()->GetObject(value.Get<plUuid>());

    // search for prefab components
    if (pChild->GetTypeAccessor().GetType()->IsDerivedFrom<plPrefabReferenceComponent>())
    {
      plVariant varPrefab = pChild->GetTypeAccessor().GetValue("Prefab");

      if (varPrefab.IsA<plString>())
      {
        if (out_pPrefabAssetGuid)
        {
          const plString sAsset = varPrefab.Get<plString>();

          const auto info = plAssetCurator::GetSingleton()->FindSubAsset(sAsset);

          if (info.isValid())
          {
            *out_pPrefabAssetGuid = info->m_Data.m_Guid;
          }
        }

        return true;
      }
    }
  }

  return false;
}

void plSceneDocument::UpdatePrefabs()
{
  PLASMA_LOCK(m_GameObjectMetaData->GetMutex());
  SUPER::UpdatePrefabs();
}


plUuid plSceneDocument::ReplaceByPrefab(const plDocumentObject* pRootObject, plStringView sPrefabFile, const plUuid& prefabAsset, const plUuid& prefabSeed, bool bEnginePrefab)
{
  plUuid newGuid = SUPER::ReplaceByPrefab(pRootObject, sPrefabFile, prefabAsset, prefabSeed, bEnginePrefab);
  if (newGuid.IsValid())
  {
    auto pMeta = m_GameObjectMetaData->BeginModifyMetaData(newGuid);
    pMeta->m_CachedNodeName.Clear();
    m_GameObjectMetaData->EndModifyMetaData(plGameObjectMetaData::CachedName);
  }
  return newGuid;
}

plUuid plSceneDocument::RevertPrefab(const plDocumentObject* pObject)
{
  auto pHistory = GetCommandHistory();
  const plVec3 vLocalPos = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<plVec3>();
  const plQuat vLocalRot = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<plQuat>();
  const plVec3 vLocalScale = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<plVec3>();
  const float fLocalUniformScale = pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

  plUuid newGuid = SUPER::RevertPrefab(pObject);

  if (newGuid.IsValid())
  {
    plSetObjectPropertyCommand setCmd;
    setCmd.m_Object = newGuid;

    setCmd.m_sProperty = "LocalPosition";
    setCmd.m_NewValue = vLocalPos;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalRotation";
    setCmd.m_NewValue = vLocalRot;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalScaling";
    setCmd.m_NewValue = vLocalScale;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalUniformScaling";
    setCmd.m_NewValue = fLocalUniformScale;
    pHistory->AddCommand(setCmd).AssertSuccess();
  }
  return newGuid;
}

void plSceneDocument::UpdatePrefabObject(plDocumentObject* pObject, const plUuid& PrefabAsset, const plUuid& PrefabSeed, plStringView sBasePrefab)
{
  auto pHistory = GetCommandHistory();
  const plVec3 vLocalPos = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<plVec3>();
  const plQuat vLocalRot = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<plQuat>();
  const plVec3 vLocalScale = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<plVec3>();
  const float fLocalUniformScale = pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

  SUPER::UpdatePrefabObject(pObject, PrefabAsset, PrefabSeed, sBasePrefab);

  // the root object has the same GUID as the PrefabSeed
  if (PrefabSeed.IsValid())
  {
    plSetObjectPropertyCommand setCmd;
    setCmd.m_Object = PrefabSeed;

    setCmd.m_sProperty = "LocalPosition";
    setCmd.m_NewValue = vLocalPos;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalRotation";
    setCmd.m_NewValue = vLocalRot;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalScaling";
    setCmd.m_NewValue = vLocalScale;
    pHistory->AddCommand(setCmd).AssertSuccess();

    setCmd.m_sProperty = "LocalUniformScaling";
    setCmd.m_NewValue = fLocalUniformScale;
    pHistory->AddCommand(setCmd).AssertSuccess();
  }
}

void plSceneDocument::ConvertToEditorPrefab(const plDeque<const plDocumentObject*>& selection)
{
  plDeque<const plDocumentObject*> newSelection;

  auto pHistory = GetCommandHistory();
  pHistory->StartTransaction("Convert to Editor Prefab");

  for (const plDocumentObject* pObject : selection)
  {
    plUuid assetGuid;
    if (!IsObjectEnginePrefab(pObject->GetGuid(), &assetGuid))
      continue;

    auto pAsset = plAssetCurator::GetSingleton()->GetSubAsset(assetGuid);

    if (!pAsset.isValid())
      continue;

    const plTransform transform = GetGlobalTransform(pObject);

    plUuid newGuid = plUuid::MakeUuid();
    plUuid newObject = ReplaceByPrefab(pObject, pAsset->m_pAssetInfo->m_Path.GetAbsolutePath(), assetGuid, newGuid, false);

    if (newObject.IsValid())
    {
      const plDocumentObject* pNewObject = GetObjectManager()->GetObject(newObject);
      SetGlobalTransform(pNewObject, transform, TransformationChanges::All);

      newSelection.PushBack(pNewObject);
    }
  }

  pHistory->FinishTransaction();

  GetSelectionManager()->SetSelection(newSelection);
}

void plSceneDocument::ConvertToEnginePrefab(const plDeque<const plDocumentObject*>& selection)
{
  plDeque<const plDocumentObject*> newSelection;

  auto pHistory = GetCommandHistory();
  pHistory->StartTransaction("Convert to Engine Prefab");

  plStringBuilder tmp;

  for (const plDocumentObject* pObject : selection)
  {
    plUuid assetGuid;
    if (!IsObjectEditorPrefab(pObject->GetGuid(), &assetGuid))
      continue;

    auto pAsset = plAssetCurator::GetSingleton()->GetSubAsset(assetGuid);

    if (!pAsset.isValid())
      continue;

    const plTransform transform = ComputeGlobalTransform(pObject);

    const plDocumentObject* pNewObject = nullptr;

    // create an object with the reference prefab component
    {
      plUuid ObjectGuid, CmpGuid;
      ObjectGuid = plUuid::MakeUuid();
      CmpGuid = plUuid::MakeUuid();

      plAddObjectCommand cmd;
      cmd.m_Parent = (pObject->GetParent() == GetObjectManager()->GetRootObject()) ? plUuid() : pObject->GetParent()->GetGuid();
      cmd.m_Index = pObject->GetPropertyIndex();
      cmd.SetType("plGameObject");
      cmd.m_NewObjectGuid = ObjectGuid;
      cmd.m_sParentProperty = "Children";

      PLASMA_VERIFY(pHistory->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

      cmd.SetType("plPrefabReferenceComponent");
      cmd.m_sParentProperty = "Components";
      cmd.m_Index = -1;
      cmd.m_NewObjectGuid = CmpGuid;
      cmd.m_Parent = ObjectGuid;
      PLASMA_VERIFY(pHistory->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

      plSetObjectPropertyCommand cmd2;
      cmd2.m_Object = CmpGuid;
      cmd2.m_sProperty = "Prefab";
      cmd2.m_NewValue = plConversionUtils::ToString(assetGuid, tmp).GetData();
      PLASMA_VERIFY(pHistory->AddCommand(cmd2).m_Result.Succeeded(), "AddCommand failed");


      pNewObject = GetObjectManager()->GetObject(ObjectGuid);
    }

    // set same position
    SetGlobalTransform(pNewObject, transform, TransformationChanges::All);

    newSelection.PushBack(pNewObject);

    // delete old object
    {
      plRemoveObjectCommand rem;
      rem.m_Object = pObject->GetGuid();

      PLASMA_VERIFY(pHistory->AddCommand(rem).m_Result.Succeeded(), "AddCommand failed");
    }
  }

  pHistory->FinishTransaction();

  GetSelectionManager()->SetSelection(newSelection);
}
