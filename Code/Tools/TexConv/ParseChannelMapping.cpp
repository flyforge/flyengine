#include <TexConv/TexConvPCH.h>

#include <TexConv/TexConv.h>

static plStringView ToString(plTexConvChannelValue::Enum e)
{
  switch (e)
  {
    case plTexConvChannelValue::Red:
      return "Red";
    case plTexConvChannelValue::Green:
      return "Green";
    case plTexConvChannelValue::Blue:
      return "Blue";
    case plTexConvChannelValue::Alpha:
      return "Alpha";
    case plTexConvChannelValue::Black:
      return "Black";
    case plTexConvChannelValue::White:
      return "White";

    default:
      PL_ASSERT_NOT_IMPLEMENTED;
  }

  return "";
}

plResult plTexConv::ParseChannelMappings()
{
  if (m_Processor.m_Descriptor.m_OutputType == plTexConvOutputType::Atlas)
    return PL_SUCCESS;

  auto& mappings = m_Processor.m_Descriptor.m_ChannelMappings;

  PL_SUCCEED_OR_RETURN(ParseChannelSliceMapping(-1));

  for (plUInt32 slice = 0; slice < 64; ++slice)
  {
    const plUInt32 uiPrevMappings = mappings.GetCount();

    PL_SUCCEED_OR_RETURN(ParseChannelSliceMapping(slice));

    if (uiPrevMappings == mappings.GetCount())
    {
      // if no new mapping was found, don't try to find more
      break;
    }
  }

  if (!mappings.IsEmpty())
  {
    plLog::Info("Custom output channel mapping:");
    for (plUInt32 m = 0; m < mappings.GetCount(); ++m)
    {
      plLog::Info("Slice {}, R -> Input file {}, {}", m, mappings[m].m_Channel[0].m_iInputImageIndex, ToString(mappings[m].m_Channel[0].m_ChannelValue));
      plLog::Info("Slice {}, G -> Input file {}, {}", m, mappings[m].m_Channel[1].m_iInputImageIndex, ToString(mappings[m].m_Channel[1].m_ChannelValue));
      plLog::Info("Slice {}, B -> Input file {}, {}", m, mappings[m].m_Channel[2].m_iInputImageIndex, ToString(mappings[m].m_Channel[2].m_ChannelValue));
      plLog::Info("Slice {}, A -> Input file {}, {}", m, mappings[m].m_Channel[3].m_iInputImageIndex, ToString(mappings[m].m_Channel[3].m_ChannelValue));
    }
  }

  return PL_SUCCESS;
}

plResult plTexConv::ParseChannelSliceMapping(plInt32 iSlice)
{
  const auto pCmd = plCommandLineUtils::GetGlobalInstance();
  auto& mappings = m_Processor.m_Descriptor.m_ChannelMappings;
  plStringBuilder tmp, param;

  const plUInt32 uiMappingIdx = iSlice < 0 ? 0 : iSlice;

  // input to output mappings
  {
    param = "-rgba";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      PL_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[0], tmp, 0, false));
      PL_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[1], tmp, 1, false));
      PL_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[2], tmp, 2, false));
      PL_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[3], tmp, 3, false));
    }

    param = "-rgb";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      PL_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[0], tmp, 0, false));
      PL_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[1], tmp, 1, false));
      PL_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[2], tmp, 2, false));
    }

    param = "-rg";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      PL_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[0], tmp, 0, false));
      PL_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[1], tmp, 1, false));
    }

    param = "-r";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      PL_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[0], tmp, 0, true));
    }

    param = "-g";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      PL_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[1], tmp, 1, true));
    }

    param = "-b";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      PL_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[2], tmp, 2, true));
    }

    param = "-a";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      PL_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[3], tmp, 3, true));
    }
  }
  return PL_SUCCESS;
}

plResult plTexConv::ParseChannelMappingConfig(plTexConvChannelMapping& out_mapping, plStringView sCfg, plInt32 iChannelIndex, bool bSingleChannel)
{
  out_mapping.m_iInputImageIndex = -1;
  out_mapping.m_ChannelValue = plTexConvChannelValue::White;

  plStringBuilder tmp = sCfg;

  // '-r black' for setting it to zero
  if (tmp.IsEqual_NoCase("black"))
  {
    out_mapping.m_ChannelValue = plTexConvChannelValue::Black;
    return PL_SUCCESS;
  }

  // '-r white' for setting it to 255
  if (tmp.IsEqual_NoCase("white"))
  {
    out_mapping.m_ChannelValue = plTexConvChannelValue::White;
    return PL_SUCCESS;
  }

  // skip the 'in', if found
  // 'in' is optional, one can also write '-r 1.r' for '-r in1.r'
  if (tmp.StartsWith_NoCase("in"))
    tmp.Shrink(2, 0);

  if (tmp.StartsWith("."))
  {
    // no index given, e.g. '-r in.r'
    // in is equal to in0

    out_mapping.m_iInputImageIndex = 0;
  }
  else
  {
    plInt32 num = -1;
    const char* szLastPos = nullptr;
    if (plConversionUtils::StringToInt(tmp, num, &szLastPos).Failed())
    {
      plLog::Error("Could not parse channel mapping '{0}'", sCfg);
      return PL_FAILURE;
    }

    // valid index after the 'in'
    if (num >= 0 && num < (plInt32)m_Processor.m_Descriptor.m_InputFiles.GetCount())
    {
      out_mapping.m_iInputImageIndex = (plInt8)num;
    }
    else
    {
      plLog::Error("Invalid channel mapping input file index '{0}'", num);
      return PL_FAILURE;
    }

    plStringBuilder dummy = szLastPos;

    // continue after the index
    tmp = dummy;
  }

  // no additional info, e.g. '-g in2' is identical to '-g in2.g' (same channel)
  if (tmp.IsEmpty())
  {
    out_mapping.m_ChannelValue = (plTexConvChannelValue::Enum)((plInt32)plTexConvChannelValue::Red + iChannelIndex);
    return PL_SUCCESS;
  }

  if (!tmp.StartsWith("."))
  {
    plLog::Error("Invalid channel mapping: Expected '.' after input file index in '{0}'", sCfg);
    return PL_FAILURE;
  }

  tmp.Shrink(1, 0);

  if (!bSingleChannel)
  {
    // in case of '-rgb in1.bgr' map r to b, g to g, b to r, etc.
    // in case of '-rgb in1.r' map everything to the same input
    if (tmp.GetCharacterCount() > 1)
      tmp.Shrink(iChannelIndex, 0);
  }

  // no additional info, e.g. '-rgb in2.rg'
  if (tmp.IsEmpty())
  {
    plLog::Error("Invalid channel mapping: Too few channel identifiers '{0}'", sCfg);
    return PL_FAILURE;
  }

  {
    const plUInt32 uiChar = tmp.GetIteratorFront().GetCharacter();

    if (uiChar == 'r')
    {
      out_mapping.m_ChannelValue = plTexConvChannelValue::Red;
    }
    else if (uiChar == 'g')
    {
      out_mapping.m_ChannelValue = plTexConvChannelValue::Green;
    }
    else if (uiChar == 'b')
    {
      out_mapping.m_ChannelValue = plTexConvChannelValue::Blue;
    }
    else if (uiChar == 'a')
    {
      out_mapping.m_ChannelValue = plTexConvChannelValue::Alpha;
    }
    else
    {
      plLog::Error("Invalid channel mapping: Unexpected channel identifier in '{}'", sCfg);
      return PL_FAILURE;
    }

    tmp.Shrink(1, 0);
  }

  return PL_SUCCESS;
}
