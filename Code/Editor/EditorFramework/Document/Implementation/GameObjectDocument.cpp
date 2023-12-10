#include <EditorFramework/EditorFrameworkPCH.h>

#include "EditorFramework/Panels/LogPanel/LogPanel.moc.h"
#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Panels/LogPanel/LogPanel.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <GuiFoundation/Models/LogModel.moc.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGameObjectMetaData, 1, plRTTINoAllocator)
{
  //PLASMA_BEGIN_PROPERTIES
  //{
  //  //PLASMA_MEMBER_PROPERTY("MetaHidden", m_bHidden) // remove this property to disable serialization
  //}
  //PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGameObjectDocument, 2, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plEvent<const plGameObjectDocumentEvent&> plGameObjectDocument::s_GameObjectDocumentEvents;

plGameObjectDocument::plGameObjectDocument(
  plStringView sDocumentPath, plDocumentObjectManager* pObjectManager, plAssetDocEngineConnection engineConnectionType)
  : plAssetDocument(sDocumentPath, pObjectManager, engineConnectionType)
{
  using Meta = plObjectMetaData<plUuid, plGameObjectMetaData>;
  m_GameObjectMetaData = PLASMA_DEFAULT_NEW(Meta);

  PLASMA_ASSERT_DEV(engineConnectionType == plAssetDocEngineConnection::FullObjectMirroring,
    "plGameObjectDocument only supports full mirroring engine connection types. The parameter only exists for interface compatibility.");

  m_CurrentMode.m_bRenderSelectionOverlay = true;
  m_CurrentMode.m_bRenderShapeIcons = true;
  m_CurrentMode.m_bRenderVisualizers = true;
}

plGameObjectDocument::~plGameObjectDocument()
{
  UnsubscribeGameObjectEventHandlers();
  DeallocateEditTools();
}

void plGameObjectDocument::SubscribeGameObjectEventHandlers()
{
  m_SelectionManagerEventHandlerID = GetSelectionManager()->m_Events.AddEventHandler(plMakeDelegate(&plGameObjectDocument::SelectionManagerEventHandler, this));
  m_ObjectPropertyEventHandlerID = GetObjectManager()->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plGameObjectDocument::ObjectPropertyEventHandler, this));
  m_ObjectStructureEventHandlerID = GetObjectManager()->m_StructureEvents.AddEventHandler(plMakeDelegate(&plGameObjectDocument::ObjectStructureEventHandler, this));
  m_ObjectEventHandlerID = GetObjectManager()->m_ObjectEvents.AddEventHandler(plMakeDelegate(&plGameObjectDocument::ObjectEventHandler, this));

  s_GameObjectDocumentEvents.AddEventHandler(plMakeDelegate(&plGameObjectDocument::GameObjectDocumentEventHandler, this));
}

void plGameObjectDocument::UnsubscribeGameObjectEventHandlers()
{
  GetSelectionManager()->m_Events.RemoveEventHandler(m_SelectionManagerEventHandlerID);
  GetObjectManager()->m_PropertyEvents.RemoveEventHandler(m_ObjectPropertyEventHandlerID);
  GetObjectManager()->m_StructureEvents.RemoveEventHandler(m_ObjectStructureEventHandlerID);
  GetObjectManager()->m_ObjectEvents.RemoveEventHandler(m_ObjectEventHandlerID);

  s_GameObjectDocumentEvents.RemoveEventHandler(plMakeDelegate(&plGameObjectDocument::GameObjectDocumentEventHandler, this));
}

void plGameObjectDocument::GameObjectDocumentEventHandler(const plGameObjectDocumentEvent& e)
{
  switch (e.m_Type)
  {
    // case plGameObjectDocumentEvent::Type::GameMode_StartingExternal: // the external player doesn't log to the editor panel, so don't need to clear that
    case plGameObjectDocumentEvent::Type::GameMode_StartingPlay:
    case plGameObjectDocumentEvent::Type::GameMode_StartingSimulate:
    {
      auto pEditorPrefsUser = plPreferences::QueryPreferences<plEditorPreferencesUser>();
      if (pEditorPrefsUser && pEditorPrefsUser->m_bClearEditorLogsOnPlay)
      {
        // on play, the engine log has a lot of activity, so makes sense to clear that first
        plQtLogPanel::GetSingleton()->EngineLog->GetLog()->Clear();
        // but I think we usually want to keep the editor log around
        // plQtLogPanel::GetSingleton()->EditorLog->GetLog()->Clear();
      }
    }
    break;
    default:
      break;
  }
}

plEditorInputContext* plGameObjectDocument::GetEditorInputContextOverride()
{
  if (GetActiveEditTool() && GetActiveEditTool()->GetEditorInputContextOverride() != nullptr)
  {
    return GetActiveEditTool()->GetEditorInputContextOverride();
  }

  return nullptr;
}

