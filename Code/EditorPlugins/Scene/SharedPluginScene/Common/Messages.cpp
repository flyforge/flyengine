#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <SharedPluginScene/Common/Messages.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plExposedSceneProperty, 1, plRTTIDefaultAllocator<plExposedSceneProperty>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Name", m_sName),
    PLASMA_MEMBER_PROPERTY("Object", m_Object)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("PropertyPath", m_sPropertyPath),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plExposedDocumentObjectPropertiesMsgToEngine, 1, plRTTIDefaultAllocator<plExposedDocumentObjectPropertiesMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY("Properties", m_Properties),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plExportSceneGeometryMsgToEngine, 1, plRTTIDefaultAllocator<plExportSceneGeometryMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Selection", m_bSelectionOnly),
    PLASMA_MEMBER_PROPERTY("File", m_sOutputFile),
    PLASMA_MEMBER_PROPERTY("Mode", m_iExtractionMode),
    PLASMA_MEMBER_PROPERTY("Transform", m_Transform),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plPullObjectStateMsgToEngine, 1, plRTTIDefaultAllocator<plPullObjectStateMsgToEngine>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plPushObjectStateData, plNoBase, 1, plRTTIDefaultAllocator<plPushObjectStateData>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("LayerGuid", m_LayerGuid),
    PLASMA_MEMBER_PROPERTY("Guid", m_ObjectGuid),
    PLASMA_MEMBER_PROPERTY("Pos", m_vPosition),
    PLASMA_MEMBER_PROPERTY("Rot", m_qRotation),
    PLASMA_MAP_MEMBER_PROPERTY("Bones", m_BoneTransforms),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plPushObjectStateMsgToEditor, 1, plRTTIDefaultAllocator<plPushObjectStateMsgToEditor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY("States", m_ObjectStates)
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plActiveLayerChangedMsgToEngine, 1, plRTTIDefaultAllocator<plActiveLayerChangedMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ActiveLayer", m_ActiveLayer),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLayerVisibilityChangedMsgToEngine, 1, plRTTIDefaultAllocator<plLayerVisibilityChangedMsgToEngine>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY("HiddenLayers", m_HiddenLayers),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
