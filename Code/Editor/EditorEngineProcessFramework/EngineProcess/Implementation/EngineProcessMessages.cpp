#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>

// clang-format off

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSyncWithProcessMsgToEngine, 1, plRTTIDefaultAllocator<plSyncWithProcessMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("RedrawCount", m_uiRedrawCount),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSyncWithProcessMsgToEditor, 1, plRTTIDefaultAllocator<plSyncWithProcessMsgToEditor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("RedrawCount", m_uiRedrawCount),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

///////////////////////////////////// PlasmaEditorEngineMsg /////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(PlasmaEditorEngineMsg, 1, plRTTINoAllocator )
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plUpdateReflectionTypeMsgToEditor, 1, plRTTIDefaultAllocator<plUpdateReflectionTypeMsgToEditor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Descriptor", m_desc),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSetupProjectMsgToEngine, 1, plRTTIDefaultAllocator<plSetupProjectMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ProjectDir", m_sProjectDir),
    PLASMA_MEMBER_PROPERTY("FileSystemConfig", m_FileSystemConfig),
    PLASMA_MEMBER_PROPERTY("PluginConfig", m_PluginConfig),
    PLASMA_MEMBER_PROPERTY("FileserveAddress", m_sFileserveAddress),
    PLASMA_MEMBER_PROPERTY("Platform", m_sAssetProfile),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plShutdownProcessMsgToEngine, 1, plRTTIDefaultAllocator<plShutdownProcessMsgToEngine>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProjectReadyMsgToEditor, 1, plRTTIDefaultAllocator<plProjectReadyMsgToEditor> )
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSimpleConfigMsgToEngine, 1, plRTTIDefaultAllocator<plSimpleConfigMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo),
    PLASMA_MEMBER_PROPERTY("Payload", m_sPayload),
    PLASMA_MEMBER_PROPERTY("PayloadValue", m_fPayload),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSaveProfilingResponseToEditor, 1, plRTTIDefaultAllocator<plSaveProfilingResponseToEditor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ProfilingFile", m_sProfilingFile),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plResourceUpdateMsgToEngine, 1, plRTTIDefaultAllocator<plResourceUpdateMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Type", m_sResourceType),
    PLASMA_MEMBER_PROPERTY("ID", m_sResourceID),
    PLASMA_MEMBER_PROPERTY("Data", m_Data),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRestoreResourceMsgToEngine, 1, plRTTIDefaultAllocator<plRestoreResourceMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Type", m_sResourceType),
    PLASMA_MEMBER_PROPERTY("ID", m_sResourceID),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plChangeCVarMsgToEngine, 1, plRTTIDefaultAllocator<plChangeCVarMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Name", m_sCVarName),
    PLASMA_MEMBER_PROPERTY("Value", m_NewValue),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plConsoleCmdMsgToEngine, 1, plRTTIDefaultAllocator<plConsoleCmdMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Type", m_iType),
    PLASMA_MEMBER_PROPERTY("Cmd", m_sCommand),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plConsoleCmdResultMsgToEditor, 1, plRTTIDefaultAllocator<plConsoleCmdResultMsgToEditor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Result", m_sResult),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDynamicStringEnumMsgToEditor, 1, plRTTIDefaultAllocator<plDynamicStringEnumMsgToEditor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("EnumName", m_sEnumName),
    PLASMA_ARRAY_MEMBER_PROPERTY("EnumValues", m_EnumValues),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLongOpReplicationMsg, 1, plRTTIDefaultAllocator<plLongOpReplicationMsg>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("OpGuid", m_OperationGuid),
    PLASMA_MEMBER_PROPERTY("DocGuid", m_DocumentGuid),
    PLASMA_MEMBER_PROPERTY("Type", m_sReplicationType),
    PLASMA_MEMBER_PROPERTY("Data", m_ReplicationData),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLongOpProgressMsg, 1, plRTTIDefaultAllocator<plLongOpProgressMsg>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("OpGuid", m_OperationGuid),
    PLASMA_MEMBER_PROPERTY("Completion", m_fCompletion),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLongOpResultMsg, 1, plRTTIDefaultAllocator<plLongOpResultMsg>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("OpGuid", m_OperationGuid),
    PLASMA_MEMBER_PROPERTY("Success", m_bSuccess),
    PLASMA_MEMBER_PROPERTY("Data", m_ResultData),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