void plGameObjectDocument::SetEditToolConfigDelegate(plDelegate<void(plGameObjectEditTool*)> configDelegate)
{
  m_EditToolConfigDelegate = configDelegate;
}

bool plGameObjectDocument::IsActiveEditTool(const plRTTI* pEditToolType) const
{
  if (m_pActiveEditTool == nullptr)
    return pEditToolType == nullptr;

  if (pEditToolType == nullptr)
    return false;

  return m_pActiveEditTool->IsInstanceOf(pEditToolType);
}

void plGameObjectDocument::SetActiveEditTool(const plRTTI* pEditToolType)
{
  plGameObjectEditTool* pEditTool = nullptr;

  if (pEditToolType != nullptr)
  {
    auto it = m_CreatedEditTools.Find(pEditToolType);
    if (it.IsValid())
    {
      pEditTool = it.Value();
    }
    else
    {
      PLASMA_ASSERT_DEBUG(m_EditToolConfigDelegate.IsValid(), "Window did not specify a delegate to configure edit tools");

      pEditTool = pEditToolType->GetAllocator()->Allocate<plGameObjectEditTool>();
      m_CreatedEditTools[pEditToolType] = pEditTool;

      m_EditToolConfigDelegate(pEditTool);
    }
  }

  if (m_pActiveEditTool == pEditTool)
  {
    if (m_pActiveEditTool == nullptr)
    {
      // if there is currently no active edit tool, we may still have a manipulator active
      // if so, when repeatedly selecting this action, toggle the visibility of manipulators
      plManipulatorManager::GetSingleton()->ToggleHideActiveManipulator(this);
    }

    return;
  }

  if (m_pActiveEditTool)
    m_pActiveEditTool->SetActive(false);

  m_pActiveEditTool = pEditTool;

  if (m_pActiveEditTool)
    m_pActiveEditTool->SetActive(true);

  plGameObjectEvent e;
  e.m_Type = plGameObjectEvent::Type::ActiveEditToolChanged;
  m_GameObjectEvents.Broadcast(e);
}

void plGameObjectDocument::SetAddAmbientLight(bool b)
{
  if (m_bAddAmbientLight == b)
    return;

  m_bAddAmbientLight = b;

  plGameObjectEvent e;
  e.m_Type = plGameObjectEvent::Type::AddAmbientLightChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(plFmt("Ambient Light: {}", m_bAddAmbientLight ? "ON" : "OFF"));
}

void plGameObjectDocument::SetPickTransparent(bool b)
{
  if (m_bPickTransparent == b)
    return;

  m_bPickTransparent = b;

  plGameObjectEvent e;
  e.m_Type = plGameObjectEvent::Type::PickTransparentChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(plFmt("Select Transparent: {}", m_bPickTransparent ? "ON" : "OFF"));

  if (m_bPickTransparent == false)
  {
    // make sure no transparent object is currently selected
    GetSelectionManager()->Clear();
  }
}

void plGameObjectDocument::SetGizmoWorldSpace(bool bWorldSpace)
{
  if (m_bGizmoWorldSpace == bWorldSpace)
    return;

  m_bGizmoWorldSpace = bWorldSpace;

  plGameObjectEvent e;
  e.m_Type = plGameObjectEvent::Type::ActiveEditToolChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(plFmt("Transform in {}", m_bGizmoWorldSpace ? "World Space" : "Object Space"));
}

bool plGameObjectDocument::GetGizmoWorldSpace() const
{
  return m_bGizmoWorldSpace;
}

void plGameObjectDocument::SetGizmoMoveParentOnly(bool bMoveParent)
{
  if (m_bGizmoMoveParentOnly == bMoveParent)
    return;

  m_bGizmoMoveParentOnly = bMoveParent;

  plGameObjectEvent e;
  e.m_Type = plGameObjectEvent::Type::ActiveEditToolChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(plFmt("Move Parent Only: {}", m_bGizmoMoveParentOnly ? "ON" : "OFF"));
}

