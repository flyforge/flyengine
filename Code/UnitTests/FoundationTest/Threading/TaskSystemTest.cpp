#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Utilities/DGMLWriter.h>

class plTestTask final : public plTask
{
public:
  plUInt32 m_uiIterations;
  plTestTask* m_pDependency;
  bool m_bSupportCancel;
  plInt32 m_iTaskID;

  plTestTask()
  {
    m_uiIterations = 50;
    m_pDependency = nullptr;
    m_bStarted = false;
    m_bDone = false;
    m_bSupportCancel = false;
    m_iTaskID = -1;

    ConfigureTask("plTestTask", plTaskNesting::Never);
  }

  bool IsStarted() const { return m_bStarted; }
  bool IsDone() const { return m_bDone; }
  bool IsMultiplicityDone() const { return m_iMultiplicityCount == (int)GetMultiplicity(); }

private:
  bool m_bStarted;
  bool m_bDone;
  mutable plAtomicInteger32 m_iMultiplicityCount;

  virtual void ExecuteWithMultiplicity(plUInt32 uiInvocation) const override { m_iMultiplicityCount.Increment(); }

  virtual void Execute() override
  {
    if (m_iTaskID >= 0)
      plLog::Printf("Starting Task %i at %.4f\n", m_iTaskID, plTime::Now().GetSeconds());

    m_bStarted = true;

    PLASMA_TEST_BOOL(m_pDependency == nullptr || m_pDependency->IsTaskFinished());

    for (plUInt32 obst = 0; obst < m_uiIterations; ++obst)
    {
      plThreadUtils::Sleep(plTime::Milliseconds(1));
      plTime::Now();

      if (HasBeenCanceled() && m_bSupportCancel)
      {
        if (m_iTaskID >= 0)
          plLog::Printf("Canceling Task %i at %.4f\n", m_iTaskID, plTime::Now().GetSeconds());
        return;
      }
    }

    m_bDone = true;

    if (m_iTaskID >= 0)
      plLog::Printf("Finishing Task %i at %.4f\n", m_iTaskID, plTime::Now().GetSeconds());
  }
};

class TaskCallbacks
{
public:
  void TaskFinished(const plSharedPtr<plTask>& pTask) { m_pInt->Increment(); }

  void TaskGroupFinished(plTaskGroupID id) { m_pInt->Increment(); }

  plAtomicInteger32* m_pInt;
};

