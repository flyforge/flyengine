#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <SharedPluginScene/SharedPluginSceneDLL.h>

class PL_SHAREDPLUGINSCENE_DLL plExposedSceneProperty : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plExposedSceneProperty, plReflectedClass);

public:
  plString m_sName;
  plUuid m_Object;
  plString m_sPropertyPath;
};

class PL_SHAREDPLUGINSCENE_DLL plExposedDocumentObjectPropertiesMsgToEngine : public plEditorEngineDocumentMsg
{
  PL_ADD_DYNAMIC_REFLECTION(plExposedDocumentObjectPropertiesMsgToEngine, plEditorEngineDocumentMsg);

public:
  plDynamicArray<plExposedSceneProperty> m_Properties;
};

class PL_SHAREDPLUGINSCENE_DLL plExportSceneGeometryMsgToEngine : public plEditorEngineDocumentMsg
{
  PL_ADD_DYNAMIC_REFLECTION(plExportSceneGeometryMsgToEngine, plEditorEngineDocumentMsg);

public:
  bool m_bSelectionOnly = false;
  plString m_sOutputFile;
  int m_iExtractionMode; // plWorldGeoExtractionUtil::ExtractionMode
  plMat3 m_Transform;
};

class PL_SHAREDPLUGINSCENE_DLL plPullObjectStateMsgToEngine : public plEditorEngineDocumentMsg
{
  PL_ADD_DYNAMIC_REFLECTION(plPullObjectStateMsgToEngine, plEditorEngineDocumentMsg);
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

PL_DECLARE_REFLECTABLE_TYPE(PL_SHAREDPLUGINSCENE_DLL, plPushObjectStateData);

class PL_SHAREDPLUGINSCENE_DLL plPushObjectStateMsgToEditor : public plEditorEngineDocumentMsg
{
  PL_ADD_DYNAMIC_REFLECTION(plPushObjectStateMsgToEditor, plEditorEngineDocumentMsg);

public:
  plDynamicArray<plPushObjectStateData> m_ObjectStates;
};

class PL_SHAREDPLUGINSCENE_DLL plActiveLayerChangedMsgToEngine : public plEditorEngineDocumentMsg
{
  PL_ADD_DYNAMIC_REFLECTION(plActiveLayerChangedMsgToEngine, plEditorEngineDocumentMsg);

public:
  plUuid m_ActiveLayer;
};

class PL_SHAREDPLUGINSCENE_DLL plLayerVisibilityChangedMsgToEngine : public plEditorEngineDocumentMsg
{
  PL_ADD_DYNAMIC_REFLECTION(plLayerVisibilityChangedMsgToEngine, plEditorEngineDocumentMsg);

public:
  plHybridArray<plUuid, 1> m_HiddenLayers;
};
