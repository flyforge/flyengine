#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorPluginScene/Scene/SceneDocumentManager.h>
#include <Foundation/Strings/PathUtils.h>
#include <ToolsFoundation/Command/TreeCommands.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSceneDocumentManager, 1, plRTTIDefaultAllocator<plSceneDocumentManager>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;


plSceneDocumentManager::plSceneDocumentManager()
{
  // Document type descriptor for a standard PLASMA scene
  {
    auto& docTypeDesc = m_DocTypeDescs.ExpandAndGetRef();
    docTypeDesc.m_sDocumentTypeName = "Scene";
    docTypeDesc.m_sFileExtension = "plScene";
    docTypeDesc.m_sIcon = ":/AssetIcons/Scene.svg";
    docTypeDesc.m_pDocumentType = plGetStaticRTTI<plScene2Document>();
    docTypeDesc.m_pManager = this;
    docTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Scene");

    docTypeDesc.m_sResourceFileExtension = "plObjectGraph";
    docTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::OnlyTransformManually | plAssetDocumentFlags::SupportsThumbnail;
  }

  // Document type descriptor for a prefab
  {
    auto& docTypeDesc = m_DocTypeDescs.ExpandAndGetRef();

    docTypeDesc.m_sDocumentTypeName = "Prefab";
    docTypeDesc.m_sFileExtension = "plPrefab";
    docTypeDesc.m_sIcon = ":/AssetIcons/Prefab.svg";
    docTypeDesc.m_pDocumentType = plGetStaticRTTI<plSceneDocument>();
    docTypeDesc.m_pManager = this;
    docTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Prefab");

    docTypeDesc.m_sResourceFileExtension = "plObjectGraph";
    docTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::AutoTransformOnSave | plAssetDocumentFlags::SupportsThumbnail;
  }

  // Document type descriptor for a layer (similar to a normal scene) as it holds a scene object graph
  {
    auto& docTypeDesc = m_DocTypeDescs.ExpandAndGetRef();
    docTypeDesc.m_sDocumentTypeName = "Layer";
    docTypeDesc.m_sFileExtension = "plSceneLayer";
    docTypeDesc.m_sIcon = ":/AssetIcons/Layer.svg";
    docTypeDesc.m_pDocumentType = plGetStaticRTTI<plLayerDocument>();
    docTypeDesc.m_pManager = this;
    docTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Scene_Layer");

    docTypeDesc.m_sResourceFileExtension = "";
    // A layer can not be transformed individually (at least at the moment)
    // all layers for a scene are gathered and put into one cohesive runtime scene
    docTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::DisableTransform; // TODO: Disable creation in "New Document"?
  }
}

void plSceneDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  if (plStringUtils::IsEqual(szDocumentTypeName, "Scene"))
  {
    out_pDocument = new plScene2Document(szPath);

    if (bCreateNewDocument)
    {
      SetupDefaultScene(out_pDocument);
    }
  }
  else if (plStringUtils::IsEqual(szDocumentTypeName, "Prefab"))
  {
    out_pDocument = new plSceneDocument(szPath, plSceneDocument::DocumentType::Prefab);
  }
  else if (plStringUtils::IsEqual(szDocumentTypeName, "Layer"))
  {
    if (pOpenContext == nullptr)
    {
      // Opened individually
      out_pDocument = new plSceneDocument(szPath, plSceneDocument::DocumentType::Layer);
    }
    else
    {
      // Opened via a parent scene document
      plScene2Document* pDoc = const_cast<plScene2Document*>(plDynamicCast<const plScene2Document*>(pOpenContext->GetDocumentObjectManager()->GetDocument()));
      out_pDocument = new plLayerDocument(szPath, pDoc);
    }
  }
}

void plSceneDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  for (auto& docTypeDesc : m_DocTypeDescs)
  {
    inout_DocumentTypes.PushBack(&docTypeDesc);
  }
}

void plSceneDocumentManager::InternalCloneDocument(const char* szPath, const char* szClonePath, const plUuid& documentId, const plUuid& seedGuid, const plUuid& cloneGuid, plAbstractObjectGraph* pHeader, plAbstractObjectGraph* pObjects, plAbstractObjectGraph* pTypes)
{
  plAssetDocumentManager::InternalCloneDocument(szPath, szClonePath, documentId, seedGuid, cloneGuid, pHeader, pObjects, pTypes);


  auto pRoot = pObjects->GetNodeByName("ObjectTree");
  plUuid settingsGuid = pRoot->FindProperty("Settings")->m_Value.Get<plUuid>();
  auto pSettings = pObjects->GetNode(settingsGuid);
  if (plRTTI::FindTypeByName(pSettings->GetType()) != plGetStaticRTTI<plSceneDocumentSettings>())
    return;

  // Fix up scene layers during cloning
  pObjects->ModifyNodeViaNativeCounterpart(pSettings, [&](void* pNativeObject, const plRTTI* pType) {
    plSceneDocumentSettings* pObject = static_cast<plSceneDocumentSettings*>(pNativeObject);

    for (plSceneLayerBase* pLayerBase : pObject->m_Layers)
    {
      if (auto pLayer = plDynamicCast<plSceneLayer*>(pLayerBase))
      {
        if (pLayer->m_Layer == documentId)
        {
          // Fix up main layer reference in layer list
          pLayer->m_Layer = cloneGuid;
        }
        else
        {
          // Clone layer.
          plStringBuilder sLayerPath;
          {
            auto assetInfo = plAssetCurator::GetSingleton()->GetSubAsset(pLayer->m_Layer);
            if (assetInfo.isValid())
            {
              sLayerPath = assetInfo->m_pAssetInfo->m_sAbsolutePath;
            }
            else
            {
              plLog::Error("Failed to resolve layer: {}. Cloned Layer will be invalid.");
              pLayer->m_Layer.SetInvalid();
            }
          }
          if (!sLayerPath.IsEmpty())
          {
            plUuid newLayerGuid = pLayer->m_Layer;
            newLayerGuid.CombineWithSeed(seedGuid);

            plStringBuilder sLayerClonePath = szClonePath;
            sLayerClonePath.RemoveFileExtension();
            sLayerClonePath.Append("_data");
            plStringBuilder sCloneFleName = plPathUtils::GetFileNameAndExtension(sLayerPath.GetData());
            sLayerClonePath.AppendPath(sCloneFleName);
            // We assume that all layers are handled by the same document manager, i.e. this.
            CloneDocument(sLayerPath, sLayerClonePath, newLayerGuid).LogFailure();
            pLayer->m_Layer = newLayerGuid;
          }
        }
      }
    }
  });
}

