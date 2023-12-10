#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <SharedPluginAssets/Common/Messages.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditorEngineRestartSimulationMsg, 1, plRTTIDefaultAllocator<plEditorEngineRestartSimulationMsg>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditorEngineLoopAnimationMsg, 1, plRTTIDefaultAllocator<plEditorEngineLoopAnimationMsg>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Loop", m_bLoop),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditorEngineSetMaterialsMsg, 1, plRTTIDefaultAllocator<plEditorEngineSetMaterialsMsg>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY("Materials", m_Materials),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
