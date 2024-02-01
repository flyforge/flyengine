#pragma once

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Strings/String.h>
#include <GameEngine/GameEngineDLL.h>

/// \brief A 32x32 matrix of named filters that can be configured to enable or disable collisions
class PL_GAMEENGINE_DLL plCollisionFilterConfig
{
public:
  plCollisionFilterConfig();

  void SetGroupName(plUInt32 uiGroup, plStringView sName);

  plStringView GetGroupName(plUInt32 uiGroup) const;

  void EnableCollision(plUInt32 uiGroup1, plUInt32 uiGroup2, bool bEnable = true);

  bool IsCollisionEnabled(plUInt32 uiGroup1, plUInt32 uiGroup2) const;

  inline plUInt32 GetFilterMask(plUInt32 uiGroup) const { return m_GroupMasks[uiGroup]; }

  /// \brief Returns how many groups have non-empty names
  plUInt32 GetNumNamedGroups() const;

  /// \brief Returns the index of the n-th group that has a non-empty name (ie. maps index '3' to index '5' if there are two unnamed groups in
  /// between)
  plUInt32 GetNamedGroupIndex(plUInt32 uiGroup) const;

  /// \brief Returns plInvalidIndex if no group with the given name exists.
  plUInt32 GetFilterGroupByName(plStringView sName) const;

  /// \brief Searches for a group without a name and returns the index or plInvalidIndex if none found.
  plUInt32 FindUnnamedGroup() const;

  void Save(plStreamWriter& inout_stream) const;
  void Load(plStreamReader& inout_stream);

  static constexpr const plStringView s_sConfigFile = ":project/RuntimeConfigs/CollisionLayers.cfg"_plsv;

  plResult Save(plStringView sFile = s_sConfigFile) const;
  plResult Load(plStringView sFile = s_sConfigFile);


private:
  plUInt32 m_GroupMasks[32];
  char m_GroupNames[32][32];
};
