#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/TagSet.h>
#include <Foundation/Types/Uuid.h>

/// \brief Describes the initial state of a game object.
struct PLASMA_CORE_DLL plGameObjectDesc
{
  PLASMA_DECLARE_POD_TYPE();

  bool m_bActiveFlag = true; ///< Whether the object should have the 'active flag' set. See plGameObject::SetActiveFlag().
  bool m_bDynamic = false;   ///< Whether the object should start out as 'dynamic'. See plGameObject::MakeDynamic().
  plUInt16 m_uiTeamID = 0;   ///< See plGameObject::GetTeamID().

  plHashedString m_sName;       ///< See plGameObject::SetName().
  plGameObjectHandle m_hParent; ///< An optional parent object to attach this object to as a child.

  plVec3 m_LocalPosition = plVec3::MakeZero();         ///< The local position relative to the parent (or the world)
  plQuat m_LocalRotation = plQuat::MakeIdentity(); ///< The local rotation relative to the parent (or the world)
  plVec3 m_LocalScaling = plVec3(1, 1, 1);               ///< The local scaling relative to the parent (or the world)
  float m_LocalUniformScaling = 1.0f;                    ///< An additional local uniform scaling relative to the parent (or the world)
  plTagSet m_Tags;                                       ///< See plGameObject::GetTags()
  plUInt32 m_uiStableRandomSeed = 0xFFFFFFFF;            ///< 0 means the game object gets a random value assigned, 0xFFFFFFFF means that if the object has a parent, the value will be derived deterministically from that one's seed, otherwise it gets a random value, any other value will be used directly
};
