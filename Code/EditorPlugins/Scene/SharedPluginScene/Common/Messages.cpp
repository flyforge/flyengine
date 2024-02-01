#include <SharedPluginScene/SharedPluginScenePCH.h>

#include <SharedPluginScene/Common/Messages.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plExposedSceneProperty, 1, plRTTIDefaultAllocator<plExposedSceneProperty>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Name", m_sName),
    PL_MEMBER_PROPERTY("Object", m_Object)->AddAttributes(new plHiddenAttribute()),
    PL_MEMBER_PROPERTY("PropertyPath", m_sPropertyPath),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plExposedDocumentObjectPropertiesMsgToEngine, 1, plRTTIDefaultAllocator<plExposedDocumentObjectPropertiesMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ARRAY_MEMBER_PROPERTY("Properties", m_Properties),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plExportSceneGeometryMsgToEngine, 1, plRTTIDefaultAllocator<plExportSceneGeometryMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Selection", m_bSelectionOnly),
    PL_MEMBER_PROPERTY("File", m_sOutputFile),
    PL_MEMBER_PROPERTY("Mode", m_iExtractionMode),
    PL_MEMBER_PROPERTY("Transform", m_Transform),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plPullObjectStateMsgToEngine, 1, plRTTIDefaultAllocator<plPullObjectStateMsgToEngine>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plPushObjectStateData, plNoBase, 1, plRTTIDefaultAllocator<plPushObjectStateData>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("LayerGuid", m_LayerGuid),
    PL_MEMBER_PROPERTY("Guid", m_ObjectGuid),
    PL_MEMBER_PROPERTY("Pos", m_vPosition),
    PL_MEMBER_PROPERTY("Rot", m_qRotation),
    PL_MAP_MEMBER_PROPERTY("Bones", m_BoneTransforms),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plPushObjectStateMsgToEditor, 1, plRTTIDefaultAllocator<plPushObjectStateMsgToEditor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ARRAY_MEMBER_PROPERTY("States", m_ObjectStates)
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plActiveLayerChangedMsgToEngine, 1, plRTTIDefaultAllocator<plActiveLayerChangedMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ActiveLayer", m_ActiveLayer),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLayerVisibilityChangedMsgToEngine, 1, plRTTIDefaultAllocator<plLayerVisibilityChangedMsgToEngine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ARRAY_MEMBER_PROPERTY("HiddenLayers", m_HiddenLayers),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