void plGameObjectDocument::DetermineNodeName(const plDocumentObject* pObject, const plUuid& prefabGuid, plStringBuilder& out_sResult, QIcon* out_pIcon /*= nullptr*/) const
{
  // tries to find a good name for a node by looking at the attached components and their properties

  bool bHasIcon = false;

  if (prefabGuid.IsValid())
  {
    auto pInfo = plAssetCurator::GetSingleton()->GetSubAsset(prefabGuid);

    if (pInfo)
    {
      plStringBuilder sPath = pInfo->m_pAssetInfo->m_Path.GetDataDirParentRelativePath();
      sPath = sPath.GetFileName();

      out_sResult.Set("Prefab: ", sPath);
    }
    else
      out_sResult = "Prefab: Invalid Asset";
  }

  const bool bHasChildren = pObject->GetTypeAccessor().GetCount("Children") > 0;

  plStringBuilder tmp;

  const plInt32 iComponents = pObject->GetTypeAccessor().GetCount("Components");
  for (plInt32 i = 0; i < iComponents; i++)
  {
    plVariant value = pObject->GetTypeAccessor().GetValue("Components", i);
    auto pChild = GetObjectManager()->GetObject(value.Get<plUuid>());
    PLASMA_ASSERT_DEBUG(pChild->GetTypeAccessor().GetType()->IsDerivedFrom<plComponent>(), "Non-component found in component set.");
    // take the first components name
    if (!bHasIcon && out_pIcon != nullptr)
    {
      bHasIcon = true;

      plColor color = plColor::MakeZero();

      if (auto pCatAttr = pChild->GetTypeAccessor().GetType()->GetAttributeByType<plCategoryAttribute>())
      {
        color = plColorScheme::GetCategoryColor(pCatAttr->GetCategory(), plColorScheme::CategoryColorUsage::SceneTreeIcon);
      }

      plStringBuilder sIconName;
      sIconName.Set(":/TypeIcons/", pChild->GetTypeAccessor().GetType()->GetTypeName(), ".svg");
      *out_pIcon = plQtUiServices::GetCachedIconResource(sIconName.GetData(), color);
    }

    if (out_sResult.IsEmpty())
    {
      // try to translate the component name, that will typically make it a nice clean name already
      out_sResult = plTranslate(pChild->GetTypeAccessor().GetType()->GetTypeName().GetData(tmp));

      // if no translation is available, clean up the component name in a simple way
      if (out_sResult.EndsWith_NoCase("Component"))
        out_sResult.Shrink(0, 9);
      if (out_sResult.StartsWith("pl"))
        out_sResult.Shrink(2, 0);

      if (auto pInDev = pChild->GetTypeAccessor().GetType()->GetAttributeByType<plInDevelopmentAttribute>())
      {
        out_sResult.AppendFormat(" [ {} ]", pInDev->GetString());
      }
    }

    if (prefabGuid.IsValid())
      continue;

    const auto& properties = pChild->GetTypeAccessor().GetType()->GetProperties();

    for (auto pProperty : properties)
    {
      // search for string properties that also have an asset browser property -> they reference an asset, so this is most likely the most
      // relevant property
      if (
        (pProperty->GetSpecificType() == plGetStaticRTTI<const char*>() || pProperty->GetSpecificType() == plGetStaticRTTI<plString>()) && pProperty->GetAttributeByType<plAssetBrowserAttribute>() != nullptr)
      {
        plStringBuilder sValue;
        if (pProperty->GetCategory() == plPropertyCategory::Member)
        {
          sValue = pChild->GetTypeAccessor().GetValue(pProperty->GetPropertyName()).ConvertTo<plString>();
        }
        else if (pProperty->GetCategory() == plPropertyCategory::Array)
        {
          const plInt32 iCount = pChild->GetTypeAccessor().GetCount(pProperty->GetPropertyName());
          if (iCount > 0)
          {
            sValue = pChild->GetTypeAccessor().GetValue(pProperty->GetPropertyName(), 0).ConvertTo<plString>();
          }
        }

        // if the property is a full asset guid reference, convert it to a file name
        if (plConversionUtils::IsStringUuid(sValue))
        {
          const plUuid AssetGuid = plConversionUtils::ConvertStringToUuid(sValue);

          auto pAsset = plAssetCurator::GetSingleton()->GetSubAsset(AssetGuid);

          if (pAsset)
            sValue = pAsset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath();
          else
            sValue = "<unknown>";
        }

        // only use the file name for our display
        sValue = sValue.GetFileName();

        if (!sValue.IsEmpty())
          out_sResult.Append(": ", sValue);

        return;
      }
    }
  }

  if (!out_sResult.IsEmpty())
    return;

  if (bHasChildren)
    out_sResult = "Group";
  else
    out_sResult = "Object";
}


void plGameObjectDocument::QueryCachedNodeName(
  const plDocumentObject* pObject, plStringBuilder& out_sResult, plUuid* out_pPrefabGuid, QIcon* out_pIcon /*= nullptr*/) const
{
  auto pMetaScene = m_GameObjectMetaData->BeginReadMetaData(pObject->GetGuid());
  auto pMetaDoc = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid());
  const plUuid prefabGuid = pMetaDoc->m_CreateFromPrefab;

  if (out_pPrefabGuid != nullptr)
    *out_pPrefabGuid = prefabGuid;

  out_sResult = pMetaScene->m_CachedNodeName;
  if (out_pIcon)
    *out_pIcon = pMetaScene->m_Icon;
  m_GameObjectMetaData->EndReadMetaData();
  m_DocumentObjectMetaData->EndReadMetaData();

  if (out_sResult.IsEmpty())
  {
    // the cached node name is only determined once
    // after that only a node rename (EditRole) will currently trigger a cache cleaning and thus a reevaluation
    // this is to prevent excessive re-computation of the name, which is quite involved

    QIcon icon;
    DetermineNodeName(pObject, prefabGuid, out_sResult, &icon);
    plString sNodeName = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<plString>();
    if (!sNodeName.IsEmpty())
    {
      out_sResult = sNodeName;
    }
    auto pMetaWrite = m_GameObjectMetaData->BeginModifyMetaData(pObject->GetGuid());
    pMetaWrite->m_CachedNodeName = out_sResult;
    pMetaWrite->m_Icon = icon;
    m_GameObjectMetaData->EndModifyMetaData(0); // no need to broadcast this change

    if (out_pIcon != nullptr)
      *out_pIcon = icon;
  }
}


