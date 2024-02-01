#include <Core/CorePCH.h>

#include <Core/ActorSystem/ActorPlugin.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plActorPlugin, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plActorPlugin::plActorPlugin() = default;
plActorPlugin::~plActorPlugin() = default;

plActor* plActorPlugin::GetActor() const
{
  return m_pOwningActor;
}


PL_STATICLINK_FILE(Core, Core_ActorSystem_Implementation_ActorPlugin);
