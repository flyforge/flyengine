#include <Core/CorePCH.h>

#include <Core/ActorSystem/ActorApiService.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plActorApiService, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plActorApiService::plActorApiService() = default;
plActorApiService::~plActorApiService() = default;



PLASMA_STATICLINK_FILE(Core, Core_ActorSystem_Implementation_ActorApiService);
