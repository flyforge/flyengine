#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/Preferences/ScenePreferences.h>
#include <EditorPluginScene/EditTools/GreyBoxEditTool.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <ToolsFoundation/Command/TreeCommands.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGreyBoxEditTool, 1, plRTTIDefaultAllocator<plGreyBoxEditTool>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plGreyBoxEditTool::plGreyBoxEditTool()
{
  m_DrawBoxGizmo.m_GizmoEvents.AddEventHandler(plMakeDelegate(&plGreyBoxEditTool::GizmoEventHandler, this));
}

plGreyBoxEditTool::~plGreyBoxEditTool()
{
  m_DrawBoxGizmo.m_GizmoEvents.RemoveEventHandler(plMakeDelegate(&plGreyBoxEditTool::GizmoEventHandler, this));
}

PlasmaEditorInputContext* plGreyBoxEditTool::GetEditorInputContextOverride()
{
  if (IsActive())
    return &m_DrawBoxGizmo;

  return nullptr;
}

plEditToolSupportedSpaces plGreyBoxEditTool::GetSupportedSpaces() const
{
  return plEditToolSupportedSpaces::WorldSpaceOnly;
}

bool plGreyBoxEditTool::GetSupportsMoveParentOnly() const
{
  return false;
}


void plGreyBoxEditTool::GetGridSettings(plGridSettingsMsgToEngine& msg)
{
  auto pSceneDoc = GetDocument();
  plScenePreferencesUser* pPreferences = plPreferences::QueryPreferences<plScenePreferencesUser>(GetDocument());

  msg.m_fGridDensity = plSnapProvider::GetTranslationSnapValue(); // negative density = local space
  msg.m_vGridTangent1.SetZero();
  msg.m_vGridTangent2.SetZero();

  if (pPreferences->GetShowGrid())
  {
    if (m_DrawBoxGizmo.GetCurrentMode() == plDrawBoxGizmo::ManipulateMode::DrawBase)
    {
      msg.m_vGridCenter = m_DrawBoxGizmo.GetStartPosition();

      msg.m_vGridTangent1 = plVec3(1, 0, 0);
      msg.m_vGridTangent2 = plVec3(0, 1, 0);
    }
    else if (m_DrawBoxGizmo.GetCurrentMode() == plDrawBoxGizmo::ManipulateMode::DrawHeight)
    {
      const plVec3 vCamDir = GetWindow()->GetFocusedViewWidget()->m_pViewConfig->m_Camera.GetDirForwards();

      msg.m_vGridCenter = m_DrawBoxGizmo.GetStartPosition();

      if (plMath::Abs(plVec3(1, 0, 0).Dot(vCamDir)) < plMath::Abs(plVec3(0, 1, 0).Dot(vCamDir)))
      {
        msg.m_vGridTangent1 = plVec3(1, 0, 0);
        msg.m_vGridTangent2 = plVec3(0, 0, 1);
      }
      else
      {
        msg.m_vGridTangent1 = plVec3(0, 1, 0);
        msg.m_vGridTangent2 = plVec3(0, 0, 1);
      }
    }
    else if (m_DrawBoxGizmo.GetCurrentMode() == plDrawBoxGizmo::ManipulateMode::None)
    {
      if (m_DrawBoxGizmo.GetDisplayGrid())
      {
        msg.m_vGridCenter = m_DrawBoxGizmo.GetStartPosition();

        msg.m_vGridTangent1 = plVec3(1, 0, 0);
        msg.m_vGridTangent2 = plVec3(0, 1, 0);
      }
    }
  }
}

void plGreyBoxEditTool::UpdateGizmoState()
{
  plManipulatorManager::GetSingleton()->HideActiveManipulator(GetDocument(), GetDocument()->GetActiveEditTool() != nullptr);

  m_DrawBoxGizmo.SetVisible(IsActive());
  m_DrawBoxGizmo.SetTransformation(plTransform::IdentityTransform());
}

void plGreyBoxEditTool::GameObjectEventHandler(const plGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case plGameObjectEvent::Type::ActiveEditToolChanged:
      UpdateGizmoState();
      break;

    default:
      break;
  }
}

void plGreyBoxEditTool::ManipulatorManagerEventHandler(const plManipulatorManagerEvent& e)
{
  if (!IsActive())
    return;

  // make sure the gizmo is deactivated when a manipulator becomes active
  if (e.m_pDocument == GetDocument() && e.m_pManipulator != nullptr && e.m_pSelection != nullptr && !e.m_pSelection->IsEmpty() &&
      !e.m_bHideManipulators)
  {
    GetDocument()->SetActiveEditTool(nullptr);
  }
}

