#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <SharedPluginScene/SharedPluginSceneDLL.h>

class PLASMA_SHAREDPLUGINSCENE_DLL plExposedSceneProperty : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plExposedSceneProperty, plReflectedClass);

public:
  plString m_sName;
  plUuid m_Object;
  plString m_sPropertyPath;
};

class PLASMA_SHAREDPLUGINSCENE_DLL plExposedDocumentObjectPropertiesMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plExposedDocumentObjectPropertiesMsgToEngine, PlasmaEditorEngineDocumentMsg);

public:
  plDynamicArray<plExposedSceneProperty> m_Properties;
};

class PLASMA_SHAREDPLUGINSCENE_DLL plExportSceneGeometryMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plExportSceneGeometryMsgToEngine, PlasmaEditorEngineDocumentMsg);

public:
  bool m_bSelectionOnly = false;
  plString m_sOutputFile;
  int m_iExtractionMode; // plWorldGeoExtractionUtil::ExtractionMode
  plMat3 m_Transform;
};

class PLASMA_SHAREDPLUGINSCENE_DLL plPullObjectStateMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPullObjectStateMsgToEngine, PlasmaEditorEngineDocumentMsg);
};

struct plPushObjectStateData
{
  plUuid m_LayerGuid;
  plUuid m_ObjectGuid;
  plVec3 m_vPosition;
  plQuat m_qRotation;
  bool m_bAdjustFromPrefabRootChild = false; // only used internally, not synchronized
  plMap<plString, plTransform> m_BoneTransforms;
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_SHAREDPLUGINSCENE_DLL, plPushObjectStateData);

class PLASMA_SHAREDPLUGINSCENE_DLL plPushObjectStateMsgToEditor : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPushObjectStateMsgToEditor, PlasmaEditorEngineDocumentMsg);

public:
  plDynamicArray<plPushObjectStateData> m_ObjectStates;
};

class PLASMA_SHAREDPLUGINSCENE_DLL plActiveLayerChangedMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plActiveLayerChangedMsgToEngine, PlasmaEditorEngineDocumentMsg);

public:
  plUuid m_ActiveLayer;
};

class PLASMA_SHAREDPLUGINSCENE_DLL plLayerVisibilityChangedMsgToEngine : public PlasmaEditorEngineDocumentMsg
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLayerVisibilityChangedMsgToEngine, PlasmaEditorEngineDocumentMsg);

public:
  plHybridArray<plUuid, 1> m_HiddenLayers;
};
