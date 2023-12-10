#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Tracks/ColorGradient.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/EventTrack.h>
#include <Foundation/Types/SharedPtr.h>
#include <GameEngine/GameEngineDLL.h>

struct PLASMA_GAMEENGINE_DLL plPropertyAnimTarget
{
  using StorageType = plUInt8;

  enum Enum
  {
    Number,
    VectorX,
    VectorY,
    VectorZ,
    VectorW,
    RotationX,
    RotationY,
    RotationZ,
    Color,

    Default = Number,
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GAMEENGINE_DLL, plPropertyAnimTarget);

//////////////////////////////////////////////////////////////////////////

struct PLASMA_GAMEENGINE_DLL plPropertyAnimMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    Once,
    Loop,
    BackAndForth,

    Default = Loop,
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GAMEENGINE_DLL, plPropertyAnimMode);

//////////////////////////////////////////////////////////////////////////

struct PLASMA_GAMEENGINE_DLL plPropertyAnimEntry
{
  plString m_sObjectSearchSequence; ///< Sequence of named objects to search for the target
  plString m_sComponentType;        ///< Empty to reference the game object properties (position etc.)
  plString m_sPropertyPath;
  plEnum<plPropertyAnimTarget> m_Target;
  const plRTTI* m_pComponentRtti = nullptr;
};

struct PLASMA_GAMEENGINE_DLL plFloatPropertyAnimEntry : public plPropertyAnimEntry
{
  plCurve1D m_Curve;
};

struct PLASMA_GAMEENGINE_DLL plColorPropertyAnimEntry : public plPropertyAnimEntry
{
  plColorGradient m_Gradient;
};

//////////////////////////////////////////////////////////////////////////

// this class is actually ref counted and used with plSharedPtr to allow to work on the same data, even when the resource was reloaded
struct PLASMA_GAMEENGINE_DLL plPropertyAnimResourceDescriptor : public plRefCounted
{
  plTime m_AnimationDuration;
  plDynamicArray<plFloatPropertyAnimEntry> m_FloatAnimations;
  plDynamicArray<plColorPropertyAnimEntry> m_ColorAnimations;
  plEventTrack m_EventTrack;

  void Save(plStreamWriter& inout_stream) const;
  void Load(plStreamReader& inout_stream);
};

//////////////////////////////////////////////////////////////////////////

using plPropertyAnimResourceHandle = plTypedResourceHandle<class plPropertyAnimResource>;

class PLASMA_GAMEENGINE_DLL plPropertyAnimResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plPropertyAnimResource, plResource);

  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plPropertyAnimResource);
  PLASMA_RESOURCE_DECLARE_CREATEABLE(plPropertyAnimResource, plPropertyAnimResourceDescriptor);

public:
  plPropertyAnimResource();

  plSharedPtr<plPropertyAnimResourceDescriptor> GetDescriptor() const { return m_pDescriptor; }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  plSharedPtr<plPropertyAnimResourceDescriptor> m_pDescriptor;
};
