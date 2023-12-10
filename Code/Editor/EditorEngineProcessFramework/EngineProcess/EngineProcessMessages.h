#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/LogEntry.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

///////////////////////////////////// plProcessMessages /////////////////////////////////////



///////////////////////////////////// plEditorEngineMsg /////////////////////////////////////

/// \brief Base class for all messages between editor and engine that are not bound to any document
class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plEditorEngineMsg : public plProcessMessage
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plEditorEngineMsg, plProcessMessage);

public:
  plEditorEngineMsg() = default;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plUpdateReflectionTypeMsgToEditor : public plEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plUpdateReflectionTypeMsgToEditor, plEditorEngineMsg);

public:
  // Mutable because it is eaten up by plPhantomRttiManager.
  mutable plReflectedTypeDescriptor m_desc;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plSetupProjectMsgToEngine : public plEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSetupProjectMsgToEngine, plEditorEngineMsg);

public:
  plString m_sProjectDir;
  plApplicationFileSystemConfig m_FileSystemConfig;
  plApplicationPluginConfig m_PluginConfig;
  plString m_sFileserveAddress; ///< Optionally used for remote processes to tell them with which IP address to connect to the host
  plString m_sAssetProfile;
};

/// \brief Sent to remote processes to shut them down.
/// Local processes are simply killed through QProcess::close, but remote processes have to close themselves.
class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plShutdownProcessMsgToEngine : public plEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plShutdownProcessMsgToEngine, plEditorEngineMsg);

public:
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plProjectReadyMsgToEditor : public plEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProjectReadyMsgToEditor, plEditorEngineMsg);

public:
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plSimpleConfigMsgToEngine : public plEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSimpleConfigMsgToEngine, plEditorEngineMsg);

public:
  plString m_sWhatToDo;
  plString m_sPayload;
  double m_fPayload;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plSaveProfilingResponseToEditor : public plEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSaveProfilingResponseToEditor, plEditorEngineMsg);

public:
  plString m_sProfilingFile;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plReloadResourceMsgToEngine : public plEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plReloadResourceMsgToEngine, plEditorEngineMsg);

public:
  plString m_sResourceType;
  plString m_sResourceID;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plResourceUpdateMsgToEngine : public plEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plResourceUpdateMsgToEngine, plEditorEngineMsg);

public:
  plString m_sResourceType;
  plString m_sResourceID;
  plDataBuffer m_Data;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plRestoreResourceMsgToEngine : public plEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRestoreResourceMsgToEngine, plEditorEngineMsg);

public:
  plString m_sResourceType;
  plString m_sResourceID;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plChangeCVarMsgToEngine : public plEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plChangeCVarMsgToEngine, plEditorEngineMsg);

public:
  plString m_sCVarName;
  plVariant m_NewValue;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plConsoleCmdMsgToEngine : public plEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plConsoleCmdMsgToEngine, plEditorEngineMsg);

public:
  plInt8 m_iType; // 0 = execute, 1 = auto complete
  plString m_sCommand;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plConsoleCmdResultMsgToEditor : public plEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plConsoleCmdResultMsgToEditor, plEditorEngineMsg);

public:
  plString m_sResult;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plDynamicStringEnumMsgToEditor : public plEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDynamicStringEnumMsgToEditor, plEditorEngineMsg);

public:
  plString m_sEnumName;
  plHybridArray<plString, 8> m_EnumValues;
};

///////////////////////////////////// plEditorEngineDocumentMsg /////////////////////////////////////

/// \brief Base class for all messages that are tied to some document.
class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plEditorEngineDocumentMsg : public plProcessMessage
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plEditorEngineDocumentMsg, plProcessMessage);

public:
  plUuid m_DocumentGuid;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plSimpleDocumentConfigMsgToEngine : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSimpleDocumentConfigMsgToEngine, plEditorEngineDocumentMsg);

public:
  plString m_sWhatToDo;
  plString m_sPayload;
  double m_fPayload;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plSimpleDocumentConfigMsgToEditor : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSimpleDocumentConfigMsgToEditor, plEditorEngineDocumentMsg);

