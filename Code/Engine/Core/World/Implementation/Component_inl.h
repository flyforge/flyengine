#include <Foundation/Logging/Log.h>

PLASMA_ALWAYS_INLINE plComponent::plComponent() = default;

PLASMA_ALWAYS_INLINE plComponent::~plComponent()
{
  m_pMessageDispatchType = nullptr;
  m_pManager = nullptr;
  m_pOwner = nullptr;
  m_InternalId.Invalidate();
}

PLASMA_ALWAYS_INLINE bool plComponent::IsDynamic() const
{
  return m_ComponentFlags.IsSet(plObjectFlags::Dynamic);
}

PLASMA_ALWAYS_INLINE bool plComponent::GetActiveFlag() const
{
  return m_ComponentFlags.IsSet(plObjectFlags::ActiveFlag);
}

PLASMA_ALWAYS_INLINE bool plComponent::IsActive() const
{
  return m_ComponentFlags.IsSet(plObjectFlags::ActiveState);
}

PLASMA_ALWAYS_INLINE bool plComponent::IsActiveAndInitialized() const
{
  return m_ComponentFlags.AreAllSet(plObjectFlags::ActiveState | plObjectFlags::Initialized);
}

PLASMA_ALWAYS_INLINE plComponentManagerBase* plComponent::GetOwningManager()
{
  return m_pManager;
}

PLASMA_ALWAYS_INLINE const plComponentManagerBase* plComponent::GetOwningManager() const
{
  return m_pManager;
}

PLASMA_ALWAYS_INLINE plGameObject* plComponent::GetOwner()
{
  return m_pOwner;
}

PLASMA_ALWAYS_INLINE const plGameObject* plComponent::GetOwner() const
{
  return m_pOwner;
}

PLASMA_ALWAYS_INLINE plComponentHandle plComponent::GetHandle() const
{
  return plComponentHandle(m_InternalId);
}

PLASMA_ALWAYS_INLINE plUInt32 plComponent::GetUniqueID() const
{
  return m_uiUniqueID;
}

PLASMA_ALWAYS_INLINE void plComponent::SetUniqueID(plUInt32 uiUniqueID)
{
  m_uiUniqueID = uiUniqueID;
}

PLASMA_ALWAYS_INLINE bool plComponent::IsInitialized() const
{
  return m_ComponentFlags.IsSet(plObjectFlags::Initialized);
}

PLASMA_ALWAYS_INLINE bool plComponent::IsInitializing() const
{
  return m_ComponentFlags.IsSet(plObjectFlags::Initializing);
}

PLASMA_ALWAYS_INLINE bool plComponent::IsSimulationStarted() const
{
  return m_ComponentFlags.IsSet(plObjectFlags::SimulationStarted);
}

PLASMA_ALWAYS_INLINE bool plComponent::IsActiveAndSimulating() const
{
  return m_ComponentFlags.AreAllSet(plObjectFlags::Initialized | plObjectFlags::ActiveState) &&
         m_ComponentFlags.IsAnySet(plObjectFlags::SimulationStarting | plObjectFlags::SimulationStarted);
}
