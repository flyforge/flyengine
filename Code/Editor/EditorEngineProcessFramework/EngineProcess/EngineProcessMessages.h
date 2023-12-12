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



///////////////////////////////////// PlasmaEditorEngineMsg /////////////////////////////////////

/// \brief Base class for all messages between editor and engine that are not bound to any document
class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL PlasmaEditorEngineMsg : public plProcessMessage
{
  PLASMA_ADD_DYNAMIC_REFLECTION(PlasmaEditorEngineMsg, plProcessMessage);

public:
  PlasmaEditorEngineMsg() {}
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plUpdateReflectionTypeMsgToEditor : public PlasmaEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plUpdateReflectionTypeMsgToEditor, PlasmaEditorEngineMsg);

public:
  // Mutable because it is eaten up by plPhantomRttiManager.
  mutable plReflectedTypeDescriptor m_desc;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plSetupProjectMsgToEngine : public PlasmaEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSetupProjectMsgToEngine, PlasmaEditorEngineMsg);

public:
  plString m_sProjectDir;
  plApplicationFileSystemConfig m_FileSystemConfig;
  plApplicationPluginConfig m_PluginConfig;
  plString m_sFileserveAddress; ///< Optionally used for remote processes to tell them with which IP address to connect to the host
  plString m_sAssetProfile;
};

/// \brief Sent to remote processes to shut them down.
/// Local processes are simply killed through QProcess::close, but remote processes have to close themselves.
class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plShutdownProcessMsgToEngine : public PlasmaEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plShutdownProcessMsgToEngine, PlasmaEditorEngineMsg);

public:
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plProjectReadyMsgToEditor : public PlasmaEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plProjectReadyMsgToEditor, PlasmaEditorEngineMsg);

public:
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plSimpleConfigMsgToEngine : public PlasmaEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSimpleConfigMsgToEngine, PlasmaEditorEngineMsg);

public:
  plString m_sWhatToDo;
  plString m_sPayload;
  double m_fPayload;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plSaveProfilingResponseToEditor : public PlasmaEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSaveProfilingResponseToEditor, PlasmaEditorEngineMsg);

public:
  plString m_sProfilingFile;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plResourceUpdateMsgToEngine : public PlasmaEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plResourceUpdateMsgToEngine, PlasmaEditorEngineMsg);

public:
  plString m_sResourceType;
  plString m_sResourceID;
  plDataBuffer m_Data;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plRestoreResourceMsgToEngine : public PlasmaEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRestoreResourceMsgToEngine, PlasmaEditorEngineMsg);

public:
  plString m_sResourceType;
  plString m_sResourceID;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plChangeCVarMsgToEngine : public PlasmaEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plChangeCVarMsgToEngine, PlasmaEditorEngineMsg);

public:
  plString m_sCVarName;
  plVariant m_NewValue;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plConsoleCmdMsgToEngine : public PlasmaEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plConsoleCmdMsgToEngine, PlasmaEditorEngineMsg);

public:
  plInt8 m_iType; // 0 = execute, 1 = auto complete
  plString m_sCommand;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plConsoleCmdResultMsgToEditor : public PlasmaEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plConsoleCmdResultMsgToEditor, PlasmaEditorEngineMsg);

public:
  plString m_sResult;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plDynamicStringEnumMsgToEditor : public PlasmaEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDynamicStringEnumMsgToEditor, PlasmaEditorEngineMsg);

public:
  plString m_sEnumName;
  plHybridArray<plString, 8> m_EnumValues;
};

///////////////////////////////////// PlasmaEditorEngineDocumentMsg /////////////////////////////////////

/// \brief Base class for all messages that are tied to some document.
class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL PlasmaEditorEngineDocumentMsg : public plProcessMessage
{
  PLASMA_ADD_DYNAMIC_REFLECTION(PlasmaEditorEngineDocumentMsg, plProcessMessage);

public:
  plUuid m_DocumentGuid;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plSimpleDocumentConfigMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSimpleDocumentConfigMsgToEngine, PlasmaEditorEngineDocumentMsg);

public:
  plString m_sWhatToDo;
  plString m_sPayload;
  double m_fPayload;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plSimpleDocumentConfigMsgToEditor : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSimpleDocumentConfigMsgToEditor, PlasmaEditorEngineDocumentMsg);

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


class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL PlasmaEditorEngineViewMsg : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(PlasmaEditorEngineViewMsg, PlasmaEditorEngineDocumentMsg);

public:
  PlasmaEditorEngineViewMsg() { m_uiViewID = 0xFFFFFFFF; }

  plUInt32 m_uiViewID;
};