void plGameObjectDocument::GenerateFullDisplayName(const plDocumentObject* pRoot, plStringBuilder& out_sFullPath) const
{
  if (pRoot == nullptr || pRoot == GetObjectManager()->GetRootObject())
    return;

  GenerateFullDisplayName(pRoot->GetParent(), out_sFullPath);

  if (!pRoot->GetType()->IsDerivedFrom<plComponent>())
  {
    plStringBuilder sObjectName;
    QueryCachedNodeName(pRoot, sObjectName);

    out_sFullPath.AppendPath(sObjectName);
  }
}

plTransform plGameObjectDocument::GetGlobalTransform(const plDocumentObject* pObject) const
{
  if (!m_GlobalTransforms.Contains(pObject))
  {
    ComputeGlobalTransform(pObject);
  }

  return plSimdConversion::ToTransform(m_GlobalTransforms[pObject]);
}

void plGameObjectDocument::SetGlobalTransform(const plDocumentObject* pObject, const plTransform& t, plUInt8 uiTransformationChanges) const
{
  plObjectAccessorBase* pAccessor = GetObjectAccessor();
  auto pHistory = GetCommandHistory();
  if (!pHistory->IsInTransaction())
  {
    InvalidateGlobalTransformValue(pObject);
    return;
  }

  const plDocumentObject* pParent = pObject->GetParent();

  plSimdTransform tLocal;
  plSimdTransform simdT = plSimdConversion::ToTransform(t);

  if (pParent != nullptr)
  {
    if (!m_GlobalTransforms.Contains(pParent))
    {
      ComputeGlobalTransform(pParent);
    }

    plSimdTransform tParent = m_GlobalTransforms[pParent];

    tLocal = plSimdTransform::MakeLocalTransform(tParent, simdT);
  }
  else
  {
    tLocal = simdT;
  }

  plVec3 vLocalPos = plSimdConversion::ToVec3(tLocal.m_Position);
  plVec3 vLocalScale = plSimdConversion::ToVec3(tLocal.m_Scale);
  plQuat qLocalRot = plSimdConversion::ToQuat(tLocal.m_Rotation);
  float fUniformScale = 1.0f;

  if (vLocalScale.x == vLocalScale.y && vLocalScale.x == vLocalScale.z)
  {
    fUniformScale = vLocalScale.x;
    vLocalScale.Set(1.0f);
  }

  // unfortunately when we are dragging an object the 'temporary' transaction is undone every time before the new commands are sent
  // that means the values that we read here, are always the original values before the object was modified at all
  // therefore when the original position and the new position are identical, that means the user dragged the object to the previous
  // position it does NOT mean that there is no change, in fact there is a change, just back to the original value

  // if (pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<plVec3>() != vLocalPos)
  if ((uiTransformationChanges & TransformationChanges::Translation) != 0)
  {
    pAccessor->SetValue(pObject, "LocalPosition", vLocalPos).LogFailure();
  }

  // if (pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<plQuat>() != qLocalRot)
  if ((uiTransformationChanges & TransformationChanges::Rotation) != 0)
  {
    pAccessor->SetValue(pObject, "LocalRotation", qLocalRot).LogFailure();
  }

  // if (pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<plVec3>() != vLocalScale)
  if ((uiTransformationChanges & TransformationChanges::Scale) != 0)
  {
    pAccessor->SetValue(pObject, "LocalScaling", vLocalScale).LogFailure();
  }

  // if (pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>() != fUniformScale)
  if ((uiTransformationChanges & TransformationChanges::UniformScale) != 0)
  {
    pAccessor->SetValue(pObject, "LocalUniformScaling", fUniformScale).LogFailure();
  }

  // will be recomputed the next time it is queried
  InvalidateGlobalTransformValue(pObject);
}

void plGameObjectDocument::SetGlobalTransformParentOnly(const plDocumentObject* pObject, const plTransform& t, plUInt8 uiTransformationChanges) const
{
  plHybridArray<plTransform, 16> childTransforms;
  const auto& children = pObject->GetChildren();

  childTransforms.SetCountUninitialized(children.GetCount());

  for (plUInt32 i = 0; i < children.GetCount(); ++i)
  {
    const plDocumentObject* pChild = children[i];
    childTransforms[i] = GetGlobalTransform(pChild);
  }

  SetGlobalTransform(pObject, t, uiTransformationChanges);

  for (plUInt32 i = 0; i < children.GetCount(); ++i)
  {
    const plDocumentObject* pChild = children[i];
    SetGlobalTransform(pChild, childTransforms[i], TransformationChanges::All);
  }
}

