#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDragDropInfo, 1, plRTTIDefaultAllocator<plDragDropInfo>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plDragDropInfo::plDragDropInfo()
{
  m_vDropPosition.Set(plMath::NaN<float>());
  m_vDropNormal.Set(plMath::NaN<float>());
  m_iTargetObjectSubID = -1;
  m_iTargetObjectInsertChildIndex = -1;
  m_bShiftKeyDown = false;
  m_bCtrlKeyDown = false;
}


PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDragDropConfig, 1, plRTTIDefaultAllocator<plDragDropConfig>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plDragDropConfig::plDragDropConfig()
{
  m_bPickSelectedObjects = false;
}