public:
  plString m_sName;
  plString m_sPayload;
  double m_fPayload;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plSyncWithProcessMsgToEngine : public plProcessMessage
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSyncWithProcessMsgToEngine, plProcessMessage);

public:
  plUInt32 m_uiRedrawCount;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plSyncWithProcessMsgToEditor : public plProcessMessage
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSyncWithProcessMsgToEditor, plProcessMessage);

public:
  plUInt32 m_uiRedrawCount;
};


class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plEditorEngineViewMsg : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plEditorEngineViewMsg, plEditorEngineDocumentMsg);

public:
  plEditorEngineViewMsg() { m_uiViewID = 0xFFFFFFFF; }

  plUInt32 m_uiViewID;
};

/// \brief For very simple uses cases where a custom message would be too much
class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plDocumentConfigMsgToEngine : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDocumentConfigMsgToEngine, plEditorEngineDocumentMsg);

public:
  plString m_sWhatToDo;
  int m_iValue;
  float m_fValue;
  plString m_sValue;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plDocumentOpenMsgToEngine : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDocumentOpenMsgToEngine, plEditorEngineDocumentMsg);

public:
  plDocumentOpenMsgToEngine() { m_bDocumentOpen = false; }

  bool m_bDocumentOpen;
  plString m_sDocumentType;
  plVariant m_DocumentMetaData;
};

/// \brief Used to reset the engine side to an empty document before sending the full document state over
class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plDocumentClearMsgToEngine : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDocumentClearMsgToEngine, plEditorEngineDocumentMsg);

public:
  plDocumentClearMsgToEngine() = default;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plDocumentOpenResponseMsgToEditor : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDocumentOpenResponseMsgToEditor, plEditorEngineDocumentMsg);

public:
  plDocumentOpenResponseMsgToEditor() = default;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plViewDestroyedMsgToEngine : public plEditorEngineViewMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plViewDestroyedMsgToEngine, plEditorEngineViewMsg);
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plViewDestroyedResponseMsgToEditor : public plEditorEngineViewMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plViewDestroyedResponseMsgToEditor, plEditorEngineViewMsg);
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plViewRedrawMsgToEngine : public plEditorEngineViewMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plViewRedrawMsgToEngine, plEditorEngineViewMsg);

public:
  plUInt64 m_uiHWND;
  plUInt16 m_uiWindowWidth;
  plUInt16 m_uiWindowHeight;
  bool m_bUpdatePickingData;
  bool m_bEnablePickingSelected;
  bool m_bEnablePickTransparent;
  bool m_bUseCameraTransformOnDevice = true;

  plInt8 m_iCameraMode; ///< plCameraMode::Enum
  float m_fNearPlane;
  float m_fFarPlane;
  float m_fFovOrDim;
  plUInt8 m_uiRenderMode; ///< plViewRenderMode::Enum

  plVec3 m_vPosition;
  plVec3 m_vDirForwards;
  plVec3 m_vDirUp;
  plVec3 m_vDirRight;
  plMat4 m_ViewMatrix;
  plMat4 m_ProjMatrix;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plViewScreenshotMsgToEngine : public plEditorEngineViewMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plViewScreenshotMsgToEngine, plEditorEngineViewMsg);

public:
  plString m_sOutputFile;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plActivateRemoteViewMsgToEngine : public plEditorEngineViewMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plActivateRemoteViewMsgToEngine, plEditorEngineViewMsg);

public:
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plEntityMsgToEngine : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plEntityMsgToEngine, plEditorEngineDocumentMsg);

public:
  plObjectChange m_change;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plExportDocumentMsgToEngine : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plExportDocumentMsgToEngine, plEditorEngineDocumentMsg);

public:
  plExportDocumentMsgToEngine()

    = default;

  plString m_sOutputFile;
  plUInt64 m_uiAssetHash = 0;
  plUInt16 m_uiVersion = 0;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plExportDocumentMsgToEditor : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plExportDocumentMsgToEditor, plEditorEngineDocumentMsg);