PLASMA_CREATE_SIMPLE_TEST(Threading, TaskSystem)
{
  plInt8 iWorkersShort = 4;
  plInt8 iWorkersLong = 4;

  plTaskSystem::SetWorkerThreadCount(iWorkersShort, iWorkersLong);
  plThreadUtils::Sleep(plTime::Milliseconds(500));

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Single Tasks")
  {
    plSharedPtr<plTestTask> t[3];

    t[0] = PLASMA_DEFAULT_NEW(plTestTask);
    t[1] = PLASMA_DEFAULT_NEW(plTestTask);
    t[2] = PLASMA_DEFAULT_NEW(plTestTask);

    t[0]->ConfigureTask("Task 0", plTaskNesting::Never);
    t[1]->ConfigureTask("Task 1", plTaskNesting::Maybe);
    t[2]->ConfigureTask("Task 2", plTaskNesting::Never);

    auto tg0 = plTaskSystem::StartSingleTask(t[0], plTaskPriority::LateThisFrame);
    auto tg1 = plTaskSystem::StartSingleTask(t[1], plTaskPriority::ThisFrame);
    auto tg2 = plTaskSystem::StartSingleTask(t[2], plTaskPriority::EarlyThisFrame);

    plTaskSystem::WaitForGroup(tg0);
    plTaskSystem::WaitForGroup(tg1);
    plTaskSystem::WaitForGroup(tg2);

    PLASMA_TEST_BOOL(t[0]->IsDone());
    PLASMA_TEST_BOOL(t[1]->IsDone());
    PLASMA_TEST_BOOL(t[2]->IsDone());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Single Tasks with Dependencies")
  {
    plSharedPtr<plTestTask> t[4];

    t[0] = PLASMA_DEFAULT_NEW(plTestTask);
    t[1] = PLASMA_DEFAULT_NEW(plTestTask);
    t[2] = PLASMA_DEFAULT_NEW(plTestTask);
    t[3] = PLASMA_DEFAULT_NEW(plTestTask);

    plTaskGroupID g[4];

    t[0]->ConfigureTask("Task 0", plTaskNesting::Never);
    t[1]->ConfigureTask("Task 1", plTaskNesting::Maybe);
    t[2]->ConfigureTask("Task 2", plTaskNesting::Never);
    t[3]->ConfigureTask("Task 3", plTaskNesting::Maybe);

    g[0] = plTaskSystem::StartSingleTask(t[0], plTaskPriority::LateThisFrame);
    g[1] = plTaskSystem::StartSingleTask(t[1], plTaskPriority::ThisFrame, g[0]);
    g[2] = plTaskSystem::StartSingleTask(t[2], plTaskPriority::EarlyThisFrame, g[1]);
    g[3] = plTaskSystem::StartSingleTask(t[3], plTaskPriority::EarlyThisFrame, g[0]);

    plTaskSystem::WaitForGroup(g[2]);
    plTaskSystem::WaitForGroup(g[3]);

    PLASMA_TEST_BOOL(t[0]->IsDone());
    PLASMA_TEST_BOOL(t[1]->IsDone());
    PLASMA_TEST_BOOL(t[2]->IsDone());
    PLASMA_TEST_BOOL(t[3]->IsDone());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Grouped Tasks / TaskFinished Callback / GroupFinished Callback")
  {
    plSharedPtr<plTestTask> t[8];

    plTaskGroupID g[4];
    plAtomicInteger32 GroupsFinished;
    plAtomicInteger32 TasksFinished;

    TaskCallbacks callbackGroup;
    callbackGroup.m_pInt = &GroupsFinished;

    TaskCallbacks callbackTask;
    callbackTask.m_pInt = &TasksFinished;

    g[0] = plTaskSystem::CreateTaskGroup(plTaskPriority::ThisFrame, plMakeDelegate(&TaskCallbacks::TaskGroupFinished, &callbackGroup));
    g[1] = plTaskSystem::CreateTaskGroup(plTaskPriority::ThisFrame, plMakeDelegate(&TaskCallbacks::TaskGroupFinished, &callbackGroup));
    g[2] = plTaskSystem::CreateTaskGroup(plTaskPriority::ThisFrame, plMakeDelegate(&TaskCallbacks::TaskGroupFinished, &callbackGroup));
    g[3] = plTaskSystem::CreateTaskGroup(plTaskPriority::ThisFrame, plMakeDelegate(&TaskCallbacks::TaskGroupFinished, &callbackGroup));

    for (int i = 0; i < 4; ++i)
      PLASMA_TEST_BOOL(!plTaskSystem::IsTaskGroupFinished(g[i]));

    plTaskSystem::AddTaskGroupDependency(g[1], g[0]);
    plTaskSystem::AddTaskGroupDependency(g[2], g[0]);
    plTaskSystem::AddTaskGroupDependency(g[3], g[1]);

    for (int i = 0; i < 8; ++i)
    {
      t[i] = PLASMA_DEFAULT_NEW(plTestTask);
      t[i]->ConfigureTask("Test Task", plTaskNesting::Maybe, plMakeDelegate(&TaskCallbacks::TaskFinished, &callbackTask));
    }

    plTaskSystem::AddTaskToGroup(g[0], t[0]);
    plTaskSystem::AddTaskToGroup(g[1], t[1]);
    plTaskSystem::AddTaskToGroup(g[1], t[2]);
    plTaskSystem::AddTaskToGroup(g[2], t[3]);
    plTaskSystem::AddTaskToGroup(g[2], t[4]);
    plTaskSystem::AddTaskToGroup(g[2], t[5]);
    plTaskSystem::AddTaskToGroup(g[3], t[6]);
    plTaskSystem::AddTaskToGroup(g[3], t[7]);

    for (int i = 0; i < 8; ++i)
    {
      PLASMA_TEST_BOOL(!t[i]->IsTaskFinished());
      PLASMA_TEST_BOOL(!t[i]->IsDone());
    }

    // do a snapshot
    // we don't validate it, just make sure it doesn't crash
    plDGMLGraph graph;
    plTaskSystem::WriteStateSnapshotToDGML(graph);

    plTaskSystem::StartTaskGroup(g[3]);
    plTaskSystem::StartTaskGroup(g[2]);
    plTaskSystem::StartTaskGroup(g[1]);
    plTaskSystem::StartTaskGroup(g[0]);

    plTaskSystem::WaitForGroup(g[3]);
    plTaskSystem::WaitForGroup(g[2]);
    plTaskSystem::WaitForGroup(g[1]);
    plTaskSystem::WaitForGroup(g[0]);

    PLASMA_TEST_INT(TasksFinished, 8);

    // It is not guaranteed that group finished callback is called after WaitForGroup returned so we need to wait a bit here.
    for (int i = 0; i < 10; i++)
    {
      if (GroupsFinished == 4)
      {
        break;
      }
      plThreadUtils::Sleep(plTime::Milliseconds(10));
    }
    PLASMA_TEST_INT(GroupsFinished, 4);

    for (int i = 0; i < 4; ++i)
      PLASMA_TEST_BOOL(plTaskSystem::IsTaskGroupFinished(g[i]));

    for (int i = 0; i < 8; ++i)
    {
      PLASMA_TEST_BOOL(t[i]->IsTaskFinished());
      PLASMA_TEST_BOOL(t[i]->IsDone());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "This Frame Tasks / Next Frame Tasks")
  {
    const plUInt32 uiNumTasks = 20;
    plSharedPtr<plTestTask> t[uiNumTasks];
    plTaskGroupID tg[uiNumTasks];
    bool finished[uiNumTasks];

    for (plUInt32 i = 0; i < uiNumTasks; i += 2)
    {
      finished[i] = false;
      finished[i + 1] = false;

      t[i] = PLASMA_DEFAULT_NEW(plTestTask);
      t[i + 1] = PLASMA_DEFAULT_NEW(plTestTask);

      t[i]->m_uiIterations = 10;
      t[i + 1]->m_uiIterations = 20;

      tg[i] = plTaskSystem::StartSingleTask(t[i], plTaskPriority::ThisFrame);
      tg[i + 1] = plTaskSystem::StartSingleTask(t[i + 1], plTaskPriority::NextFrame);
    }

    // 'finish' the first frame
    plTaskSystem::FinishFrameTasks();

    {
      plUInt32 uiNotAllThisTasksFinished = 0;
      plUInt32 uiNotAllNextTasksFinished = 0;

      for (plUInt32 i = 0; i < uiNumTasks; i += 2)
      {
        if (!t[i]->IsTaskFinished())
        {
          PLASMA_TEST_BOOL(!finished[i]);
          ++uiNotAllThisTasksFinished;
        }
        else
        {
          finished[i] = true;
        }

        if (!t[i + 1]->IsTaskFinished())
        {
          PLASMA_TEST_BOOL(!finished[i + 1]);
          ++uiNotAllNextTasksFinished;
        }
        else
        {
          finished[i + 1] = true;
        }
      }

      // up to the number of worker threads tasks can still be active
      PLASMA_TEST_BOOL(uiNotAllThisTasksFinished <= plTaskSystem::GetNumAllocatedWorkerThreads(plWorkerThreadType::ShortTasks));
      PLASMA_TEST_BOOL(uiNotAllNextTasksFinished <= uiNumTasks);
    }


    // 'finish' the second frame
    plTaskSystem::FinishFrameTasks();

    {
      plUInt32 uiNotAllThisTasksFinished = 0;
      plUInt32 uiNotAllNextTasksFinished = 0;

      for (int i = 0; i < uiNumTasks; i += 2)
      {
        if (!t[i]->IsTaskFinished())
        {
          PLASMA_TEST_BOOL(!finished[i]);
          ++uiNotAllThisTasksFinished;
        }
        else
        {
          finished[i] = true;
        }

        if (!t[i + 1]->IsTaskFinished())
        {
          PLASMA_TEST_BOOL(!finished[i + 1]);
          ++uiNotAllNextTasksFinished;
        }
        else
        {
          finished[i + 1] = true;
        }
      }

      PLASMA_TEST_BOOL(
        uiNotAllThisTasksFinished + uiNotAllNextTasksFinished <= plTaskSystem::GetNumAllocatedWorkerThreads(plWorkerThreadType::ShortTasks));
    }

    // 'finish' all frames
    plTaskSystem::FinishFrameTasks();

    {
      plUInt32 uiNotAllThisTasksFinished = 0;
      plUInt32 uiNotAllNextTasksFinished = 0;

      for (plUInt32 i = 0; i < uiNumTasks; i += 2)
      {
        if (!t[i]->IsTaskFinished())
        {
          PLASMA_TEST_BOOL(!finished[i]);
          ++uiNotAllThisTasksFinished;
        }
        else
        {
          finished[i] = true;
        }

        if (!t[i + 1]->IsTaskFinished())
        {
          PLASMA_TEST_BOOL(!finished[i + 1]);
          ++uiNotAllNextTasksFinished;
        }
        else
        {
          finished[i + 1] = true;
        }
      }

      // even after finishing multiple frames, the previous frame tasks may still be in execution
      // since no N+x tasks enforce their completion in this test
      PLASMA_TEST_BOOL(
        uiNotAllThisTasksFinished + uiNotAllNextTasksFinished <= plTaskSystem::GetNumAllocatedWorkerThreads(plWorkerThreadType::ShortTasks));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Main Thread Tasks")
  {
    const plUInt32 uiNumTasks = 20;
    plSharedPtr<plTestTask> t[uiNumTasks];

    for (plUInt32 i = 0; i < uiNumTasks; ++i)
    {
      t[i] = PLASMA_DEFAULT_NEW(plTestTask);
      t[i]->m_uiIterations = 10;

      plTaskSystem::StartSingleTask(t[i], plTaskPriority::ThisFrameMainThread);
    }

    plTaskSystem::FinishFrameTasks();

    for (plUInt32 i = 0; i < uiNumTasks; ++i)
    {
      PLASMA_TEST_BOOL(t[i]->IsTaskFinished());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Canceling Tasks")
  {
    const plUInt32 uiNumTasks = 20;
    plSharedPtr<plTestTask> t[uiNumTasks];
    plTaskGroupID tg[uiNumTasks];

    for (int i = 0; i < uiNumTasks; ++i)
    {
      t[i] = PLASMA_DEFAULT_NEW(plTestTask);
      t[i]->m_uiIterations = 50;

      tg[i] = plTaskSystem::StartSingleTask(t[i], plTaskPriority::ThisFrame);
    }

    plThreadUtils::Sleep(plTime::Milliseconds(1));

    plUInt32 uiCanceled = 0;

    for (plUInt32 i0 = uiNumTasks; i0 > 0; --i0)
    {
      const plUInt32 i = i0 - 1;

      if (plTaskSystem::CancelTask(t[i], plOnTaskRunning::ReturnWithoutBlocking) == PLASMA_SUCCESS)
        ++uiCanceled;
    }

    plUInt32 uiDone = 0;
    plUInt32 uiStarted = 0;

    for (int i = 0; i < uiNumTasks; ++i)
    {
      plTaskSystem::WaitForGroup(tg[i]);
      PLASMA_TEST_BOOL(t[i]->IsTaskFinished());

      if (t[i]->IsDone())
        ++uiDone;
      if (t[i]->IsStarted())
        ++uiStarted;
    }

    // at least one task should have run and thus be 'done'
    PLASMA_TEST_BOOL(uiDone > 0);
    PLASMA_TEST_BOOL(uiDone < uiNumTasks);

    PLASMA_TEST_BOOL(uiStarted > 0);
    PLASMA_TEST_BOOL_MSG(uiStarted <= plTaskSystem::GetNumAllocatedWorkerThreads(plWorkerThreadType::ShortTasks),
      "This test can fail when the PC is under heavy load."); // should not have managed to start more tasks than there are threads
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Canceling Tasks (forcefully)")
  {
    const plUInt32 uiNumTasks = 20;
    plSharedPtr<plTestTask> t[uiNumTasks];
    plTaskGroupID tg[uiNumTasks];

    for (int i = 0; i < uiNumTasks; ++i)
    {
      t[i] = PLASMA_DEFAULT_NEW(plTestTask);
      t[i]->m_uiIterations = 50;
      t[i]->m_bSupportCancel = true;

      tg[i] = plTaskSystem::StartSingleTask(t[i], plTaskPriority::ThisFrame);
    }

    plThreadUtils::Sleep(plTime::Milliseconds(1));

    plUInt32 uiCanceled = 0;

    for (int i = uiNumTasks - 1; i >= 0; --i)
    {
      if (plTaskSystem::CancelTask(t[i], plOnTaskRunning::ReturnWithoutBlocking) == PLASMA_SUCCESS)
        ++uiCanceled;
    }

    plUInt32 uiDone = 0;
    plUInt32 uiStarted = 0;

    for (int i = 0; i < uiNumTasks; ++i)
    {
      plTaskSystem::WaitForGroup(tg[i]);
      PLASMA_TEST_BOOL(t[i]->IsTaskFinished());

      if (t[i]->IsDone())
        ++uiDone;
      if (t[i]->IsStarted())
        ++uiStarted;
    }

    // not a single thread should have finished the execution
    if (PLASMA_TEST_BOOL_MSG(uiDone == 0, "This test can fail when the PC is under heavy load."))
    {
      PLASMA_TEST_BOOL(uiStarted > 0);
      PLASMA_TEST_BOOL(uiStarted <= plTaskSystem::GetNumAllocatedWorkerThreads(
                                  plWorkerThreadType::ShortTasks)); // should not have managed to start more tasks than there are threads
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Canceling Group")
  {
    const plUInt32 uiNumTasks = 4;
    plSharedPtr<plTestTask> t1[uiNumTasks];
    plSharedPtr<plTestTask> t2[uiNumTasks];

    plTaskGroupID g1, g2;
    g1 = plTaskSystem::CreateTaskGroup(plTaskPriority::ThisFrame);
    g2 = plTaskSystem::CreateTaskGroup(plTaskPriority::ThisFrame);

    plTaskSystem::AddTaskGroupDependency(g2, g1);

    for (plUInt32 i = 0; i < uiNumTasks; ++i)
    {
      t1[i] = PLASMA_DEFAULT_NEW(plTestTask);
      t2[i] = PLASMA_DEFAULT_NEW(plTestTask);

      plTaskSystem::AddTaskToGroup(g1, t1[i]);
      plTaskSystem::AddTaskToGroup(g2, t2[i]);
    }

    plTaskSystem::StartTaskGroup(g2);
    plTaskSystem::StartTaskGroup(g1);

    plThreadUtils::Sleep(plTime::Milliseconds(10));

    PLASMA_TEST_BOOL(plTaskSystem::CancelGroup(g2, plOnTaskRunning::WaitTillFinished) == PLASMA_SUCCESS);

    for (int i = 0; i < uiNumTasks; ++i)
    {
      PLASMA_TEST_BOOL(!t2[i]->IsDone());
      PLASMA_TEST_BOOL(t2[i]->IsTaskFinished());
    }

    plThreadUtils::Sleep(plTime::Milliseconds(1));

    PLASMA_TEST_BOOL(plTaskSystem::CancelGroup(g1, plOnTaskRunning::WaitTillFinished) == PLASMA_FAILURE);

    for (int i = 0; i < uiNumTasks; ++i)
    {
      PLASMA_TEST_BOOL(!t2[i]->IsDone());

      PLASMA_TEST_BOOL(t1[i]->IsTaskFinished());
      PLASMA_TEST_BOOL(t2[i]->IsTaskFinished());
    }

    plThreadUtils::Sleep(plTime::Milliseconds(100));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Tasks with Multiplicity")
  {
    plSharedPtr<plTestTask> t[3];
    plTaskGroupID tg[3];

    t[0] = PLASMA_DEFAULT_NEW(plTestTask);
    t[1] = PLASMA_DEFAULT_NEW(plTestTask);
    t[2] = PLASMA_DEFAULT_NEW(plTestTask);

    t[0]->ConfigureTask("Task 0", plTaskNesting::Maybe);
    t[1]->ConfigureTask("Task 1", plTaskNesting::Maybe);
    t[2]->ConfigureTask("Task 2", plTaskNesting::Never);

    t[0]->SetMultiplicity(1);
    t[1]->SetMultiplicity(100);
    t[2]->SetMultiplicity(1000);

    tg[0] = plTaskSystem::StartSingleTask(t[0], plTaskPriority::LateThisFrame);
    tg[1] = plTaskSystem::StartSingleTask(t[1], plTaskPriority::ThisFrame);
    tg[2] = plTaskSystem::StartSingleTask(t[2], plTaskPriority::EarlyThisFrame);

    plTaskSystem::WaitForGroup(tg[0]);
    plTaskSystem::WaitForGroup(tg[1]);
    plTaskSystem::WaitForGroup(tg[2]);

    PLASMA_TEST_BOOL(t[0]->IsMultiplicityDone());
    PLASMA_TEST_BOOL(t[1]->IsMultiplicityDone());
    PLASMA_TEST_BOOL(t[2]->IsMultiplicityDone());
  }

  // capture profiling info for testing
  /*plStringBuilder sOutputPath = plTestFramework::GetInstance()->GetAbsOutputPath();

  plFileSystem::AddDataDirectory(sOutputPath.GetData());

  plFileWriter fileWriter;
  if (fileWriter.Open("profiling.json") == PLASMA_SUCCESS)
  {
  plProfilingSystem::Capture(fileWriter);
  }*/
}