///////////////////////////////////// PlasmaEditorEngineDocumentMsg /////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(PlasmaEditorEngineDocumentMsg, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("DocumentGuid", m_DocumentGuid),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDocumentConfigMsgToEngine, 1, plRTTIDefaultAllocator<plDocumentConfigMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo),
    PLASMA_MEMBER_PROPERTY("Int", m_iValue),
    PLASMA_MEMBER_PROPERTY("Float", m_fValue),
    PLASMA_MEMBER_PROPERTY("String", m_sValue),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(PlasmaEditorEngineViewMsg, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ViewID", m_uiViewID),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDocumentOpenMsgToEngine, 1, plRTTIDefaultAllocator<plDocumentOpenMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("DocumentOpen", m_bDocumentOpen),
    PLASMA_MEMBER_PROPERTY("DocumentType", m_sDocumentType),
    PLASMA_MEMBER_PROPERTY("DocumentMetaData", m_DocumentMetaData),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDocumentClearMsgToEngine, 1, plRTTIDefaultAllocator<plDocumentClearMsgToEngine>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDocumentOpenResponseMsgToEditor, 1, plRTTIDefaultAllocator<plDocumentOpenResponseMsgToEditor> )
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewDestroyedMsgToEngine, 1, plRTTIDefaultAllocator<plViewDestroyedMsgToEngine>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewDestroyedResponseMsgToEditor, 1, plRTTIDefaultAllocator<plViewDestroyedResponseMsgToEditor>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewRedrawMsgToEngine, 1, plRTTIDefaultAllocator<plViewRedrawMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("HWND", m_uiHWND),
    PLASMA_MEMBER_PROPERTY("WindowWidth", m_uiWindowWidth),
    PLASMA_MEMBER_PROPERTY("WindowHeight", m_uiWindowHeight),
    PLASMA_MEMBER_PROPERTY("UpdatePickingData", m_bUpdatePickingData),
    PLASMA_MEMBER_PROPERTY("EnablePickSelected", m_bEnablePickingSelected),
    PLASMA_MEMBER_PROPERTY("EnablePickTransparent", m_bEnablePickTransparent),
    PLASMA_MEMBER_PROPERTY("UseCamOnDevice", m_bUseCameraTransformOnDevice),
    PLASMA_MEMBER_PROPERTY("CameraMode", m_iCameraMode),
    PLASMA_MEMBER_PROPERTY("NearPlane", m_fNearPlane),
    PLASMA_MEMBER_PROPERTY("FarPlane", m_fFarPlane),
    PLASMA_MEMBER_PROPERTY("FovOrDim", m_fFovOrDim),
    PLASMA_MEMBER_PROPERTY("Position", m_vPosition),
    PLASMA_MEMBER_PROPERTY("Forwards", m_vDirForwards),
    PLASMA_MEMBER_PROPERTY("Up", m_vDirUp),
    PLASMA_MEMBER_PROPERTY("Right", m_vDirRight),
    PLASMA_MEMBER_PROPERTY("ViewMat", m_ViewMatrix),
    PLASMA_MEMBER_PROPERTY("ProjMat", m_ProjMatrix),
    PLASMA_MEMBER_PROPERTY("RenderMode", m_uiRenderMode),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewScreenshotMsgToEngine, 1, plRTTIDefaultAllocator<plViewScreenshotMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("File", m_sOutputFile)
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plActivateRemoteViewMsgToEngine, 1, plRTTIDefaultAllocator<plActivateRemoteViewMsgToEngine>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plEntityMsgToEngine, 1, plRTTIDefaultAllocator<plEntityMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Change", m_change),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSimpleDocumentConfigMsgToEngine, 1, plRTTIDefaultAllocator<plSimpleDocumentConfigMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo),
    PLASMA_MEMBER_PROPERTY("Payload", m_sPayload),
    PLASMA_MEMBER_PROPERTY("PayloadValue", m_fPayload),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSimpleDocumentConfigMsgToEditor, 1, plRTTIDefaultAllocator<plSimpleDocumentConfigMsgToEditor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Name", m_sName),
    PLASMA_MEMBER_PROPERTY("PayloadString", m_sPayload),
    PLASMA_MEMBER_PROPERTY("PayloadFloat", m_fPayload),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plExportDocumentMsgToEngine, 1, plRTTIDefaultAllocator<plExportDocumentMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("OutputFile", m_sOutputFile),
    PLASMA_MEMBER_PROPERTY("AssetHash", m_uiAssetHash),
    PLASMA_MEMBER_PROPERTY("AssetVersion", m_uiVersion),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plExportDocumentMsgToEditor, 1, plRTTIDefaultAllocator<plExportDocumentMsgToEditor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("OutputSuccess", m_bOutputSuccess),
    PLASMA_MEMBER_PROPERTY("FailureMsg", m_sFailureMsg),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCreateThumbnailMsgToEngine, 1, plRTTIDefaultAllocator<plCreateThumbnailMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Width", m_uiWidth),
    PLASMA_MEMBER_PROPERTY("Height", m_uiHeight),
    PLASMA_ARRAY_MEMBER_PROPERTY("ViewExcludeTags", m_ViewExcludeTags),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCreateThumbnailMsgToEditor, 1, plRTTIDefaultAllocator<plCreateThumbnailMsgToEditor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ThumbnailData", m_ThumbnailData),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewPickingMsgToEngine, 1, plRTTIDefaultAllocator<plViewPickingMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("PickPosX", m_uiPickPosX),
    PLASMA_MEMBER_PROPERTY("PickPosY", m_uiPickPosY),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewPickingResultMsgToEditor, 1, plRTTIDefaultAllocator<plViewPickingResultMsgToEditor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    PLASMA_MEMBER_PROPERTY("ComponentGuid", m_ComponentGuid),
    PLASMA_MEMBER_PROPERTY("OtherGuid", m_OtherGuid),
    PLASMA_MEMBER_PROPERTY("PartIndex", m_uiPartIndex),
    PLASMA_MEMBER_PROPERTY("PickedPos", m_vPickedPosition),
    PLASMA_MEMBER_PROPERTY("PickedNormal", m_vPickedNormal),
    PLASMA_MEMBER_PROPERTY("PickRayStart", m_vPickingRayStartPosition),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewMarqueePickingMsgToEngine, 1, plRTTIDefaultAllocator<plViewMarqueePickingMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("PickPosX0", m_uiPickPosX0),
    PLASMA_MEMBER_PROPERTY("PickPosY0", m_uiPickPosY0),
    PLASMA_MEMBER_PROPERTY("PickPosX1", m_uiPickPosX1),
    PLASMA_MEMBER_PROPERTY("PickPosY1", m_uiPickPosY1),
    PLASMA_MEMBER_PROPERTY("what", m_uiWhatToDo),
    PLASMA_MEMBER_PROPERTY("aid", m_uiActionIdentifier),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewMarqueePickingResultMsgToEditor, 1, plRTTIDefaultAllocator<plViewMarqueePickingResultMsgToEditor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY("Objects", m_ObjectGuids),
    PLASMA_MEMBER_PROPERTY("what", m_uiWhatToDo),
    PLASMA_MEMBER_PROPERTY("aid", m_uiActionIdentifier),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewHighlightMsgToEngine, 1, plRTTIDefaultAllocator<plViewHighlightMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("HighlightObject", m_HighlightObject),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLogMsgToEditor, 1, plRTTIDefaultAllocator<plLogMsgToEditor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Entry", m_Entry),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCVarMsgToEditor, 1, plRTTIDefaultAllocator<plCVarMsgToEditor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Name", m_sName),
    PLASMA_MEMBER_PROPERTY("Plugin", m_sPlugin),
    PLASMA_MEMBER_PROPERTY("Desc", m_sDescription),
    PLASMA_MEMBER_PROPERTY("Value", m_Value),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(PlasmaEditorEngineSyncObjectMsg, 1, plRTTIDefaultAllocator<PlasmaEditorEngineSyncObjectMsg>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    PLASMA_MEMBER_PROPERTY("ObjectType", m_sObjectType),
    PLASMA_ACCESSOR_PROPERTY("ObjectData", GetObjectData, SetObjectData),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plObjectTagMsgToEngine, 1, plRTTIDefaultAllocator<plObjectTagMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    PLASMA_MEMBER_PROPERTY("Tag", m_sTag),
    PLASMA_MEMBER_PROPERTY("Set", m_bSetTag),
    PLASMA_MEMBER_PROPERTY("Recursive", m_bApplyOnAllChildren),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plObjectSelectionMsgToEngine, 1, plRTTIDefaultAllocator<plObjectSelectionMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Selection", m_sSelection),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSimulationSettingsMsgToEngine, 1, plRTTIDefaultAllocator<plSimulationSettingsMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("SimulateWorld", m_bSimulateWorld),
    PLASMA_MEMBER_PROPERTY("SimulationSpeed", m_fSimulationSpeed),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGridSettingsMsgToEngine, 1, plRTTIDefaultAllocator<plGridSettingsMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("GridDensity", m_fGridDensity),
    PLASMA_MEMBER_PROPERTY("GridCenter", m_vGridCenter),
    PLASMA_MEMBER_PROPERTY("GridTangent1", m_vGridTangent1),
    PLASMA_MEMBER_PROPERTY("GridTangent2", m_vGridTangent2),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGlobalSettingsMsgToEngine, 1, plRTTIDefaultAllocator<plGlobalSettingsMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("GizmoScale", m_fGizmoScale),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plWorldSettingsMsgToEngine, 1, plRTTIDefaultAllocator<plWorldSettingsMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("RenderOverlay", m_bRenderOverlay),
    PLASMA_MEMBER_PROPERTY("ShapeIcons", m_bRenderShapeIcons),
    PLASMA_MEMBER_PROPERTY("RenderSelectionBoxes", m_bRenderSelectionBoxes),
    PLASMA_MEMBER_PROPERTY("AddAmbient", m_bAddAmbientLight),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGameModeMsgToEngine, 1, plRTTIDefaultAllocator<plGameModeMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Run", m_bEnablePTG),
    PLASMA_MEMBER_PROPERTY("UsePos", m_bUseStartPosition),
    PLASMA_MEMBER_PROPERTY("Pos", m_vStartPosition),
    PLASMA_MEMBER_PROPERTY("Dir", m_vStartDirection),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGameModeMsgToEditor, 1, plRTTIDefaultAllocator<plGameModeMsgToEditor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Run", m_bRunningPTG),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plQuerySelectionBBoxMsgToEngine, 1, plRTTIDefaultAllocator<plQuerySelectionBBoxMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ViewID", m_uiViewID),
    PLASMA_MEMBER_PROPERTY("Purpose", m_iPurpose),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plQuerySelectionBBoxResultMsgToEditor, 1, plRTTIDefaultAllocator<plQuerySelectionBBoxResultMsgToEditor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Center", m_vCenter),
    PLASMA_MEMBER_PROPERTY("Extents", m_vHalfExtents),
    PLASMA_MEMBER_PROPERTY("ViewID", m_uiViewID),
    PLASMA_MEMBER_PROPERTY("Purpose", m_iPurpose),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGatherObjectsOfTypeMsgInterDoc, 1, plRTTIDefaultAllocator<plGatherObjectsOfTypeMsgInterDoc>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGatherObjectsForDebugVisMsgInterDoc, 1, plRTTIDefaultAllocator<plGatherObjectsForDebugVisMsgInterDoc>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plObjectsForDebugVisMsgToEngine, 1, plRTTIDefaultAllocator<plObjectsForDebugVisMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Objects", m_Objects),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

