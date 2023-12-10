#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

// clang-format off

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plParticleTypeRenderMode, 1)
  PLASMA_ENUM_CONSTANT(plParticleTypeRenderMode::Opaque),
  PLASMA_ENUM_CONSTANT(plParticleTypeRenderMode::Additive),
  PLASMA_ENUM_CONSTANT(plParticleTypeRenderMode::Blended),
  PLASMA_ENUM_CONSTANT(plParticleTypeRenderMode::BlendedForeground),
  PLASMA_ENUM_CONSTANT(plParticleTypeRenderMode::BlendedBackground),
  PLASMA_ENUM_CONSTANT(plParticleTypeRenderMode::Distortion),
  PLASMA_ENUM_CONSTANT(plParticleTypeRenderMode::BlendAdd),
PLASMA_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plEffectInvisibleUpdateRate, 1)
  PLASMA_ENUM_CONSTANT(plEffectInvisibleUpdateRate::FullUpdate),
  PLASMA_ENUM_CONSTANT(plEffectInvisibleUpdateRate::Max20fps),
  PLASMA_ENUM_CONSTANT(plEffectInvisibleUpdateRate::Max10fps),
  PLASMA_ENUM_CONSTANT(plEffectInvisibleUpdateRate::Max5fps),
  PLASMA_ENUM_CONSTANT(plEffectInvisibleUpdateRate::Pause),
  PLASMA_ENUM_CONSTANT(plEffectInvisibleUpdateRate::Discard),
PLASMA_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plParticleTextureAtlasType, 1)
  PLASMA_ENUM_CONSTANT(plParticleTextureAtlasType::None),
  PLASMA_ENUM_CONSTANT(plParticleTextureAtlasType::RandomVariations),
  PLASMA_ENUM_CONSTANT(plParticleTextureAtlasType::FlipbookAnimation),
  PLASMA_ENUM_CONSTANT(plParticleTextureAtlasType::RandomYAnimatedX),
PLASMA_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plParticleColorGradientMode, 1)
  PLASMA_ENUM_CONSTANT(plParticleColorGradientMode::Age),
  PLASMA_ENUM_CONSTANT(plParticleColorGradientMode::Speed),
PLASMA_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plParticleOutOfBoundsMode, 1)
  PLASMA_ENUM_CONSTANT(plParticleOutOfBoundsMode::Teleport),
  PLASMA_ENUM_CONSTANT(plParticleOutOfBoundsMode::Die),
PLASMA_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

// clang-format on

PLASMA_STATICLINK_LIBRARY(ParticlePlugin)
{
  if (bReturn)
    return;

  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_ColorGradient);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Gravity);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Raycast);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_SizeCurve);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Velocity);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Components_ParticleComponent);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Effect_ParticleEffectController);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Effect_ParticleEffectDescriptor);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Effect_ParticleEffectInstance);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_Continuous);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_OnEvent);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_BoxPosition);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_CylinderPosition);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_RandomColor);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_RandomRotationSpeed);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_RandomSize);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_SpherePosition);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_VelocityCone);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Renderer_ParticleExtractor);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Renderer_ParticleRenderer);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Resources_ParticleEffectResource);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Startup);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Streams_DefaultParticleStreams);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Streams_ParticleStream);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_System_ParticleSystemDescriptor);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_System_ParticleSystemInstance);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Type_Effect_ParticleTypeEffect);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Type_Light_ParticleTypeLight);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Type_ParticleType);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Type_Point_ParticleTypePoint);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Type_Point_PointRenderer);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Type_Trail_ParticleTypeTrail);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_Type_Trail_TrailRenderer);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_WorldModule_ParticleEffects);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_WorldModule_ParticleSystems);
  PLASMA_STATICLINK_REFERENCE(ParticlePlugin_WorldModule_ParticleWorldModule);
}
