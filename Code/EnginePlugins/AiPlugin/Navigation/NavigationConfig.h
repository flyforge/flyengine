#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Strings/String.h>

static constexpr plUInt32 plAiNumGroundTypes = 32;

struct PLASMA_AIPLUGIN_DLL plAiNavmeshConfig
{
  plString m_sName;

  /// The physics collision layer to use for building this navmesh (retrieving the physics geometry).
  plUInt8 m_uiCollisionLayer = 0;

  float m_fCellSize = 0.2f;
  float m_fCellHeight = 0.2f;

  float m_fAgentRadius = 0.2f;
  float m_fAgentHeight = 1.5f;
  float m_fAgentStepHeight = 0.6f;

  plAngle m_WalkableSlope = plAngle::MakeFromDegree(45);

  float m_fMaxEdgeLength = 4.0f;
  float m_fMaxSimplificationError = 1.3f;
  float m_fMinRegionSize = 0.5f;
  float m_fRegionMergeSize = 5.0f;
  float m_fDetailMeshSampleDistanceFactor = 1.0f;
  float m_fDetailMeshSampleErrorFactor = 1.0f;
};

struct PLASMA_AIPLUGIN_DLL plAiPathSearchConfig
{
  plAiPathSearchConfig();

  plString m_sName;
  float m_fGroundTypeCost[plAiNumGroundTypes];   // = 1.0f
  bool m_bGroundTypeAllowed[plAiNumGroundTypes]; // = true
};

struct PLASMA_AIPLUGIN_DLL plAiNavigationConfig
{
  plAiNavigationConfig();

  struct GroundType
  {
    bool m_bUsed = false;
    plString m_sName;
    plColorGammaUB m_Color;
  };

  GroundType m_GroundTypes[plAiNumGroundTypes];

  plDynamicArray<plAiPathSearchConfig> m_PathSearchConfigs;
  plDynamicArray<plAiNavmeshConfig> m_NavmeshConfigs;

  static constexpr const plStringView s_sConfigFile = ":project/RuntimeConfigs/AiPluginConfig.cfg"_plsv;

  plResult Save(plStringView sFile = s_sConfigFile) const;
  plResult Load(plStringView sFile = s_sConfigFile);

  void Save(plStreamWriter& inout_stream) const;
  void Load(plStreamReader& inout_stream);
};
