#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDragDropInfo, 1, plRTTIDefaultAllocator<plDragDropInfo>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plDragDropInfo::plDragDropInfo()
{
  m_vDropPosition.Set(plMath::NaN<float>());
  m_vDropNormal.Set(plMath::NaN<float>());
  m_iTargetObjectSubID = -1;
  m_iTargetObjectInsertChildIndex = -1;
  m_bShiftKeyDown = false;
  m_bCtrlKeyDown = false;
}


PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDragDropConfig, 1, plRTTIDefaultAllocator<plDragDropConfig>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plDragDropConfig::plDragDropConfig()
{
  m_bPickSelectedObjects = false;
}