void plGameObjectDocument::InvalidateGlobalTransformValue(const plDocumentObject* pObject) const
{
  // will be recomputed the next time it is queried
  m_GlobalTransforms.Remove(pObject);

  /// \todo If all parents are always inserted as well, we can stop once an object is found that is not in the list

  for (auto pChild : pObject->GetChildren())
  {
    InvalidateGlobalTransformValue(pChild);
  }
}

plResult plGameObjectDocument::ComputeObjectTransformation(const plDocumentObject* pObject, plTransform& out_result) const
{
  const plDocumentObject* pObj = pObject;

  while (pObj && !pObj->GetTypeAccessor().GetType()->IsDerivedFrom<plGameObject>())
  {
    pObj = pObj->GetParent();
  }

  if (pObj)
  {
    out_result = ComputeGlobalTransform(pObj);
    return PLASMA_SUCCESS;
  }
  else
  {
    out_result.SetIdentity();
    return PLASMA_FAILURE;
  }
}

bool plGameObjectDocument::GetGizmoMoveParentOnly() const
{
  return m_bGizmoMoveParentOnly;
}

void plGameObjectDocument::DeallocateEditTools()
{
  for (auto it = m_CreatedEditTools.GetIterator(); it.IsValid(); ++it)
  {
    it.Value()->GetDynamicRTTI()->GetAllocator()->Deallocate(it.Value());
  }

  m_CreatedEditTools.Clear();
}

void plGameObjectDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);
  SubscribeGameObjectEventHandlers();
}


void plGameObjectDocument::AttachMetaDataBeforeSaving(plAbstractObjectGraph& graph) const
{
  plAssetDocument::AttachMetaDataBeforeSaving(graph);

  m_GameObjectMetaData->AttachMetaDataToAbstractGraph(graph);
}

void plGameObjectDocument::RestoreMetaDataAfterLoading(const plAbstractObjectGraph& graph, bool bUndoable)
{
  plAssetDocument::RestoreMetaDataAfterLoading(graph, bUndoable);

  m_GameObjectMetaData->RestoreMetaDataFromAbstractGraph(graph);
}

void plGameObjectDocument::TriggerShowSelectionInScenegraph() const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  plGameObjectEvent e;
  e.m_Type = plGameObjectEvent::Type::TriggerShowSelectionInScenegraph;
  m_GameObjectEvents.Broadcast(e);
}

void plGameObjectDocument::TriggerFocusOnSelection(bool bAllViews) const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  plGameObjectEvent e;
  e.m_Type = bAllViews ? plGameObjectEvent::Type::TriggerFocusOnSelection_All : plGameObjectEvent::Type::TriggerFocusOnSelection_Hovered;
  m_GameObjectEvents.Broadcast(e);
}

void plGameObjectDocument::TriggerSnapPivotToGrid() const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  plGameObjectEvent e;
  e.m_Type = plGameObjectEvent::Type::TriggerSnapSelectionPivotToGrid;
  m_GameObjectEvents.Broadcast(e);
}

void plGameObjectDocument::TriggerSnapEachObjectToGrid() const
{
  if (GetSelectionManager()->GetSelection().IsEmpty())
    return;

  plGameObjectEvent e;
  e.m_Type = plGameObjectEvent::Type::TriggerSnapEachSelectedObjectToGrid;
  m_GameObjectEvents.Broadcast(e);
}

void plGameObjectDocument::SnapCameraToObject()
{
  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.GetCount() != 1)
    return;

  plTransform trans;
  if (ComputeObjectTransformation(selection[0], trans).Failed())
    return;

  const auto& ctxt = plQtEngineViewWidget::GetInteractionContext();

  if (ctxt.m_pLastHoveredViewWidget == nullptr)
    return;

  if (ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Perspective != plSceneViewPerspective::Perspective)
  {
    ShowDocumentStatus("Note: This operation can only be performed in perspective views.");
    return;
  }

  const plCamera* pCamera = &ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

  const plVec3 vForward = trans.m_qRotation * plVec3(1, 0, 0);
  const plVec3 vUp = trans.m_qRotation * plVec3(0, 0, 1);

  ctxt.m_pLastHoveredViewWidget->InterpolateCameraTo(trans.m_vPosition, vForward, pCamera->GetFovOrDim(), &vUp);
}


