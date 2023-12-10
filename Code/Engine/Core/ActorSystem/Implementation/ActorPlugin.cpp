#include <Core/CorePCH.h>

#include <Core/ActorSystem/ActorPlugin.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plActorPlugin, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plActorPlugin::plActorPlugin() = default;
plActorPlugin::~plActorPlugin() = default;

plActor* plActorPlugin::GetActor() const
{
  return m_pOwningActor;
}


PLASMA_STATICLINK_FILE(Core, Core_ActorSystem_Implementation_ActorPlugin);