void plSceneDocumentManager::SetupDefaultScene(plDocument* pDocument)
{
  auto history = pDocument->GetCommandHistory();
  history->StartTransaction("Initial Scene Setup");

  plUuid skyObjectGuid;
  skyObjectGuid.CreateNewUuid();
  plUuid lightObjectGuid;
  lightObjectGuid.CreateNewUuid();
  plUuid meshObjectGuid;
  meshObjectGuid.CreateNewUuid();

  // Thumbnail Camera
  {
    plUuid objectGuid;
    objectGuid.CreateNewUuid();

    plAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("plGameObject");
    cmd.m_NewObjectGuid = objectGuid;
    cmd.m_sParentProperty = "Children";
    PLASMA_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    // object name
    {
      plSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "Name";
      propCmd.m_NewValue = "Scene Thumbnail Camera";
      PLASMA_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    // camera position
    {
      plSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue = plVec3(0, 0, 0);
      PLASMA_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    // camera component
    {
      plAddObjectCommand cmd;
      cmd.m_Index = -1;
      cmd.SetType("plCameraComponent");
      cmd.m_Parent = objectGuid;
      cmd.m_sParentProperty = "Components";
      PLASMA_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

      // camera shortcut
      {
        plSetObjectPropertyCommand propCmd;
        propCmd.m_Object = cmd.m_NewObjectGuid;
        propCmd.m_sProperty = "EditorShortcut";
        propCmd.m_NewValue = 1;
        PLASMA_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
      }

      // camera usage hint
      {
        plSetObjectPropertyCommand propCmd;
        propCmd.m_Object = cmd.m_NewObjectGuid;
        propCmd.m_sProperty = "UsageHint";
        propCmd.m_NewValue = (int)plCameraUsageHint::Thumbnail;
        PLASMA_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
      }
    }
  }

  {
    plAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("plGameObject");
    cmd.m_NewObjectGuid = meshObjectGuid;
    cmd.m_sParentProperty = "Children";
    PLASMA_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    {
      plSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue = plVec3(3, 0, 0);
      PLASMA_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }
  }

  {
    plAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("plGameObject");
    cmd.m_NewObjectGuid = skyObjectGuid;
    cmd.m_sParentProperty = "Children";
    PLASMA_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    {
      plSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue = plVec3(0, 0, 1);
      PLASMA_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    {
      plRemoveObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "Tags";
      propCmd.m_Index = 0; // There is only one value in the set, CastShadow.
      PLASMA_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    {
      plInsertObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "Tags";
      propCmd.m_Index = 0;
      propCmd.m_NewValue = "SkyLight";
      PLASMA_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }
  }

  {
    plAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("plGameObject");
    cmd.m_NewObjectGuid = lightObjectGuid;
    cmd.m_sParentProperty = "Children";
    PLASMA_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    {
      plSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalPosition";
      propCmd.m_NewValue = plVec3(0, 0, 2);
      PLASMA_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    {
      plQuat qRot;
      qRot.SetFromEulerAngles(plAngle::Degree(0), plAngle::Degree(55), plAngle::Degree(90));

      plSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "LocalRotation";
      propCmd.m_NewValue = qRot;
      PLASMA_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }
  }

  {
    plAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("plSkyBoxComponent");
    cmd.m_Parent = skyObjectGuid;
    cmd.m_sParentProperty = "Components";
    PLASMA_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    {
      plSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "CubeMap";
      propCmd.m_NewValue = "{ 0b202e08-a64f-465d-b38e-15b81d161822 }";
      PLASMA_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    {
      plSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "ExposureBias";
      propCmd.m_NewValue = 1.0f;
      PLASMA_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }
  }

  {
    plAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("plSkyLightComponent");
    cmd.m_Parent = lightObjectGuid;
    cmd.m_sParentProperty = "Components";
    PLASMA_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");
  }

  {
    plAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("plDirectionalLightComponent");
    cmd.m_Parent = lightObjectGuid;
    cmd.m_sParentProperty = "Components";
    PLASMA_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");
  }

  {
    plAddObjectCommand cmd;
    cmd.m_Index = -1;
    cmd.SetType("plMeshComponent");
    cmd.m_Parent = meshObjectGuid;
    cmd.m_sParentProperty = "Components";
    PLASMA_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

    {
      plSetObjectPropertyCommand propCmd;
      propCmd.m_Object = cmd.m_NewObjectGuid;
      propCmd.m_sProperty = "Mesh";
      propCmd.m_NewValue = "{ 618ee743-ed04-4fac-bf5f-572939db2f1d }"; // Base/Meshes/Sphere.plMeshAsset
      PLASMA_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }
  }

  history->FinishTransaction();
}