void plGameObjectDocument::MoveCameraHere()
{
  const auto& ctxt = plQtEngineViewWidget::GetInteractionContext();

  if (ctxt.m_pLastHoveredViewWidget == nullptr || ctxt.m_pLastPickingResult == nullptr)
    return;

  if (ctxt.m_pLastPickingResult->m_vPickedPosition.IsNaN())
    return;

  const plCamera* pCamera = &ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

  const plVec3 vCurPos = pCamera->GetCenterPosition();
  const plVec3 vDirToPos = ctxt.m_pLastPickingResult->m_vPickedPosition - vCurPos;

  // don't move the entire distance, keep some distance to the target position
  plVec3 vPos = vCurPos + 0.9f * vDirToPos;
  plVec3 vCamDir = pCamera->GetCenterDirForwards();
  plVec3 vCamUp = pCamera->GetCenterDirUp();

  // if the projection mode of the view is orthographic, ignore the direction
  if (ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Perspective != plSceneViewPerspective::Perspective)
  {
    const auto& oldCam = ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Camera;

    vCamDir = oldCam.GetCenterDirForwards();
    vCamUp = oldCam.GetCenterDirUp();

    switch (ctxt.m_pLastHoveredViewWidget->m_pViewConfig->m_Perspective)
    {
      case plSceneViewPerspective::Orthogonal_Front:
        vPos.x = oldCam.GetCenterPosition().x;
        break;
      case plSceneViewPerspective::Orthogonal_Right:
        vPos.y = oldCam.GetCenterPosition().y;
        break;
      case plSceneViewPerspective::Orthogonal_Top:
        vPos.z = oldCam.GetCenterPosition().z;
        break;

      default:
        break;
    }
  }
  else
  {
    // in ortho modes it is fine to move just anywhere, and we often don't pick a real object,
    // because of the wireframe picking

    // however, in perspective modes, don't move, if we haven't picked any real object
    // this happens for example when one picks the sky -> you would end up far away
    if (!ctxt.m_pLastPickingResult->m_PickedComponent.IsValid() && !ctxt.m_pLastPickingResult->m_PickedOther.IsValid())
      return;
  }

  ctxt.m_pLastHoveredViewWidget->InterpolateCameraTo(vPos, vCamDir, pCamera->GetFovOrDim(), &vCamUp);
}

plStatus plGameObjectDocument::CreateGameObjectHere()
{
  const auto& ctxt = plQtEngineViewWidget::GetInteractionContext();
  const bool bCanCreate =
    ctxt.m_pLastHoveredViewWidget != nullptr && ctxt.m_pLastPickingResult && !ctxt.m_pLastPickingResult->m_vPickedPosition.IsNaN();

  if (!bCanCreate)
    return plStatus(PLASMA_FAILURE);

  auto history = GetCommandHistory();

  history->StartTransaction("Create Node");

  plAddObjectCommand cmdAdd;
  cmdAdd.m_pType = plGetStaticRTTI<plGameObject>();
  cmdAdd.m_sParentProperty = "Children";
  cmdAdd.m_Index = -1;

  plUuid NewNode;

  const auto& Sel = GetSelectionManager()->GetSelection();

  if (true)
  {
    cmdAdd.m_NewObjectGuid = plUuid::MakeUuid();
    NewNode = cmdAdd.m_NewObjectGuid;

    auto res = history->AddCommand(cmdAdd);
    if (res.Failed())
    {
      history->CancelTransaction();
      return res;
    }
  }

  plVec3 vCreatePos = ctxt.m_pLastPickingResult->m_vPickedPosition;
  plSnapProvider::SnapTranslation(vCreatePos);

  plSetObjectPropertyCommand cmdSet;
  cmdSet.m_NewValue = vCreatePos;
  cmdSet.m_Object = NewNode;
  cmdSet.m_sProperty = "LocalPosition";

  auto res = history->AddCommand(cmdSet);
  if (res.Failed())
  {
    history->CancelTransaction();
    return res;
  }

  // Add a dummy shape icon component, which enables picking
  {
    plAddObjectCommand cmdAdd2;
    cmdAdd2.m_pType = plRTTI::FindTypeByName("plShapeIconComponent");
    cmdAdd2.m_sParentProperty = "Components";
    cmdAdd2.m_Index = -1;
    cmdAdd2.m_Parent = NewNode;

    auto result = history->AddCommand(cmdAdd2);
  }

  history->FinishTransaction();

  GetSelectionManager()->SetSelection(GetObjectManager()->GetObject(NewNode));

  return plStatus(PLASMA_SUCCESS);
}

void plGameObjectDocument::ScheduleSendObjectSelection()
{
  m_iResendSelection = 2;
}

void plGameObjectDocument::SendGameWorldToEngine()
{
  SendDocumentOpenMessage(true);
}

void plGameObjectDocument::SetSimulationSpeed(float f)
{
  if (m_fSimulationSpeed == f)
    return;

  m_fSimulationSpeed = f;

  plGameObjectEvent e;
  e.m_Type = plGameObjectEvent::Type::SimulationSpeedChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(plFmt("Simulation Speed: {0}%%", (plInt32)(m_fSimulationSpeed * 100.0f)));
}

