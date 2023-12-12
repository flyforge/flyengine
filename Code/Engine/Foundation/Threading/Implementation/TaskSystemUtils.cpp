#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Utilities/DGMLWriter.h>

const char* plWorkerThreadType::GetThreadTypeName(plWorkerThreadType::Enum threadType)
{
  switch (threadType)
  {
    case plWorkerThreadType::ShortTasks:
      return "Short Task";

    case plWorkerThreadType::LongTasks:
      return "Long Task";

    case plWorkerThreadType::FileAccess:
      return "File Access";

    default:
      PLASMA_REPORT_FAILURE("Invalid Thread Type");
      return "unknown";
  }
}

void plTaskSystem::WriteStateSnapshotToDGML(plDGMLGraph& ref_graph)
{
  PLASMA_LOCK(s_TaskSystemMutex);

  plHashTable<const plTaskGroup*, plDGMLGraph::NodeId> groupNodeIds;

  plStringBuilder title, tmp;

  plDGMLGraph::NodeDesc taskGroupND;
  taskGroupND.m_Color = plColor::CornflowerBlue;
  taskGroupND.m_Shape = plDGMLGraph::NodeShape::Rectangle;

  plDGMLGraph::NodeDesc taskNodeND;
  taskNodeND.m_Color = plColor::OrangeRed;
  taskNodeND.m_Shape = plDGMLGraph::NodeShape::RoundedRectangle;

  const plDGMLGraph::PropertyId startedByUserId = ref_graph.AddPropertyType("StartByUser");
  const plDGMLGraph::PropertyId activeDepsId = ref_graph.AddPropertyType("ActiveDependencies");
  const plDGMLGraph::PropertyId scheduledId = ref_graph.AddPropertyType("Scheduled");
  const plDGMLGraph::PropertyId finishedId = ref_graph.AddPropertyType("Finished");
  const plDGMLGraph::PropertyId multiplicityId = ref_graph.AddPropertyType("Multiplicity");
  const plDGMLGraph::PropertyId remainingRunsId = ref_graph.AddPropertyType("RemainingRuns");
  const plDGMLGraph::PropertyId priorityId = ref_graph.AddPropertyType("GroupPriority");

  const char* szTaskPriorityNames[plTaskPriority::ENUM_COUNT] = {};
  szTaskPriorityNames[plTaskPriority::EarlyThisFrame] = "EarlyThisFrame";
  szTaskPriorityNames[plTaskPriority::ThisFrame] = "ThisFrame";
  szTaskPriorityNames[plTaskPriority::LateThisFrame] = "LateThisFrame";
  szTaskPriorityNames[plTaskPriority::EarlyNextFrame] = "EarlyNextFrame";
  szTaskPriorityNames[plTaskPriority::NextFrame] = "NextFrame";
  szTaskPriorityNames[plTaskPriority::LateNextFrame] = "LateNextFrame";
  szTaskPriorityNames[plTaskPriority::In2Frames] = "In 2 Frames";
  szTaskPriorityNames[plTaskPriority::In3Frames] = "In 3 Frames";
  szTaskPriorityNames[plTaskPriority::In4Frames] = "In 4 Frames";
  szTaskPriorityNames[plTaskPriority::In5Frames] = "In 5 Frames";
  szTaskPriorityNames[plTaskPriority::In6Frames] = "In 6 Frames";
  szTaskPriorityNames[plTaskPriority::In7Frames] = "In 7 Frames";
  szTaskPriorityNames[plTaskPriority::In8Frames] = "In 8 Frames";
  szTaskPriorityNames[plTaskPriority::In9Frames] = "In 9 Frames";
  szTaskPriorityNames[plTaskPriority::LongRunningHighPriority] = "LongRunningHighPriority";
  szTaskPriorityNames[plTaskPriority::LongRunning] = "LongRunning";
  szTaskPriorityNames[plTaskPriority::FileAccessHighPriority] = "FileAccessHighPriority";
  szTaskPriorityNames[plTaskPriority::FileAccess] = "FileAccess";
  szTaskPriorityNames[plTaskPriority::ThisFrameMainThread] = "ThisFrameMainThread";
  szTaskPriorityNames[plTaskPriority::SomeFrameMainThread] = "SomeFrameMainThread";

  for (plUInt32 g = 0; g < s_pState->m_TaskGroups.GetCount(); ++g)
  {
    const plTaskGroup& tg = s_pState->m_TaskGroups[g];

    if (!tg.m_bInUse)
      continue;

    title.Format("Group {}", g);

    const plDGMLGraph::NodeId taskGroupId = ref_graph.AddGroup(title, plDGMLGraph::GroupType::Expanded, &taskGroupND);
    groupNodeIds[&tg] = taskGroupId;

    ref_graph.AddNodeProperty(taskGroupId, startedByUserId, tg.m_bStartedByUser ? "true" : "false");
    ref_graph.AddNodeProperty(taskGroupId, priorityId, szTaskPriorityNames[tg.m_Priority]);
    ref_graph.AddNodeProperty(taskGroupId, activeDepsId, plFmt("{}", tg.m_iNumActiveDependencies));

    for (plUInt32 t = 0; t < tg.m_Tasks.GetCount(); ++t)
    {
      const plTask& task = *tg.m_Tasks[t];
      const plDGMLGraph::NodeId taskNodeId = ref_graph.AddNode(task.m_sTaskName, &taskNodeND);

      ref_graph.AddNodeToGroup(taskNodeId, taskGroupId);

      ref_graph.AddNodeProperty(taskNodeId, scheduledId, task.m_bTaskIsScheduled ? "true" : "false");
      ref_graph.AddNodeProperty(taskNodeId, finishedId, task.IsTaskFinished() ? "true" : "false");

      tmp.Format("{}", task.GetMultiplicity());
      ref_graph.AddNodeProperty(taskNodeId, multiplicityId, tmp);

      tmp.Format("{}", task.m_iRemainingRuns);
      ref_graph.AddNodeProperty(taskNodeId, remainingRunsId, tmp);
    }
  }

  for (plUInt32 g = 0; g < s_pState->m_TaskGroups.GetCount(); ++g)
  {
    const plTaskGroup& tg = s_pState->m_TaskGroups[g];

    if (!tg.m_bInUse)
      continue;

    const plDGMLGraph::NodeId ownNodeId = groupNodeIds[&tg];

    for (const plTaskGroupID& dependsOn : tg.m_DependsOnGroups)
    {
      plDGMLGraph::NodeId otherNodeId;

      // filter out already fulfilled dependencies
      if (dependsOn.m_pTaskGroup->m_uiGroupCounter != dependsOn.m_uiGroupCounter)
        continue;

      // filter out already fulfilled dependencies
      if (!groupNodeIds.TryGetValue(dependsOn.m_pTaskGroup, otherNodeId))
        continue;

      PLASMA_ASSERT_DEBUG(otherNodeId != ownNodeId, "");

      ref_graph.AddConnection(otherNodeId, ownNodeId);
    }
  }
}

void plTaskSystem::WriteStateSnapshotToFile(const char* szPath /*= nullptr*/)
{
  plStringBuilder sPath = szPath;

  if (sPath.IsEmpty())
  {
    sPath = ":appdata/TaskGraphs/";

    const plDateTime dt = plTimestamp::CurrentTimestamp();

    sPath.AppendFormat("{0}-{1}-{2}_{3}-{4}-{5}-{6}", dt.GetYear(), plArgU(dt.GetMonth(), 2, true), plArgU(dt.GetDay(), 2, true), plArgU(dt.GetHour(), 2, true), plArgU(dt.GetMinute(), 2, true), plArgU(dt.GetSecond(), 2, true), plArgU(dt.GetMicroseconds() / 1000, 3, true));

    sPath.ChangeFileExtension("dgml");
  }

  plDGMLGraph graph;
  plTaskSystem::WriteStateSnapshotToDGML(graph);

  plDGMLGraphWriter::WriteGraphToFile(sPath, graph).IgnoreResult();

  plStringBuilder absPath;
  plFileSystem::ResolvePath(sPath, &absPath, nullptr).IgnoreResult();
  plLog::Info("Task graph snapshot saved to '{}'", absPath);
}


PLASMA_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskSystemUtils);
