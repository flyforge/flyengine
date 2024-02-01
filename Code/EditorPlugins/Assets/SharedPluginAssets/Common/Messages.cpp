#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <SharedPluginAssets/Common/Messages.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditorEngineRestartSimulationMsg, 1, plRTTIDefaultAllocator<plEditorEngineRestartSimulationMsg>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditorEngineLoopAnimationMsg, 1, plRTTIDefaultAllocator<plEditorEngineLoopAnimationMsg>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Loop", m_bLoop),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditorEngineSetMaterialsMsg, 1, plRTTIDefaultAllocator<plEditorEngineSetMaterialsMsg>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ARRAY_MEMBER_PROPERTY("Materials", m_Materials),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