/// \brief For very simple uses cases where a custom message would be too much
class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plDocumentConfigMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDocumentConfigMsgToEngine, PlasmaEditorEngineDocumentMsg);

public:
  plString m_sWhatToDo;
  int m_iValue;
  float m_fValue;
  plString m_sValue;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plDocumentOpenMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDocumentOpenMsgToEngine, PlasmaEditorEngineDocumentMsg);

public:
  plDocumentOpenMsgToEngine() { m_bDocumentOpen = false; }

  bool m_bDocumentOpen;
  plString m_sDocumentType;
  plVariant m_DocumentMetaData;
};

/// \brief Used to reset the engine side to an empty document before sending the full document state over
class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plDocumentClearMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDocumentClearMsgToEngine, PlasmaEditorEngineDocumentMsg);

public:
  plDocumentClearMsgToEngine() {}
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plDocumentOpenResponseMsgToEditor : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDocumentOpenResponseMsgToEditor, PlasmaEditorEngineDocumentMsg);

public:
  plDocumentOpenResponseMsgToEditor() {}
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plViewDestroyedMsgToEngine : public PlasmaEditorEngineViewMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plViewDestroyedMsgToEngine, PlasmaEditorEngineViewMsg);
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plViewDestroyedResponseMsgToEditor : public PlasmaEditorEngineViewMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plViewDestroyedResponseMsgToEditor, PlasmaEditorEngineViewMsg);
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plViewRedrawMsgToEngine : public PlasmaEditorEngineViewMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plViewRedrawMsgToEngine, PlasmaEditorEngineViewMsg);

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

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plViewScreenshotMsgToEngine : public PlasmaEditorEngineViewMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plViewScreenshotMsgToEngine, PlasmaEditorEngineViewMsg);

public:
  plString m_sOutputFile;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plActivateRemoteViewMsgToEngine : public PlasmaEditorEngineViewMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plActivateRemoteViewMsgToEngine, PlasmaEditorEngineViewMsg);

public:
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plEntityMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plEntityMsgToEngine, PlasmaEditorEngineDocumentMsg);

public:
  plObjectChange m_change;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plExportDocumentMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plExportDocumentMsgToEngine, PlasmaEditorEngineDocumentMsg);

public:
  plExportDocumentMsgToEngine()
    : m_uiAssetHash(0)
    , m_uiVersion(0)
  {
  }

  plString m_sOutputFile;
  plUInt64 m_uiAssetHash;
  plUInt16 m_uiVersion;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plExportDocumentMsgToEditor : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plExportDocumentMsgToEditor, PlasmaEditorEngineDocumentMsg);

public:
  bool m_bOutputSuccess = false;
  plString m_sFailureMsg;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plCreateThumbnailMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCreateThumbnailMsgToEngine, PlasmaEditorEngineDocumentMsg);

public:
  plUInt16 m_uiWidth = 0;
  plUInt16 m_uiHeight = 0;
  plHybridArray<plString, 1> m_ViewExcludeTags;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plCreateThumbnailMsgToEditor : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCreateThumbnailMsgToEditor, PlasmaEditorEngineDocumentMsg);

public:
  plCreateThumbnailMsgToEditor() {}
  plDataBuffer m_ThumbnailData; ///< Raw 8-bit RGBA data (256x256x4 bytes)
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plViewPickingMsgToEngine : public PlasmaEditorEngineViewMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plViewPickingMsgToEngine, PlasmaEditorEngineViewMsg);

public:
  plUInt16 m_uiPickPosX;
  plUInt16 m_uiPickPosY;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plViewPickingResultMsgToEditor : public PlasmaEditorEngineViewMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plViewPickingResultMsgToEditor, PlasmaEditorEngineViewMsg);

public:
  plUuid m_ObjectGuid;
  plUuid m_ComponentGuid;
  plUuid m_OtherGuid;
  plUInt32 m_uiPartIndex;

  plVec3 m_vPickedPosition;
  plVec3 m_vPickedNormal;
  plVec3 m_vPickingRayStartPosition;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plViewMarqueePickingMsgToEngine : public PlasmaEditorEngineViewMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plViewMarqueePickingMsgToEngine, PlasmaEditorEngineViewMsg);

public:
  plUInt16 m_uiPickPosX0;
  plUInt16 m_uiPickPosY0;

  plUInt16 m_uiPickPosX1;
  plUInt16 m_uiPickPosY1;

  plUInt8 m_uiWhatToDo; // 0 == select, 1 == add, 2 == remove
  plUInt32 m_uiActionIdentifier;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plViewMarqueePickingResultMsgToEditor : public PlasmaEditorEngineViewMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plViewMarqueePickingResultMsgToEditor, PlasmaEditorEngineViewMsg);