public:
  bool m_bOutputSuccess = false;
  plString m_sFailureMsg;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plCreateThumbnailMsgToEngine : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCreateThumbnailMsgToEngine, plEditorEngineDocumentMsg);

public:
  plUInt16 m_uiWidth = 0;
  plUInt16 m_uiHeight = 0;
  plHybridArray<plString, 1> m_ViewExcludeTags;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plCreateThumbnailMsgToEditor : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCreateThumbnailMsgToEditor, plEditorEngineDocumentMsg);

public:
  plCreateThumbnailMsgToEditor() = default;
  plDataBuffer m_ThumbnailData; ///< Raw 8-bit RGBA data (256x256x4 bytes)
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plViewPickingMsgToEngine : public plEditorEngineViewMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plViewPickingMsgToEngine, plEditorEngineViewMsg);

public:
  plUInt16 m_uiPickPosX;
  plUInt16 m_uiPickPosY;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plViewPickingResultMsgToEditor : public plEditorEngineViewMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plViewPickingResultMsgToEditor, plEditorEngineViewMsg);

public:
  plUuid m_ObjectGuid;
  plUuid m_ComponentGuid;
  plUuid m_OtherGuid;
  plUInt32 m_uiPartIndex;

  plVec3 m_vPickedPosition;
  plVec3 m_vPickedNormal;
  plVec3 m_vPickingRayStartPosition;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plViewMarqueePickingMsgToEngine : public plEditorEngineViewMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plViewMarqueePickingMsgToEngine, plEditorEngineViewMsg);

public:
  plUInt16 m_uiPickPosX0;
  plUInt16 m_uiPickPosY0;

  plUInt16 m_uiPickPosX1;
  plUInt16 m_uiPickPosY1;

  plUInt8 m_uiWhatToDo; // 0 == select, 1 == add, 2 == remove
  plUInt32 m_uiActionIdentifier;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plViewMarqueePickingResultMsgToEditor : public plEditorEngineViewMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plViewMarqueePickingResultMsgToEditor, plEditorEngineViewMsg);

public:
  plDynamicArray<plUuid> m_ObjectGuids;
  plUInt8 m_uiWhatToDo; // 0 == select, 1 == add, 2 == remove
  plUInt32 m_uiActionIdentifier;
};


class plEditorEngineConnection;

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plViewHighlightMsgToEngine : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plViewHighlightMsgToEngine, plEditorEngineDocumentMsg);

public:
  plUuid m_HighlightObject;
  // currently used for highlighting which object the mouse hovers over
  // extend this message if other types of highlighting become necessary
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plLogMsgToEditor : public plEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLogMsgToEditor, plEditorEngineMsg);

public:
  plLogEntry m_Entry;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plCVarMsgToEditor : public plEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCVarMsgToEditor, plEditorEngineMsg);

public:
  plString m_sName;
  plString m_sPlugin;
  plString m_sDescription;
  plVariant m_Value;
};


class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plLongOpReplicationMsg : public plEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLongOpReplicationMsg, plEditorEngineMsg);

public:
  plUuid m_OperationGuid;
  plUuid m_DocumentGuid;
  plString m_sReplicationType;
  plDataBuffer m_ReplicationData;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plLongOpProgressMsg : public plEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLongOpProgressMsg, plEditorEngineMsg);

public:
  plUuid m_OperationGuid;
  float m_fCompletion;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plLongOpResultMsg : public plEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLongOpResultMsg, plEditorEngineMsg);

public:
  plUuid m_OperationGuid;
  bool m_bSuccess;
  plDataBuffer m_ResultData;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plEditorEngineSyncObjectMsg : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plEditorEngineSyncObjectMsg, plEditorEngineDocumentMsg);

public:
  plUuid m_ObjectGuid;
  plString m_sObjectType;
  plDataBuffer m_ObjectData;

  const plDataBuffer& GetObjectData() const { return m_ObjectData; }
  void SetObjectData(const plDataBuffer& s) { m_ObjectData = s; }
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plObjectTagMsgToEngine : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plObjectTagMsgToEngine, plEditorEngineDocumentMsg);

