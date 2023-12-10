#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/ActionManager.h>

const plActionDescriptor* plActionDescriptorHandle::GetDescriptor() const
{
  return plActionManager::GetActionDescriptor(*this);
}

plActionDescriptor::plActionDescriptor(plActionType::Enum type, plActionScope::Enum scope, const char* szName, const char* szCategoryPath,
  const char* szShortcut, CreateActionFunc createAction, DeleteActionFunc deleteAction)
  : m_Type(type)
  , m_Scope(scope)
  , m_sActionName(szName)
  , m_sCategoryPath(szCategoryPath)
  , m_sShortcut(szShortcut)
  , m_sDefaultShortcut(szShortcut)
  , m_CreateAction(createAction)
  , m_DeleteAction(deleteAction)
{
}

plAction* plActionDescriptor::CreateAction(const plActionContext& context) const
{
  PLASMA_ASSERT_DEV(!m_Handle.IsInvalidated(), "Handle invalid!");
  auto pAction = m_CreateAction(context);
  pAction->m_hDescriptorHandle = m_Handle;

  m_CreatedActions.PushBack(pAction);
  return pAction;
}

void plActionDescriptor::DeleteAction(plAction* pAction) const
{
  m_CreatedActions.RemoveAndSwap(pAction);

  if (m_DeleteAction == nullptr)
  {
    PLASMA_DEFAULT_DELETE(pAction);
  }
  else
    m_DeleteAction(pAction);
}


void plActionDescriptor::UpdateExistingActions()
{
  for (auto pAction : m_CreatedActions)
  {
    pAction->TriggerUpdate();
  }
}

void plAction::TriggerUpdate()
{
  m_StatusUpdateEvent.Broadcast(this);
}

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