void plGameObjectDocument::SetRenderSelectionOverlay(bool b)
{
  if (m_CurrentMode.m_bRenderSelectionOverlay == b)
    return;

  m_CurrentMode.m_bRenderSelectionOverlay = b;

  plGameObjectEvent e;
  e.m_Type = plGameObjectEvent::Type::RenderSelectionOverlayChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(plFmt("Selection Overlay: {}", m_CurrentMode.m_bRenderSelectionOverlay ? "ON" : "OFF"));
}


void plGameObjectDocument::SetRenderVisualizers(bool b)
{
  if (m_CurrentMode.m_bRenderVisualizers == b)
    return;

  m_CurrentMode.m_bRenderVisualizers = b;

  plVisualizerManager::GetSingleton()->SetVisualizersActive(GetActiveSubDocument(), m_CurrentMode.m_bRenderVisualizers);

  plGameObjectEvent e;
  e.m_Type = plGameObjectEvent::Type::RenderVisualizersChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(plFmt("Visualizers: {}", m_CurrentMode.m_bRenderVisualizers ? "ON" : "OFF"));
}

void plGameObjectDocument::SetRenderShapeIcons(bool b)
{
  if (m_CurrentMode.m_bRenderShapeIcons == b)
    return;

  m_CurrentMode.m_bRenderShapeIcons = b;

  plGameObjectEvent e;
  e.m_Type = plGameObjectEvent::Type::RenderShapeIconsChanged;
  m_GameObjectEvents.Broadcast(e);

  ShowDocumentStatus(plFmt("Shape Icons: {}", m_CurrentMode.m_bRenderShapeIcons ? "ON" : "OFF"));
}

void plGameObjectDocument::ObjectPropertyEventHandler(const plDocumentObjectPropertyEvent& e)
{
  if (e.m_sProperty == "LocalPosition" || e.m_sProperty == "LocalRotation" || e.m_sProperty == "LocalScaling" ||
      e.m_sProperty == "LocalUniformScaling")
  {
    InvalidateGlobalTransformValue(e.m_pObject);
  }

  if (e.m_sProperty == "Name")
  {
    auto pMetaWrite = m_GameObjectMetaData->BeginModifyMetaData(e.m_pObject->GetGuid());
    pMetaWrite->m_CachedNodeName.Clear();
    m_GameObjectMetaData->EndModifyMetaData(plGameObjectMetaData::CachedName);
  }
}

void plGameObjectDocument::ObjectStructureEventHandler(const plDocumentObjectStructureEvent& e)
{
  if (e.m_pObject && e.m_pObject->GetTypeAccessor().GetType()->IsDerivedFrom<plGameObject>())
  {
    switch (e.m_EventType)
    {
      case plDocumentObjectStructureEvent::Type::BeforeObjectMoved:
      {
        // make sure the cache is filled with a proper value
        GetGlobalTransform(e.m_pObject);
      }
      break;

      case plDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      {
        // read cached value, hopefully it was not invalidated in between BeforeObjectMoved and AfterObjectMoved
        plTransform t = GetGlobalTransform(e.m_pObject);

        SetGlobalTransform(e.m_pObject, t, TransformationChanges::All);
      }
      break;

      default:
        break;
    }
  }
  else
  {
    switch (e.m_EventType)
    {
      case plDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      case plDocumentObjectStructureEvent::Type::AfterObjectAdded:
      case plDocumentObjectStructureEvent::Type::AfterObjectRemoved:
        if (e.m_sParentProperty == "Components")
        {
          if (e.m_pPreviousParent != nullptr)
          {
            auto pMeta = m_GameObjectMetaData->BeginModifyMetaData(e.m_pPreviousParent->GetGuid());
            pMeta->m_CachedNodeName.Clear();
            m_GameObjectMetaData->EndModifyMetaData(plGameObjectMetaData::CachedName);
          }

          if (e.m_pNewParent != nullptr)
          {
            auto pMeta = m_GameObjectMetaData->BeginModifyMetaData(e.m_pNewParent->GetGuid());
            pMeta->m_CachedNodeName.Clear();
            m_GameObjectMetaData->EndModifyMetaData(plGameObjectMetaData::CachedName);
          }
        }
        break;

      default:
        break;
    }
  }
}


void plGameObjectDocument::ObjectEventHandler(const plDocumentObjectEvent& e)
{
  if (!e.m_pObject->GetTypeAccessor().GetType()->IsDerivedFrom<plGameObject>())
    return;

  switch (e.m_EventType)
  {
    case plDocumentObjectEvent::Type::BeforeObjectDestroyed:
    {
      // clean up object meta data upon object destruction, because we can :-P
      if (GetObjectManager()->GetObject(e.m_pObject->GetGuid()) == nullptr)
      {
        // make sure there is no object with this GUID still "added" to the document
        // this can happen if two objects use the same GUID, only one object can be "added" at a time, but multiple objects with the same
        // GUID may exist the same GUID is in use, when a prefab is recreated (updated) and the GUIDs are restored, such that references
        // don't change the object that is being destroyed is typically referenced by a command that was in the redo-queue that got purged

        m_DocumentObjectMetaData->ClearMetaData(e.m_pObject->GetGuid());
        m_GameObjectMetaData->ClearMetaData(e.m_pObject->GetGuid());
      }
    }
    break;

    default:
      break;
  }
}