void plGreyBoxEditTool::OnConfigured()
{
  GetDocument()->m_GameObjectEvents.AddEventHandler(plMakeDelegate(&plGreyBoxEditTool::GameObjectEventHandler, this));
  plManipulatorManager::GetSingleton()->m_Events.AddEventHandler(plMakeDelegate(&plGreyBoxEditTool::ManipulatorManagerEventHandler, this));

  m_DrawBoxGizmo.SetOwner(GetWindow(), nullptr);
}

void plGreyBoxEditTool::OnActiveChanged(bool bIsActive)
{
  if (bIsActive)
  {
    m_DrawBoxGizmo.UpdateStatusBarText(GetWindow());
  }
}

void plGreyBoxEditTool::GizmoEventHandler(const plGizmoEvent& e)
{
  if (e.m_Type == plGizmoEvent::Type::EndInteractions)
  {
    plVec3 vCenter;
    float negx, posx, negy, posy, negz, posz;
    m_DrawBoxGizmo.GetResult(vCenter, negx, posx, negy, posy, negz, posz);

    auto* pDoc = GetDocument();
    auto* pHistory = pDoc->GetCommandHistory();

    plUuid materialGuid;

    // check if there is a material asset currently selected in the asset browser
    // if so, assign that material to the greybox component
    {
      const plUuid lastSelected = plQtAssetBrowserPanel::GetSingleton()->GetLastSelectedAsset();

      if (lastSelected.IsValid())
      {
        const auto pSubAsset = plAssetCurator::GetSingleton()->GetSubAsset(lastSelected);
        if (pSubAsset && plStringUtils::IsEqual(pSubAsset->m_pAssetInfo->m_Info->GetAssetsDocumentTypeName(), "Material"))
        {
          materialGuid = lastSelected;
        }
      }
    }

    pHistory->StartTransaction("Add Grey-Box");

    plUuid objGuid, compGuid;
    objGuid.CreateNewUuid();
    compGuid.CreateNewUuid();

    {
      plAddObjectCommand cmdAdd;
      cmdAdd.m_NewObjectGuid = objGuid;
      cmdAdd.m_pType = plGetStaticRTTI<plGameObject>();
      cmdAdd.m_sParentProperty = "Children";
      pHistory->AddCommand(cmdAdd).IgnoreResult();
    }
    {
      plSetObjectPropertyCommand cmdPos;
      cmdPos.m_NewValue = vCenter;
      cmdPos.m_Object = objGuid;
      cmdPos.m_sProperty = "LocalPosition";
      pHistory->AddCommand(cmdPos).IgnoreResult();
    }
    {
      plAddObjectCommand cmdComp;
      cmdComp.m_NewObjectGuid = compGuid;
      cmdComp.m_pType = plRTTI::FindTypeByName("plGreyBoxComponent");
      cmdComp.m_sParentProperty = "Components";
      cmdComp.m_Parent = objGuid;
      cmdComp.m_Index = -1;
      pHistory->AddCommand(cmdComp).IgnoreResult();
    }
    if (materialGuid.IsValid())
    {
      plStringBuilder tmp;
      plSetObjectPropertyCommand cmdMat;
      cmdMat.m_NewValue = plConversionUtils::ToString(materialGuid, tmp).GetData();
      cmdMat.m_Object = compGuid;
      cmdMat.m_sProperty = "Material";
      pHistory->AddCommand(cmdMat).IgnoreResult();
    }
    {
      plSetObjectPropertyCommand cmdSize;
      cmdSize.m_Object = compGuid;

      cmdSize.m_NewValue = negx;
      cmdSize.m_sProperty = "SizeNegX";
      pHistory->AddCommand(cmdSize).IgnoreResult();

      cmdSize.m_NewValue = posx;
      cmdSize.m_sProperty = "SizePosX";
      pHistory->AddCommand(cmdSize).IgnoreResult();

      cmdSize.m_NewValue = negy;
      cmdSize.m_sProperty = "SizeNegY";
      pHistory->AddCommand(cmdSize).IgnoreResult();

      cmdSize.m_NewValue = posy;
      cmdSize.m_sProperty = "SizePosY";
      pHistory->AddCommand(cmdSize).IgnoreResult();

      cmdSize.m_NewValue = negz;
      cmdSize.m_sProperty = "SizeNegZ";
      pHistory->AddCommand(cmdSize).IgnoreResult();

      cmdSize.m_NewValue = posz;
      cmdSize.m_sProperty = "SizePosZ";
      pHistory->AddCommand(cmdSize).IgnoreResult();
    }

    pHistory->FinishTransaction();

    pDoc->GetSelectionManager()->SetSelection(pDoc->GetObjectManager()->GetObject(objGuid));
  }
}
