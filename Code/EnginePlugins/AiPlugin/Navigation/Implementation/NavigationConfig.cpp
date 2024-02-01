#include <AiPlugin/Navigation/NavigationConfig.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

plAiNavigationConfig::plAiNavigationConfig()
{
  m_GroundTypes[0].m_bUsed = true;
  m_GroundTypes[0].m_sName = "<None>";
  m_GroundTypes[1].m_bUsed = true;
  m_GroundTypes[1].m_sName = "<Default>";

  plStringBuilder tmp;
  for (plUInt32 i = 2; i < plAiNumGroundTypes; ++i)
  {
    tmp.SetFormat("Custom Ground Type {}", i + 1);
    m_GroundTypes[i].m_sName = tmp;
  }

  for (plUInt32 i = 0; i < plAiNumGroundTypes / 8; ++i)
  {
    const float f = 1.0f + i * 0.5f;
    m_GroundTypes[(i * 8) + 0].m_Color = plColor::RosyBrown.GetDarker(f);
    m_GroundTypes[(i * 8) + 1].m_Color = plColor::CornflowerBlue.GetDarker(f);
    m_GroundTypes[(i * 8) + 2].m_Color = plColor::Crimson.GetDarker(f);
    m_GroundTypes[(i * 8) + 3].m_Color = plColor::Cyan.GetDarker(f);
    m_GroundTypes[(i * 8) + 4].m_Color = plColor::DarkSeaGreen.GetDarker(f);
    m_GroundTypes[(i * 8) + 5].m_Color = plColor::DarkViolet.GetDarker(f);
    m_GroundTypes[(i * 8) + 6].m_Color = plColor::HoneyDew.GetDarker(f);
    m_GroundTypes[(i * 8) + 7].m_Color = plColor::LightSalmon.GetDarker(f);
  }
}

plResult plAiNavigationConfig::Save(plStringView sFile) const
{
  plFileWriter file;
  if (file.Open(sFile).Failed())
    return PL_FAILURE;

  Save(file);

  return PL_SUCCESS;
}

plResult plAiNavigationConfig::Load(plStringView sFile)
{
  plFileReader file;
  if (file.Open(sFile).Failed())
    return PL_FAILURE;

  Load(file);
  return PL_SUCCESS;
}

void plAiNavigationConfig::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = 1;

  inout_stream << uiVersion;

  const plUInt8 numGroundTypes = plAiNumGroundTypes;
  inout_stream << numGroundTypes;

  for (plUInt32 i = 0; i < numGroundTypes; ++i)
  {
    const auto& gt = m_GroundTypes[i];
    inout_stream << gt.m_bUsed;
    inout_stream << gt.m_sName;
    inout_stream << gt.m_Color;
  }

  const plUInt8 numSearchTypes = m_PathSearchConfigs.GetCount();
  inout_stream << numSearchTypes;

  for (plUInt32 i = 0; i < numSearchTypes; ++i)
  {
    const auto& cfg = m_PathSearchConfigs[i];

    inout_stream << cfg.m_sName;
    inout_stream.WriteArray(cfg.m_fGroundTypeCost).AssertSuccess();
    inout_stream.WriteArray(cfg.m_bGroundTypeAllowed).AssertSuccess();
  }

  const plUInt8 numNavmeshTypes = m_NavmeshConfigs.GetCount();
  inout_stream << numNavmeshTypes;

  for (plUInt32 i = 0; i < numNavmeshTypes; ++i)
  {
    const auto& nc = m_NavmeshConfigs[i];

    inout_stream << nc.m_sName;
    inout_stream << nc.m_uiCollisionLayer;
    inout_stream << nc.m_fCellSize;
    inout_stream << nc.m_fCellHeight;
    inout_stream << nc.m_fAgentRadius;
    inout_stream << nc.m_fAgentHeight;
    inout_stream << nc.m_fAgentStepHeight;
    inout_stream << nc.m_WalkableSlope;

    inout_stream << nc.m_fMaxEdgeLength;
    inout_stream << nc.m_fMaxSimplificationError;
    inout_stream << nc.m_fMinRegionSize;
    inout_stream << nc.m_fRegionMergeSize;
    inout_stream << nc.m_fDetailMeshSampleDistanceFactor;
    inout_stream << nc.m_fDetailMeshSampleErrorFactor;
  }
}

void plAiNavigationConfig::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;

  inout_stream >> uiVersion;

  PL_ASSERT_DEV(uiVersion <= 1, "Invalid version {0} for plAiNavigationConfig file", uiVersion);

  plUInt8 numGroundTypes = 0;
  inout_stream >> numGroundTypes;

  numGroundTypes = plMath::Min<plUInt8>(numGroundTypes, plAiNumGroundTypes);

  for (plUInt32 i = 0; i < numGroundTypes; ++i)
  {
    auto& gt = m_GroundTypes[i];
    inout_stream >> gt.m_bUsed;
    inout_stream >> gt.m_sName;
    inout_stream >> gt.m_Color;
  }

  plUInt8 numSearchTypes = 0;
  inout_stream >> numSearchTypes;

  m_PathSearchConfigs.Clear();
  m_PathSearchConfigs.Reserve(numSearchTypes);

  for (plUInt32 i = 0; i < numSearchTypes; ++i)
  {
    auto& cfg = m_PathSearchConfigs.ExpandAndGetRef();

    inout_stream >> cfg.m_sName;
    inout_stream.ReadArray(cfg.m_fGroundTypeCost).AssertSuccess();
    inout_stream.ReadArray(cfg.m_bGroundTypeAllowed).AssertSuccess();
  }

  plUInt8 numNavmeshTypes = 0;
  inout_stream >> numNavmeshTypes;

  m_NavmeshConfigs.Clear();
  m_NavmeshConfigs.Reserve(numNavmeshTypes);

  for (plUInt32 i = 0; i < numNavmeshTypes; ++i)
  {
    auto& nc = m_NavmeshConfigs.ExpandAndGetRef();

    inout_stream >> nc.m_sName;
    inout_stream >> nc.m_uiCollisionLayer;
    inout_stream >> nc.m_fCellSize;
    inout_stream >> nc.m_fCellHeight;
    inout_stream >> nc.m_fAgentRadius;
    inout_stream >> nc.m_fAgentHeight;
    inout_stream >> nc.m_fAgentStepHeight;
    inout_stream >> nc.m_WalkableSlope;

    inout_stream >> nc.m_fMaxEdgeLength;
    inout_stream >> nc.m_fMaxSimplificationError;
    inout_stream >> nc.m_fMinRegionSize;
    inout_stream >> nc.m_fRegionMergeSize;
    inout_stream >> nc.m_fDetailMeshSampleDistanceFactor;
    inout_stream >> nc.m_fDetailMeshSampleErrorFactor;
  }
}

plAiPathSearchConfig::plAiPathSearchConfig()
{
  for (plUInt32 i = 0; i < plAiNumGroundTypes; ++i)
  {
    m_fGroundTypeCost[i] = 1.0f;
    m_bGroundTypeAllowed[i] = true;
  }
}
