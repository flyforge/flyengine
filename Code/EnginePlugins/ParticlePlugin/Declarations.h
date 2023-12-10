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

typedef plGenericId<22, 10> plParticleEffectId;

/// \brief A handle to a particle effect
class PLASMA_PARTICLEPLUGIN_DLL plParticleEffectHandle
{
  PLASMA_DECLARE_HANDLE_TYPE(plParticleEffectHandle, plParticleEffectId);
};


struct PLASMA_PARTICLEPLUGIN_DLL plParticleSystemState
{
  enum Enum
  {
    Active,
    EmittersFinished,
    OnlyReacting,
    Inactive,
  };
};

class PLASMA_PARTICLEPLUGIN_DLL plParticleStreamBinding
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

struct PLASMA_PARTICLEPLUGIN_DLL plParticleTypeRenderMode
{
  typedef plUInt8 StorageType;

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

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_PARTICLEPLUGIN_DLL, plParticleTypeRenderMode);

//////////////////////////////////////////////////////////////////////////

/// \brief What to do when an effect is not visible.
struct PLASMA_PARTICLEPLUGIN_DLL plEffectInvisibleUpdateRate
{
  typedef plUInt8 StorageType;

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

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_PARTICLEPLUGIN_DLL, plEffectInvisibleUpdateRate);

//////////////////////////////////////////////////////////////////////////

struct PLASMA_PARTICLEPLUGIN_DLL plParticleTextureAtlasType
{
  typedef plUInt8 StorageType;

  enum Enum
  {
    None,

    RandomVariations,
    FlipbookAnimation,
    RandomYAnimatedX,

    Default = None
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_PARTICLEPLUGIN_DLL, plParticleTextureAtlasType);

//////////////////////////////////////////////////////////////////////////

struct PLASMA_PARTICLEPLUGIN_DLL plParticleColorGradientMode
{
  typedef plUInt8 StorageType;

  enum Enum
  {
    Age,
    Speed,

    Default = Age
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_PARTICLEPLUGIN_DLL, plParticleColorGradientMode);


//////////////////////////////////////////////////////////////////////////

struct PLASMA_PARTICLEPLUGIN_DLL plParticleOutOfBoundsMode
{
  typedef plUInt8 StorageType;

  enum Enum
  {
    Teleport,
    Die,

    Default = Teleport
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_PARTICLEPLUGIN_DLL, plParticleOutOfBoundsMode);

//////////////////////////////////////////////////////////////////////////

struct plParticleEffectFloatParam
{
  PLASMA_DECLARE_POD_TYPE();
  plHashedString m_sName;
  float m_Value;
};

struct plParticleEffectColorParam
{
  PLASMA_DECLARE_POD_TYPE();
  plHashedString m_sName;
  plColor m_Value;
};

class plParticleEffectParameters final : public plRefCounted
{
public:
  plHybridArray<plParticleEffectFloatParam, 2> m_FloatParams;
  plHybridArray<plParticleEffectColorParam, 2> m_ColorParams;
};