public:
  plObjectTagMsgToEngine()
  {
    m_bSetTag = false;
    m_bApplyOnAllChildren = false;
  }

  plUuid m_ObjectGuid;
  plString m_sTag;
  bool m_bSetTag;
  bool m_bApplyOnAllChildren;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plObjectSelectionMsgToEngine : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plObjectSelectionMsgToEngine, plEditorEngineDocumentMsg);

public:
  plString m_sSelection;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plSimulationSettingsMsgToEngine : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSimulationSettingsMsgToEngine, plEditorEngineDocumentMsg);

public:
  bool m_bSimulateWorld = false;
  float m_fSimulationSpeed = 1.0f;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plGridSettingsMsgToEngine : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGridSettingsMsgToEngine, plEditorEngineDocumentMsg);

public:
  float m_fGridDensity = 0.0f;
  plVec3 m_vGridCenter;
  plVec3 m_vGridTangent1;
  plVec3 m_vGridTangent2;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plGlobalSettingsMsgToEngine : public plEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGlobalSettingsMsgToEngine, plEditorEngineMsg);

public:
  float m_fGizmoScale = 0.0f;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plWorldSettingsMsgToEngine : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plWorldSettingsMsgToEngine, plEditorEngineDocumentMsg);

public:
  bool m_bRenderOverlay = false;
  bool m_bRenderShapeIcons = false;
  bool m_bRenderSelectionBoxes = false;
  bool m_bAddAmbientLight = false;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plGameModeMsgToEngine : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGameModeMsgToEngine, plEditorEngineDocumentMsg);

public:
  bool m_bEnablePTG = false;
  bool m_bUseStartPosition = false;
  plVec3 m_vStartPosition;
  plVec3 m_vStartDirection;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plGameModeMsgToEditor : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGameModeMsgToEditor, plEditorEngineDocumentMsg);

public:
  bool m_bRunningPTG;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plQuerySelectionBBoxMsgToEngine : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plQuerySelectionBBoxMsgToEngine, plEditorEngineDocumentMsg);

public:
  plUInt32 m_uiViewID; /// passed through to plQuerySelectionBBoxResultMsgToEditor
  plInt32 m_iPurpose;  /// passed through to plQuerySelectionBBoxResultMsgToEditor
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plQuerySelectionBBoxResultMsgToEditor : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plQuerySelectionBBoxResultMsgToEditor, plEditorEngineDocumentMsg);

public:
  plVec3 m_vCenter;
  plVec3 m_vHalfExtents;

  plUInt32 m_uiViewID; /// passed through from plQuerySelectionBBoxMsgToEngine
  plInt32 m_iPurpose;  /// passed through from plQuerySelectionBBoxMsgToEngine
};

/// \brief Send between editor documents, such that one document can know about objects in another document.
class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plGatherObjectsOfTypeMsgInterDoc : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGatherObjectsOfTypeMsgInterDoc, plReflectedClass);

public:
  const plRTTI* m_pType;

  struct Result
  {
    const plDocument* m_pDocument;
    plUuid m_ObjectGuid;
    plString m_sDisplayName;
  };

  plDynamicArray<Result> m_Results;
};

/// Send by the editor scene document to all other editor documents, to gather on which objects debug visualization should be enabled during
/// play-the-game.
class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plGatherObjectsForDebugVisMsgInterDoc : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGatherObjectsForDebugVisMsgInterDoc, plReflectedClass);

public:
  plDynamicArray<plUuid> m_Objects;
};

/// Send by the editor scene document to the runtime scene document, to tell it about the poll results (see plGatherObjectsForDebugVisMsgInterDoc).
class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plObjectsForDebugVisMsgToEngine : public plEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plObjectsForDebugVisMsgToEngine, plEditorEngineDocumentMsg);

public:
  plDataBuffer m_Objects;
};
