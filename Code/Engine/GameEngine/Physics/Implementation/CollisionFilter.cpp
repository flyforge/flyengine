#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GameEngine/Physics/CollisionFilter.h>


plCollisionFilterConfig::plCollisionFilterConfig()
{
  for (int i = 0; i < 32; ++i)
  {
    plMemoryUtils::ZeroFill<char>(m_GroupNames[i], 32);

    m_GroupMasks[i] = 0xFFFFFFFF; // collide with everything
  }
}

void plCollisionFilterConfig::SetGroupName(plUInt32 uiGroup, plStringView sName)
{
  plStringBuilder tmp;
  plStringUtils::Copy(m_GroupNames[uiGroup], 32, sName.GetData(tmp));
}

plStringView plCollisionFilterConfig::GetGroupName(plUInt32 uiGroup) const
{
  return m_GroupNames[uiGroup];
}

void plCollisionFilterConfig::EnableCollision(plUInt32 uiGroup1, plUInt32 uiGroup2, bool bEnable)
{
  if (bEnable)
  {
    m_GroupMasks[uiGroup1] |= PLASMA_BIT(uiGroup2);
    m_GroupMasks[uiGroup2] |= PLASMA_BIT(uiGroup1);
  }
  else
  {
    m_GroupMasks[uiGroup1] &= ~PLASMA_BIT(uiGroup2);
    m_GroupMasks[uiGroup2] &= ~PLASMA_BIT(uiGroup1);
  }
}

bool plCollisionFilterConfig::IsCollisionEnabled(plUInt32 uiGroup1, plUInt32 uiGroup2) const
{
  return (m_GroupMasks[uiGroup1] & PLASMA_BIT(uiGroup2)) != 0;
}

plUInt32 plCollisionFilterConfig::GetNumNamedGroups() const
{
  plUInt32 count = 0;

  for (plUInt32 i = 0; i < 32; ++i)
  {
    if (!plStringUtils::IsNullOrEmpty(m_GroupNames[i]))
      ++count;
  }

  return count;
}

plUInt32 plCollisionFilterConfig::GetNamedGroupIndex(plUInt32 uiGroup) const
{
  for (plUInt32 i = 0; i < 32; ++i)
  {
    if (!plStringUtils::IsNullOrEmpty(m_GroupNames[i]))
    {
      if (uiGroup == 0)
        return i;

      --uiGroup;
    }
  }

  PLASMA_REPORT_FAILURE("Invalid index, there are not so many named collision filter groups");
  return plInvalidIndex;
}

plUInt32 plCollisionFilterConfig::GetFilterGroupByName(plStringView sName) const
{
  for (plUInt32 i = 0; i < 32; ++i)
  {
    if (sName.IsEqual_NoCase(m_GroupNames[i]))
      return i;
  }

  return plInvalidIndex;
}

plUInt32 plCollisionFilterConfig::FindUnnamedGroup() const
{
  for (plUInt32 i = 0; i < 32; ++i)
  {
    if (plStringUtils::IsNullOrEmpty(m_GroupNames[i]))
      return i;
  }

  return plInvalidIndex;
}

plResult plCollisionFilterConfig::Save(plStringView sFile) const
{
  plFileWriter file;
  if (file.Open(sFile).Failed())
    return PLASMA_FAILURE;

  Save(file);

  return PLASMA_SUCCESS;
}

plResult plCollisionFilterConfig::Load(plStringView sFile)
{
#if PLASMA_ENABLED(PLASMA_MIGRATE_RUNTIMECONFIGS)
  if (sFile == s_sConfigFile)
  {
    sFile = plFileSystem::MigrateFileLocation(":project/CollisionLayers.cfg", s_sConfigFile);
  }
#endif

  plFileReader file;
  if (file.Open(sFile).Failed())
    return PLASMA_FAILURE;

  Load(file);
  return PLASMA_SUCCESS;
}

void plCollisionFilterConfig::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = 1;

  inout_stream << uiVersion;

  inout_stream.WriteBytes(m_GroupMasks, sizeof(plUInt32) * 32).IgnoreResult();
  inout_stream.WriteBytes(m_GroupNames, sizeof(char) * 32 * 32).IgnoreResult();
}


void plCollisionFilterConfig::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;

  inout_stream >> uiVersion;

  PLASMA_ASSERT_DEV(uiVersion == 1, "Invalid version {0} for plCollisionFilterConfig file", uiVersion);

  inout_stream.ReadBytes(m_GroupMasks, sizeof(plUInt32) * 32);
  inout_stream.ReadBytes(m_GroupNames, sizeof(char) * 32 * 32);
}



PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Physics_Implementation_CollisionFilter);