public:
  plDynamicArray<plUuid> m_ObjectGuids;
  plUInt8 m_uiWhatToDo; // 0 == select, 1 == add, 2 == remove
  plUInt32 m_uiActionIdentifier;
};


class PlasmaEditorEngineConnection;

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plViewHighlightMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plViewHighlightMsgToEngine, PlasmaEditorEngineDocumentMsg);

public:
  plUuid m_HighlightObject;
  // currently used for highlighting which object the mouse hovers over
  // extend this message if other types of highlighting become necessary
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plLogMsgToEditor : public PlasmaEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLogMsgToEditor, PlasmaEditorEngineMsg);

public:
  plLogEntry m_Entry;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plCVarMsgToEditor : public PlasmaEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCVarMsgToEditor, PlasmaEditorEngineMsg);

public:
  plString m_sName;
  plString m_sPlugin;
  plString m_sDescription;
  plVariant m_Value;
};


class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plLongOpReplicationMsg : public PlasmaEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLongOpReplicationMsg, PlasmaEditorEngineMsg);

public:
  plUuid m_OperationGuid;
  plUuid m_DocumentGuid;
  plString m_sReplicationType;
  plDataBuffer m_ReplicationData;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plLongOpProgressMsg : public PlasmaEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLongOpProgressMsg, PlasmaEditorEngineMsg);

public:
  plUuid m_OperationGuid;
  float m_fCompletion;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plLongOpResultMsg : public PlasmaEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLongOpResultMsg, PlasmaEditorEngineMsg);

public:
  plUuid m_OperationGuid;
  bool m_bSuccess;
  plDataBuffer m_ResultData;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL PlasmaEditorEngineSyncObjectMsg : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(PlasmaEditorEngineSyncObjectMsg, PlasmaEditorEngineDocumentMsg);

public:
  plUuid m_ObjectGuid;
  plString m_sObjectType;
  plDataBuffer m_ObjectData;

  const plDataBuffer& GetObjectData() const { return m_ObjectData; }
  void SetObjectData(const plDataBuffer& s) { m_ObjectData = s; }
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plObjectTagMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plObjectTagMsgToEngine, PlasmaEditorEngineDocumentMsg);

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

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plObjectSelectionMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plObjectSelectionMsgToEngine, PlasmaEditorEngineDocumentMsg);

public:
  plString m_sSelection;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plSimulationSettingsMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSimulationSettingsMsgToEngine, PlasmaEditorEngineDocumentMsg);

public:
  bool m_bSimulateWorld = false;
  float m_fSimulationSpeed = 1.0f;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plGridSettingsMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGridSettingsMsgToEngine, PlasmaEditorEngineDocumentMsg);

public:
  float m_fGridDensity = 0.0f;
  plVec3 m_vGridCenter;
  plVec3 m_vGridTangent1;
  plVec3 m_vGridTangent2;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plGlobalSettingsMsgToEngine : public PlasmaEditorEngineMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGlobalSettingsMsgToEngine, PlasmaEditorEngineMsg);

public:
  float m_fGizmoScale = 0.0f;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plWorldSettingsMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plWorldSettingsMsgToEngine, PlasmaEditorEngineDocumentMsg);

public:
  bool m_bRenderOverlay = false;
  bool m_bRenderShapeIcons = false;
  bool m_bRenderSelectionBoxes = false;
  bool m_bAddAmbientLight = false;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plGameModeMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGameModeMsgToEngine, PlasmaEditorEngineDocumentMsg);

public:
  bool m_bEnablePTG = false;
  bool m_bUseStartPosition = false;
  plVec3 m_vStartPosition;
  plVec3 m_vStartDirection;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plGameModeMsgToEditor : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGameModeMsgToEditor, PlasmaEditorEngineDocumentMsg);

public:
  bool m_bRunningPTG;
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plQuerySelectionBBoxMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plQuerySelectionBBoxMsgToEngine, PlasmaEditorEngineDocumentMsg);

public:
  plUInt32 m_uiViewID; /// passed through to plQuerySelectionBBoxResultMsgToEditor
  plInt32 m_iPurpose;  /// passed through to plQuerySelectionBBoxResultMsgToEditor
};

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plQuerySelectionBBoxResultMsgToEditor : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plQuerySelectionBBoxResultMsgToEditor, PlasmaEditorEngineDocumentMsg);

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
class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plObjectsForDebugVisMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plObjectsForDebugVisMsgToEngine, PlasmaEditorEngineDocumentMsg);

public:
  plDataBuffer m_Objects;
};
