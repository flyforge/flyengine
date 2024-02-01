#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

// clang-format off

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_STATIC_REFLECTED_ENUM(plParticleTypeRenderMode, 1)
  PL_ENUM_CONSTANT(plParticleTypeRenderMode::Opaque),
  PL_ENUM_CONSTANT(plParticleTypeRenderMode::Additive),
  PL_ENUM_CONSTANT(plParticleTypeRenderMode::Blended),
  PL_ENUM_CONSTANT(plParticleTypeRenderMode::BlendedForeground),
  PL_ENUM_CONSTANT(plParticleTypeRenderMode::BlendedBackground),
  PL_ENUM_CONSTANT(plParticleTypeRenderMode::Distortion),
  PL_ENUM_CONSTANT(plParticleTypeRenderMode::BlendAdd),
PL_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_STATIC_REFLECTED_ENUM(plEffectInvisibleUpdateRate, 1)
  PL_ENUM_CONSTANT(plEffectInvisibleUpdateRate::FullUpdate),
  PL_ENUM_CONSTANT(plEffectInvisibleUpdateRate::Max20fps),
  PL_ENUM_CONSTANT(plEffectInvisibleUpdateRate::Max10fps),
  PL_ENUM_CONSTANT(plEffectInvisibleUpdateRate::Max5fps),
  PL_ENUM_CONSTANT(plEffectInvisibleUpdateRate::Pause),
  PL_ENUM_CONSTANT(plEffectInvisibleUpdateRate::Discard),
PL_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_STATIC_REFLECTED_ENUM(plParticleTextureAtlasType, 1)
  PL_ENUM_CONSTANT(plParticleTextureAtlasType::None),
  PL_ENUM_CONSTANT(plParticleTextureAtlasType::RandomVariations),
  PL_ENUM_CONSTANT(plParticleTextureAtlasType::FlipbookAnimation),
  PL_ENUM_CONSTANT(plParticleTextureAtlasType::RandomYAnimatedX),
PL_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_STATIC_REFLECTED_ENUM(plParticleColorGradientMode, 1)
  PL_ENUM_CONSTANT(plParticleColorGradientMode::Age),
  PL_ENUM_CONSTANT(plParticleColorGradientMode::Speed),
PL_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_STATIC_REFLECTED_ENUM(plParticleOutOfBoundsMode, 1)
  PL_ENUM_CONSTANT(plParticleOutOfBoundsMode::Teleport),
  PL_ENUM_CONSTANT(plParticleOutOfBoundsMode::Die),
PL_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

// clang-format on

PL_STATICLINK_LIBRARY(ParticlePlugin)
{
  if (bReturn)
    return;

  PL_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Bounds);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_ColorGradient);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_FadeOut);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Flies);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Gravity);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_PullAlong);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Raycast);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_SizeCurve);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Velocity);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Components_ParticleComponent);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Components_ParticleFinisherComponent);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Effect_ParticleEffectController);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Effect_ParticleEffectDescriptor);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Effect_ParticleEffectInstance);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_Burst);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_Continuous);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_Distance);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_OnEvent);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Events_ParticleEventReaction);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Events_ParticleEventReaction_Effect);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Events_ParticleEventReaction_Prefab);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer_Age);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer_ApplyVelocity);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer_LastPosition);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer_Volume);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_BoxPosition);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_CylinderPosition);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_RandomColor);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_RandomRotationSpeed);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_RandomSize);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_SpherePosition);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_VelocityCone);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Module_ParticleModule);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Renderer_ParticleRenderer);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Resources_ParticleEffectResource);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Startup);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Streams_DefaultParticleStreams);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Streams_ParticleStream);
  PL_STATICLINK_REFERENCE(ParticlePlugin_System_ParticleSystemDescriptor);
  PL_STATICLINK_REFERENCE(ParticlePlugin_System_ParticleSystemInstance);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Type_Effect_ParticleTypeEffect);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Type_Light_ParticleTypeLight);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Type_Mesh_ParticleTypeMesh);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Type_ParticleType);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Type_Point_ParticleTypePoint);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Type_Point_PointRenderer);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Type_Quad_ParticleTypeQuad);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Type_Quad_QuadParticleRenderer);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Type_Trail_ParticleTypeTrail);
  PL_STATICLINK_REFERENCE(ParticlePlugin_Type_Trail_TrailRenderer);
  PL_STATICLINK_REFERENCE(ParticlePlugin_WorldModule_ParticleEffects);
  PL_STATICLINK_REFERENCE(ParticlePlugin_WorldModule_ParticleSystems);
  PL_STATICLINK_REFERENCE(ParticlePlugin_WorldModule_ParticleWorldModule);
}
