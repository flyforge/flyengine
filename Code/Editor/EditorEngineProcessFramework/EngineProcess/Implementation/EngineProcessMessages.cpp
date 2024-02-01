#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>

// clang-format off

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSyncWithProcessMsgToEngine, 1, plRTTIDefaultAllocator<plSyncWithProcessMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("RedrawCount", m_uiRedrawCount),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSyncWithProcessMsgToEditor, 1, plRTTIDefaultAllocator<plSyncWithProcessMsgToEditor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("RedrawCount", m_uiRedrawCount),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

///////////////////////////////////// plEditorEngineMsg /////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditorEngineMsg, 1, plRTTINoAllocator )
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plUpdateReflectionTypeMsgToEditor, 1, plRTTIDefaultAllocator<plUpdateReflectionTypeMsgToEditor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Descriptor", m_desc),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSetupProjectMsgToEngine, 1, plRTTIDefaultAllocator<plSetupProjectMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ProjectDir", m_sProjectDir),
    PL_MEMBER_PROPERTY("FileSystemConfig", m_FileSystemConfig),
    PL_MEMBER_PROPERTY("PluginConfig", m_PluginConfig),
    PL_MEMBER_PROPERTY("FileserveAddress", m_sFileserveAddress),
    PL_MEMBER_PROPERTY("Platform", m_sAssetProfile),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plShutdownProcessMsgToEngine, 1, plRTTIDefaultAllocator<plShutdownProcessMsgToEngine>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProjectReadyMsgToEditor, 1, plRTTIDefaultAllocator<plProjectReadyMsgToEditor> )
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSimpleConfigMsgToEngine, 1, plRTTIDefaultAllocator<plSimpleConfigMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo),
    PL_MEMBER_PROPERTY("Payload", m_sPayload),
    PL_MEMBER_PROPERTY("PayloadValue", m_fPayload),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSaveProfilingResponseToEditor, 1, plRTTIDefaultAllocator<plSaveProfilingResponseToEditor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ProfilingFile", m_sProfilingFile),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plReloadResourceMsgToEngine, 1, plRTTIDefaultAllocator<plReloadResourceMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Type", m_sResourceType),
    PL_MEMBER_PROPERTY("ID", m_sResourceID),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plResourceUpdateMsgToEngine, 1, plRTTIDefaultAllocator<plResourceUpdateMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Type", m_sResourceType),
    PL_MEMBER_PROPERTY("ID", m_sResourceID),
    PL_MEMBER_PROPERTY("Data", m_Data),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRestoreResourceMsgToEngine, 1, plRTTIDefaultAllocator<plRestoreResourceMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Type", m_sResourceType),
    PL_MEMBER_PROPERTY("ID", m_sResourceID),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plChangeCVarMsgToEngine, 1, plRTTIDefaultAllocator<plChangeCVarMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Name", m_sCVarName),
    PL_MEMBER_PROPERTY("Value", m_NewValue),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plConsoleCmdMsgToEngine, 1, plRTTIDefaultAllocator<plConsoleCmdMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Type", m_iType),
    PL_MEMBER_PROPERTY("Cmd", m_sCommand),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plConsoleCmdResultMsgToEditor, 1, plRTTIDefaultAllocator<plConsoleCmdResultMsgToEditor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Result", m_sResult),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDynamicStringEnumMsgToEditor, 1, plRTTIDefaultAllocator<plDynamicStringEnumMsgToEditor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("EnumName", m_sEnumName),
    PL_ARRAY_MEMBER_PROPERTY("EnumValues", m_EnumValues),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLongOpReplicationMsg, 1, plRTTIDefaultAllocator<plLongOpReplicationMsg>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("OpGuid", m_OperationGuid),
    PL_MEMBER_PROPERTY("DocGuid", m_DocumentGuid),
    PL_MEMBER_PROPERTY("Type", m_sReplicationType),
    PL_MEMBER_PROPERTY("Data", m_ReplicationData),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLongOpProgressMsg, 1, plRTTIDefaultAllocator<plLongOpProgressMsg>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("OpGuid", m_OperationGuid),
    PL_MEMBER_PROPERTY("Completion", m_fCompletion),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLongOpResultMsg, 1, plRTTIDefaultAllocator<plLongOpResultMsg>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("OpGuid", m_OperationGuid),
    PL_MEMBER_PROPERTY("Success", m_bSuccess),
    PL_MEMBER_PROPERTY("Data", m_ResultData),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

