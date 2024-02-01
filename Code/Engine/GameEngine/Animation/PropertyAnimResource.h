#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Tracks/ColorGradient.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/EventTrack.h>
#include <Foundation/Types/SharedPtr.h>
#include <GameEngine/GameEngineDLL.h>

/// \brief What data type an animation modifies.
struct PL_GAMEENGINE_DLL plPropertyAnimTarget
{
  using StorageType = plUInt8;

  enum Enum
  {
    Number,    ///< A single value.
    VectorX,   ///< The x coordinate of a vector.
    VectorY,   ///< The y coordinate of a vector.
    VectorZ,   ///< The z coordinate of a vector.
    VectorW,   ///< The w coordinate of a vector.
    RotationX, ///< The x coordinate of a rotation.
    RotationY, ///< The y coordinate of a rotation.
    RotationZ, ///< The z coordinate of a rotation.
    Color,     ///< A color.

    Default = Number,
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_GAMEENGINE_DLL, plPropertyAnimTarget);

//////////////////////////////////////////////////////////////////////////

/// \brief Describes how an animation should be played back.
struct PL_GAMEENGINE_DLL plPropertyAnimMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    Once,         ///< Play the animation once from start to end and then stop.
    Loop,         ///< Play the animation from start to end, then loop back to the start and repeat indefinitely.
    BackAndForth, ///< Play the animation from start to end, then reverse direction and play from end to start, then repeat indefinitely.

    Default = Loop,
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_GAMEENGINE_DLL, plPropertyAnimMode);

//////////////////////////////////////////////////////////////////////////

struct PL_GAMEENGINE_DLL plPropertyAnimEntry
{
  plString m_sObjectSearchSequence; ///< Sequence of named objects to search for the target
  plString m_sComponentType;        ///< Empty to reference the game object properties (position etc.)
  plString m_sPropertyPath;
  plEnum<plPropertyAnimTarget> m_Target;
  const plRTTI* m_pComponentRtti = nullptr;
};

struct PL_GAMEENGINE_DLL plFloatPropertyAnimEntry : public plPropertyAnimEntry
{
  plCurve1D m_Curve;
};

struct PL_GAMEENGINE_DLL plColorPropertyAnimEntry : public plPropertyAnimEntry
{
  plColorGradient m_Gradient;
};

//////////////////////////////////////////////////////////////////////////

// this class is actually ref counted and used with plSharedPtr to allow to work on the same data, even when the resource was reloaded
struct PL_GAMEENGINE_DLL plPropertyAnimResourceDescriptor : public plRefCounted
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

class PL_GAMEENGINE_DLL plPropertyAnimResource : public plResource
{
  PL_ADD_DYNAMIC_REFLECTION(plPropertyAnimResource, plResource);

  PL_RESOURCE_DECLARE_COMMON_CODE(plPropertyAnimResource);
  PL_RESOURCE_DECLARE_CREATEABLE(plPropertyAnimResource, plPropertyAnimResourceDescriptor);

public:
  plPropertyAnimResource();

  plSharedPtr<plPropertyAnimResourceDescriptor> GetDescriptor() const { return m_pDescriptor; }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  plSharedPtr<plPropertyAnimResourceDescriptor> m_pDescriptor;
};