void plGameObjectDocument::SelectionManagerEventHandler(const plSelectionManagerEvent& e)
{
  ScheduleSendObjectSelection();

  plEditorPreferencesUser* pPreferences = plPreferences::QueryPreferences<plEditorPreferencesUser>();

  if (pPreferences->m_bExpandSceneTreeOnSelection)
  {
    TriggerShowSelectionInScenegraph();
  }
}

void plGameObjectDocument::SendObjectSelection()
{
  if (m_iResendSelection <= 0)
    return;

  --m_iResendSelection;

  const auto& sel = GetSelectionManager()->GetSelection();

  plObjectSelectionMsgToEngine msg;
  plStringBuilder sTemp;
  plStringBuilder sGuid;

  for (const auto& item : sel)
  {
    plConversionUtils::ToString(item->GetGuid(), sGuid);

    sTemp.Append(";", sGuid);
  }

  msg.m_sSelection = sTemp;

  GetEditorEngineConnection()->SendMessage(&msg);
}
// static
plTransform plGameObjectDocument::QueryLocalTransform(const plDocumentObject* pObject)
{
  const plVec3 vTranslation = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<plVec3>();
  const plVec3 vScaling = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<plVec3>();
  const plQuat qRotation = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<plQuat>();
  const float fScaling = pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

  return plTransform(vTranslation, qRotation, vScaling * fScaling);
}

// static
plSimdTransform plGameObjectDocument::QueryLocalTransformSimd(const plDocumentObject* pObject)
{
  const plVec3 vTranslation = pObject->GetTypeAccessor().GetValue("LocalPosition").ConvertTo<plVec3>();
  const plVec3 vScaling = pObject->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<plVec3>();
  const plQuat qRotation = pObject->GetTypeAccessor().GetValue("LocalRotation").ConvertTo<plQuat>();
  const float fScaling = pObject->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();

  return plSimdTransform(plSimdConversion::ToVec3(vTranslation), plSimdConversion::ToQuat(qRotation), plSimdConversion::ToVec3(vScaling * fScaling));
}


plTransform plGameObjectDocument::ComputeGlobalTransform(const plDocumentObject* pObject) const
{
  if (pObject == nullptr || pObject->GetTypeAccessor().GetType() != plGetStaticRTTI<plGameObject>())
  {
    m_GlobalTransforms[pObject] = plSimdTransform::MakeIdentity();
    return plTransform::MakeIdentity();
  }

  const plSimdTransform tParent = plSimdConversion::ToTransform(ComputeGlobalTransform(pObject->GetParent()));
  const plSimdTransform tLocal = QueryLocalTransformSimd(pObject);

  plSimdTransform tGlobal = plSimdTransform::MakeGlobalTransform(tParent, tLocal);

  m_GlobalTransforms[pObject] = tGlobal;

  return plSimdConversion::ToTransform(tGlobal);
}

void plGameObjectDocument::ComputeTopLevelSelectedGameObjects(plDeque<plSelectedGameObject>& out_selection)
{
  // Get the list of all objects that are manipulated
  // and store their original transformation

  out_selection.Clear();

  auto hType = plGetStaticRTTI<plGameObject>();

  auto pSelMan = GetSelectionManager();
  const auto& Selection = pSelMan->GetSelection();
  for (plUInt32 sel = 0; sel < Selection.GetCount(); ++sel)
  {
    if (!Selection[sel]->GetTypeAccessor().GetType()->IsDerivedFrom(hType))
      continue;

    // ignore objects, whose parent is already selected as well, so that transformations aren't applied
    // multiple times on the same hierarchy
    if (pSelMan->IsParentSelected(Selection[sel]))
      continue;

    plSelectedGameObject& sgo = out_selection.ExpandAndGetRef();
    sgo.m_pObject = Selection[sel];
    sgo.m_GlobalTransform = GetGlobalTransform(sgo.m_pObject);
    sgo.m_vLocalScaling = Selection[sel]->GetTypeAccessor().GetValue("LocalScaling").ConvertTo<plVec3>();
    sgo.m_fLocalUniformScaling = Selection[sel]->GetTypeAccessor().GetValue("LocalUniformScaling").ConvertTo<float>();
  }
}

void plGameObjectDocument::HandleEngineMessage(const plEditorEngineDocumentMsg* pMsg)
{
  SUPER::HandleEngineMessage(pMsg);

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plDocumentOpenResponseMsgToEditor>())
  {
    ScheduleSendObjectSelection();
  }
}
