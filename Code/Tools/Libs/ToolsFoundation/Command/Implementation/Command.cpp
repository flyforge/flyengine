#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Document.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCommand, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plCommand::plCommand() = default;
plCommand::~plCommand() = default;

bool plCommand::HasModifiedDocument() const
{
  if (m_bModifiedDocument)
    return true;

  for (const auto& ca : m_ChildActions)
  {
    if (ca->HasModifiedDocument())
      return true;
  }

  return false;
}

plStatus plCommand::Do(bool bRedo)
{
  plStatus status = DoInternal(bRedo);
  if (status.m_Result == PL_FAILURE)
  {
    if (bRedo)
    {
      // A command that originally succeeded failed on redo!
      return status;
    }
    else
    {
      for (plInt32 j = m_ChildActions.GetCount() - 1; j >= 0; --j)
      {
        plStatus status2 = m_ChildActions[j]->Undo(true);
        PL_ASSERT_DEV(status2.m_Result == PL_SUCCESS, "Failed do could not be recovered! Inconsistent state!");
      }
      return status;
    }
  }
  if (!bRedo)
    return plStatus(PL_SUCCESS);

  const plUInt32 uiChildActions = m_ChildActions.GetCount();
  for (plUInt32 i = 0; i < uiChildActions; ++i)
  {
    status = m_ChildActions[i]->Do(bRedo);
    if (status.m_Result == PL_FAILURE)
    {
      for (plInt32 j = i - 1; j >= 0; --j)
      {
        plStatus status2 = m_ChildActions[j]->Undo(true);
        PL_ASSERT_DEV(status2.m_Result == PL_SUCCESS, "Failed redo could not be recovered! Inconsistent state!");
      }
      // A command that originally succeeded failed on redo!
      return status;
    }
  }
  return plStatus(PL_SUCCESS);
}

plStatus plCommand::Undo(bool bFireEvents)
{
  const plUInt32 uiChildActions = m_ChildActions.GetCount();
  for (plInt32 i = uiChildActions - 1; i >= 0; --i)
  {
    plStatus status = m_ChildActions[i]->Undo(bFireEvents);
    if (status.m_Result == PL_FAILURE)
    {
      for (plUInt32 j = i + 1; j < uiChildActions; ++j)
      {
        plStatus status2 = m_ChildActions[j]->Do(true);
        PL_ASSERT_DEV(status2.m_Result == PL_SUCCESS, "Failed undo could not be recovered! Inconsistent state!");
      }
      // A command that originally succeeded failed on undo!
      return status;
    }
  }

  plStatus status = UndoInternal(bFireEvents);
  if (status.m_Result == PL_FAILURE)
  {
    for (plUInt32 j = 0; j < uiChildActions; ++j)
    {
      plStatus status2 = m_ChildActions[j]->Do(true);
      PL_ASSERT_DEV(status2.m_Result == PL_SUCCESS, "Failed undo could not be recovered! Inconsistent state!");
    }
    // A command that originally succeeded failed on undo!
    return status;
  }

  return plStatus(PL_SUCCESS);
}

void plCommand::Cleanup(CommandState state)
{
  CleanupInternal(state);

  for (plCommand* pCommand : m_ChildActions)
  {
    pCommand->Cleanup(state);
    pCommand->GetDynamicRTTI()->GetAllocator()->Deallocate(pCommand);
  }

  m_ChildActions.Clear();
}


plStatus plCommand::AddSubCommand(plCommand& command)
{
  plCommand* pCommand = plReflectionSerializer::Clone(&command);
  const plRTTI* pRtti = pCommand->GetDynamicRTTI();

  pCommand->m_pDocument = m_pDocument;

  m_ChildActions.PushBack(pCommand);
  m_pDocument->GetCommandHistory()->GetStorage()->m_ActiveCommandStack.PushBack(pCommand);
  plStatus ret = pCommand->Do(false);
  m_pDocument->GetCommandHistory()->GetStorage()->m_ActiveCommandStack.PopBack();

  if (ret.m_Result == PL_FAILURE)
  {
    m_ChildActions.PopBack();
    pCommand->GetDynamicRTTI()->GetAllocator()->Deallocate(pCommand);
    return ret;
  }

  if (pCommand->HasReturnValues())
  {
    // Write properties back so any return values get written.
    plDefaultMemoryStreamStorage storage;
    plMemoryStreamWriter writer(&storage);
    plMemoryStreamReader reader(&storage);

    plReflectionSerializer::WriteObjectToBinary(writer, pCommand->GetDynamicRTTI(), pCommand);
    plReflectionSerializer::ReadObjectPropertiesFromBinary(reader, *pRtti, &command);
  }

  return plStatus(PL_SUCCESS);
}
