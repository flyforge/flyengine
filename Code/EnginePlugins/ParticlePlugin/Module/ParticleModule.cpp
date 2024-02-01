#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Module/ParticleModule.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleModule, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Module_ParticleModule);

