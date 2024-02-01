#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Shader/ShaderPermutationBinary.h>

struct plShaderPermutationBinaryVersion
{
  enum Enum : plUInt32
  {
    Version1 = 1,
    Version2 = 2,
    Version3 = 3,
    Version4 = 4,
    Version5 = 5,

    // Increase this version number to trigger shader recompilation

    ENUM_COUNT,
    Current = ENUM_COUNT - 1
  };
};

plShaderPermutationBinary::plShaderPermutationBinary()
{
  for (plUInt32 stage = 0; stage < plGALShaderStage::ENUM_COUNT; ++stage)
    m_uiShaderStageHashes[stage] = 0;
}

plResult plShaderPermutationBinary::Write(plStreamWriter& inout_stream)
{
  // write this at the beginning so that the file can be read as an plDependencyFile
  m_DependencyFile.StoreCurrentTimeStamp();
  PL_SUCCEED_OR_RETURN(m_DependencyFile.WriteDependencyFile(inout_stream));

  const plUInt8 uiVersion = plShaderPermutationBinaryVersion::Current;

  if (inout_stream.WriteBytes(&uiVersion, sizeof(plUInt8)).Failed())
    return PL_FAILURE;

  for (plUInt32 stage = 0; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (inout_stream.WriteDWordValue(&m_uiShaderStageHashes[stage]).Failed())
      return PL_FAILURE;
  }

  m_StateDescriptor.Save(inout_stream);

  inout_stream << m_PermutationVars.GetCount();

  for (auto& var : m_PermutationVars)
  {
    inout_stream << var.m_sName.GetString();
    inout_stream << var.m_sValue.GetString();
  }

  return PL_SUCCESS;
}

plResult plShaderPermutationBinary::Read(plStreamReader& inout_stream, bool& out_bOldVersion)
{
  PL_SUCCEED_OR_RETURN(m_DependencyFile.ReadDependencyFile(inout_stream));

  plUInt8 uiVersion = 0;

  if (inout_stream.ReadBytes(&uiVersion, sizeof(plUInt8)) != sizeof(plUInt8))
    return PL_FAILURE;

  PL_ASSERT_DEV(uiVersion <= plShaderPermutationBinaryVersion::Current, "Wrong Version {0}", uiVersion);

  out_bOldVersion = uiVersion != plShaderPermutationBinaryVersion::Current;

  for (plUInt32 stage = 0; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (inout_stream.ReadDWordValue(&m_uiShaderStageHashes[stage]).Failed())
      return PL_FAILURE;
  }

  m_StateDescriptor.Load(inout_stream);

  if (uiVersion >= plShaderPermutationBinaryVersion::Version2)
  {
    plUInt32 uiPermutationCount;
    inout_stream >> uiPermutationCount;

    m_PermutationVars.SetCount(uiPermutationCount);

    plStringBuilder tmp;
    for (plUInt32 i = 0; i < uiPermutationCount; ++i)
    {
      auto& var = m_PermutationVars[i];

      inout_stream >> tmp;
      var.m_sName.Assign(tmp.GetData());
      inout_stream >> tmp;
      var.m_sValue.Assign(tmp.GetData());
    }
  }

  return PL_SUCCESS;
}


