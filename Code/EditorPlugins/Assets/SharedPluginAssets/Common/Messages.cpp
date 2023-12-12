#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <SharedPluginAssets/Common/Messages.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(PlasmaEditorEngineRestartSimulationMsg, 1, plRTTIDefaultAllocator<PlasmaEditorEngineRestartSimulationMsg>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(PlasmaEditorEngineLoopAnimationMsg, 1, plRTTIDefaultAllocator<PlasmaEditorEngineLoopAnimationMsg>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Loop", m_bLoop),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(PlasmaEditorEngineSetMaterialsMsg, 1, plRTTIDefaultAllocator<PlasmaEditorEngineSetMaterialsMsg>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY("Materials", m_Materials),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