///////////////////////////////////// plEditorEngineDocumentMsg /////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditorEngineDocumentMsg, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("DocumentGuid", m_DocumentGuid),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDocumentConfigMsgToEngine, 1, plRTTIDefaultAllocator<plDocumentConfigMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo),
    PL_MEMBER_PROPERTY("Int", m_iValue),
    PL_MEMBER_PROPERTY("Float", m_fValue),
    PL_MEMBER_PROPERTY("String", m_sValue),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditorEngineViewMsg, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ViewID", m_uiViewID),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDocumentOpenMsgToEngine, 1, plRTTIDefaultAllocator<plDocumentOpenMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("DocumentOpen", m_bDocumentOpen),
    PL_MEMBER_PROPERTY("DocumentType", m_sDocumentType),
    PL_MEMBER_PROPERTY("DocumentMetaData", m_DocumentMetaData),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDocumentClearMsgToEngine, 1, plRTTIDefaultAllocator<plDocumentClearMsgToEngine>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDocumentOpenResponseMsgToEditor, 1, plRTTIDefaultAllocator<plDocumentOpenResponseMsgToEditor> )
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewDestroyedMsgToEngine, 1, plRTTIDefaultAllocator<plViewDestroyedMsgToEngine>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewDestroyedResponseMsgToEditor, 1, plRTTIDefaultAllocator<plViewDestroyedResponseMsgToEditor>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewRedrawMsgToEngine, 1, plRTTIDefaultAllocator<plViewRedrawMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("HWND", m_uiHWND),
    PL_MEMBER_PROPERTY("WindowWidth", m_uiWindowWidth),
    PL_MEMBER_PROPERTY("WindowHeight", m_uiWindowHeight),
    PL_MEMBER_PROPERTY("UpdatePickingData", m_bUpdatePickingData),
    PL_MEMBER_PROPERTY("EnablePickSelected", m_bEnablePickingSelected),
    PL_MEMBER_PROPERTY("EnablePickTransparent", m_bEnablePickTransparent),
    PL_MEMBER_PROPERTY("UseCamOnDevice", m_bUseCameraTransformOnDevice),
    PL_MEMBER_PROPERTY("CameraMode", m_iCameraMode),
    PL_MEMBER_PROPERTY("NearPlane", m_fNearPlane),
    PL_MEMBER_PROPERTY("FarPlane", m_fFarPlane),
    PL_MEMBER_PROPERTY("FovOrDim", m_fFovOrDim),
    PL_MEMBER_PROPERTY("Position", m_vPosition),
    PL_MEMBER_PROPERTY("Forwards", m_vDirForwards),
    PL_MEMBER_PROPERTY("Up", m_vDirUp),
    PL_MEMBER_PROPERTY("Right", m_vDirRight),
    PL_MEMBER_PROPERTY("ViewMat", m_ViewMatrix),
    PL_MEMBER_PROPERTY("ProjMat", m_ProjMatrix),
    PL_MEMBER_PROPERTY("RenderMode", m_uiRenderMode),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewScreenshotMsgToEngine, 1, plRTTIDefaultAllocator<plViewScreenshotMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("File", m_sOutputFile)
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plActivateRemoteViewMsgToEngine, 1, plRTTIDefaultAllocator<plActivateRemoteViewMsgToEngine>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plEntityMsgToEngine, 1, plRTTIDefaultAllocator<plEntityMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Change", m_change),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSimpleDocumentConfigMsgToEngine, 1, plRTTIDefaultAllocator<plSimpleDocumentConfigMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("WhatToDo", m_sWhatToDo),
    PL_MEMBER_PROPERTY("Payload", m_sPayload),
    PL_MEMBER_PROPERTY("PayloadValue", m_fPayload),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSimpleDocumentConfigMsgToEditor, 1, plRTTIDefaultAllocator<plSimpleDocumentConfigMsgToEditor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Name", m_sName),
    PL_MEMBER_PROPERTY("PayloadString", m_sPayload),
    PL_MEMBER_PROPERTY("PayloadFloat", m_fPayload),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plExportDocumentMsgToEngine, 1, plRTTIDefaultAllocator<plExportDocumentMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("OutputFile", m_sOutputFile),
    PL_MEMBER_PROPERTY("AssetHash", m_uiAssetHash),
    PL_MEMBER_PROPERTY("AssetVersion", m_uiVersion),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plExportDocumentMsgToEditor, 1, plRTTIDefaultAllocator<plExportDocumentMsgToEditor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("OutputSuccess", m_bOutputSuccess),
    PL_MEMBER_PROPERTY("FailureMsg", m_sFailureMsg),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCreateThumbnailMsgToEngine, 1, plRTTIDefaultAllocator<plCreateThumbnailMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Width", m_uiWidth),
    PL_MEMBER_PROPERTY("Height", m_uiHeight),
    PL_ARRAY_MEMBER_PROPERTY("ViewExcludeTags", m_ViewExcludeTags),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCreateThumbnailMsgToEditor, 1, plRTTIDefaultAllocator<plCreateThumbnailMsgToEditor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ThumbnailData", m_ThumbnailData),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewPickingMsgToEngine, 1, plRTTIDefaultAllocator<plViewPickingMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("PickPosX", m_uiPickPosX),
    PL_MEMBER_PROPERTY("PickPosY", m_uiPickPosY),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewPickingResultMsgToEditor, 1, plRTTIDefaultAllocator<plViewPickingResultMsgToEditor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    PL_MEMBER_PROPERTY("ComponentGuid", m_ComponentGuid),
    PL_MEMBER_PROPERTY("OtherGuid", m_OtherGuid),
    PL_MEMBER_PROPERTY("PartIndex", m_uiPartIndex),
    PL_MEMBER_PROPERTY("PickedPos", m_vPickedPosition),
    PL_MEMBER_PROPERTY("PickedNormal", m_vPickedNormal),
    PL_MEMBER_PROPERTY("PickRayStart", m_vPickingRayStartPosition),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewMarqueePickingMsgToEngine, 1, plRTTIDefaultAllocator<plViewMarqueePickingMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("PickPosX0", m_uiPickPosX0),
    PL_MEMBER_PROPERTY("PickPosY0", m_uiPickPosY0),
    PL_MEMBER_PROPERTY("PickPosX1", m_uiPickPosX1),
    PL_MEMBER_PROPERTY("PickPosY1", m_uiPickPosY1),
    PL_MEMBER_PROPERTY("what", m_uiWhatToDo),
    PL_MEMBER_PROPERTY("aid", m_uiActionIdentifier),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewMarqueePickingResultMsgToEditor, 1, plRTTIDefaultAllocator<plViewMarqueePickingResultMsgToEditor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ARRAY_MEMBER_PROPERTY("Objects", m_ObjectGuids),
    PL_MEMBER_PROPERTY("what", m_uiWhatToDo),
    PL_MEMBER_PROPERTY("aid", m_uiActionIdentifier),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewHighlightMsgToEngine, 1, plRTTIDefaultAllocator<plViewHighlightMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("HighlightObject", m_HighlightObject),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLogMsgToEditor, 1, plRTTIDefaultAllocator<plLogMsgToEditor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Entry", m_Entry),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCVarMsgToEditor, 1, plRTTIDefaultAllocator<plCVarMsgToEditor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Name", m_sName),
    PL_MEMBER_PROPERTY("Plugin", m_sPlugin),
    PL_MEMBER_PROPERTY("Desc", m_sDescription),
    PL_MEMBER_PROPERTY("Value", m_Value),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditorEngineSyncObjectMsg, 1, plRTTIDefaultAllocator<plEditorEngineSyncObjectMsg>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    PL_MEMBER_PROPERTY("ObjectType", m_sObjectType),
    PL_ACCESSOR_PROPERTY("ObjectData", GetObjectData, SetObjectData),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plObjectTagMsgToEngine, 1, plRTTIDefaultAllocator<plObjectTagMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ObjectGuid", m_ObjectGuid),
    PL_MEMBER_PROPERTY("Tag", m_sTag),
    PL_MEMBER_PROPERTY("Set", m_bSetTag),
    PL_MEMBER_PROPERTY("Recursive", m_bApplyOnAllChildren),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plObjectSelectionMsgToEngine, 1, plRTTIDefaultAllocator<plObjectSelectionMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Selection", m_sSelection),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSimulationSettingsMsgToEngine, 1, plRTTIDefaultAllocator<plSimulationSettingsMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("SimulateWorld", m_bSimulateWorld),
    PL_MEMBER_PROPERTY("SimulationSpeed", m_fSimulationSpeed),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plGridSettingsMsgToEngine, 1, plRTTIDefaultAllocator<plGridSettingsMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("GridDensity", m_fGridDensity),
    PL_MEMBER_PROPERTY("GridCenter", m_vGridCenter),
    PL_MEMBER_PROPERTY("GridTangent1", m_vGridTangent1),
    PL_MEMBER_PROPERTY("GridTangent2", m_vGridTangent2),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plGlobalSettingsMsgToEngine, 1, plRTTIDefaultAllocator<plGlobalSettingsMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("GizmoScale", m_fGizmoScale),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plWorldSettingsMsgToEngine, 1, plRTTIDefaultAllocator<plWorldSettingsMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("RenderOverlay", m_bRenderOverlay),
    PL_MEMBER_PROPERTY("ShapeIcons", m_bRenderShapeIcons),
    PL_MEMBER_PROPERTY("RenderSelectionBoxes", m_bRenderSelectionBoxes),
    PL_MEMBER_PROPERTY("AddAmbient", m_bAddAmbientLight),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plGameModeMsgToEngine, 1, plRTTIDefaultAllocator<plGameModeMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Run", m_bEnablePTG),
    PL_MEMBER_PROPERTY("UsePos", m_bUseStartPosition),
    PL_MEMBER_PROPERTY("Pos", m_vStartPosition),
    PL_MEMBER_PROPERTY("Dir", m_vStartDirection),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plGameModeMsgToEditor, 1, plRTTIDefaultAllocator<plGameModeMsgToEditor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Run", m_bRunningPTG),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plQuerySelectionBBoxMsgToEngine, 1, plRTTIDefaultAllocator<plQuerySelectionBBoxMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ViewID", m_uiViewID),
    PL_MEMBER_PROPERTY("Purpose", m_iPurpose),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plQuerySelectionBBoxResultMsgToEditor, 1, plRTTIDefaultAllocator<plQuerySelectionBBoxResultMsgToEditor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Center", m_vCenter),
    PL_MEMBER_PROPERTY("Extents", m_vHalfExtents),
    PL_MEMBER_PROPERTY("ViewID", m_uiViewID),
    PL_MEMBER_PROPERTY("Purpose", m_iPurpose),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plGatherObjectsOfTypeMsgInterDoc, 1, plRTTIDefaultAllocator<plGatherObjectsOfTypeMsgInterDoc>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plGatherObjectsForDebugVisMsgInterDoc, 1, plRTTIDefaultAllocator<plGatherObjectsForDebugVisMsgInterDoc>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plObjectsForDebugVisMsgToEngine, 1, plRTTIDefaultAllocator<plObjectsForDebugVisMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Objects", m_Objects),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

