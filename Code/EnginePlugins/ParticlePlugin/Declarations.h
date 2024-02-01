#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/RefCounted.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class plWorld;
class plParticleSystemDescriptor;
class plParticleEventReactionFactory;
class plParticleEventReaction;
class plParticleEmitter;
class plParticleInitializer;
class plParticleBehavior;
class plParticleType;
class plProcessingStreamGroup;
class plProcessingStream;
class plRandom;
struct plParticleEvent;
class plParticleEffectDescriptor;
class plParticleWorldModule;
class plParticleEffectInstance;
class plParticleSystemInstance;
struct plRenderViewContext;
class plRenderPipelinePass;
class plParticleFinalizer;
class plParticleFinalizerFactory;

using plParticleEffectResourceHandle = plTypedResourceHandle<class plParticleEffectResource>;

using plParticleEffectId = plGenericId<22, 10>;

/// \brief A handle to a particle effect
class PL_PARTICLEPLUGIN_DLL plParticleEffectHandle
{
  PL_DECLARE_HANDLE_TYPE(plParticleEffectHandle, plParticleEffectId);
};


struct PL_PARTICLEPLUGIN_DLL plParticleSystemState
{
  enum Enum
  {
    Active,
    EmittersFinished,
    OnlyReacting,
    Inactive,
  };
};

class PL_PARTICLEPLUGIN_DLL plParticleStreamBinding
{
public:
  void UpdateBindings(const plProcessingStreamGroup* pGroup) const;
  void Clear() { m_Bindings.Clear(); }

private:
  friend class plParticleSystemInstance;

  struct Binding
  {
    plString m_sName;
    plProcessingStream** m_ppStream;
  };

  plHybridArray<Binding, 4> m_Bindings;
};

//////////////////////////////////////////////////////////////////////////

struct PL_PARTICLEPLUGIN_DLL plParticleTypeRenderMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    Additive,
    Blended,
    Opaque,
    Distortion,
    BlendedBackground,
    BlendedForeground,
    BlendAdd,
    Default = Additive
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_PARTICLEPLUGIN_DLL, plParticleTypeRenderMode);

//////////////////////////////////////////////////////////////////////////

/// \brief What to do when an effect is not visible.
struct PL_PARTICLEPLUGIN_DLL plEffectInvisibleUpdateRate
{
  using StorageType = plUInt8;

  enum Enum
  {
    FullUpdate,
    Max20fps,
    Max10fps,
    Max5fps,
    Pause,
    Discard,

    Default = Max10fps
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_PARTICLEPLUGIN_DLL, plEffectInvisibleUpdateRate);

//////////////////////////////////////////////////////////////////////////

struct PL_PARTICLEPLUGIN_DLL plParticleTextureAtlasType
{
  using StorageType = plUInt8;

  enum Enum
  {
    None,

    RandomVariations,
    FlipbookAnimation,
    RandomYAnimatedX,

    Default = None
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_PARTICLEPLUGIN_DLL, plParticleTextureAtlasType);

//////////////////////////////////////////////////////////////////////////

struct PL_PARTICLEPLUGIN_DLL plParticleColorGradientMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    Age,
    Speed,

    Default = Age
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_PARTICLEPLUGIN_DLL, plParticleColorGradientMode);


//////////////////////////////////////////////////////////////////////////

struct PL_PARTICLEPLUGIN_DLL plParticleOutOfBoundsMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    Teleport,
    Die,

    Default = Teleport
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_PARTICLEPLUGIN_DLL, plParticleOutOfBoundsMode);

//////////////////////////////////////////////////////////////////////////

struct plParticleEffectFloatParam
{
  PL_DECLARE_POD_TYPE();
  plHashedString m_sName;
  float m_Value;
};

struct plParticleEffectColorParam
{
  PL_DECLARE_POD_TYPE();
  plHashedString m_sName;
  plColor m_Value;
};

class plParticleEffectParameters final : public plRefCounted
{
public:
  plHybridArray<plParticleEffectFloatParam, 2> m_FloatParams;
  plHybridArray<plParticleEffectColorParam, 2> m_ColorParams;
};
