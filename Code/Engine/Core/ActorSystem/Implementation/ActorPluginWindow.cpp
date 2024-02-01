#include <Core/CorePCH.h>

#include <Core/ActorSystem/ActorPluginWindow.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plActorPluginWindow, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plActorPluginWindow::Update()
{
  if (GetWindow())
  {
    GetWindow()->ProcessWindowMessages();
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plActorPluginWindowOwner, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plActorPluginWindowOwner::~plActorPluginWindowOwner()
{
  // The window output target has a dependency to the window, e.g. the swapchain renders to it.
  // Explicitly destroy it first to ensure correct destruction order.
  m_pWindowOutputTarget.Clear();
  m_pWindow.Clear();
}

plWindowBase* plActorPluginWindowOwner::GetWindow() const
{
  return m_pWindow.Borrow();
}
plWindowOutputTargetBase* plActorPluginWindowOwner::GetOutputTarget() const
{
  return m_pWindowOutputTarget.Borrow();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plActorPluginWindowShared, 1, plRTTINoAllocator);
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plWindowBase* plActorPluginWindowShared::GetWindow() const
{
  return m_pWindow;
}

plWindowOutputTargetBase* plActorPluginWindowShared::GetOutputTarget() const
{
  return m_pWindowOutputTarget;
}


PL_STATICLINK_FILE(Core, Core_ActorSystem_Implementation_ActorPluginWindow);
