#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/ConversionUtils.h>

PL_ENUMERABLE_CLASS_IMPLEMENTATION(plCommandLineOption);

void plCommandLineOption::GetSortingGroup(plStringBuilder& ref_sOut) const
{
  ref_sOut = m_sSortingGroup;
}

void plCommandLineOption::GetSplitOptions(plStringBuilder& out_sAll, plDynamicArray<plStringView>& ref_splitOptions) const
{
  GetOptions(out_sAll);
  out_sAll.Split(false, ref_splitOptions, ";", "|");
}

bool plCommandLineOption::IsHelpRequested(const plCommandLineUtils* pUtils /*= plCommandLineUtils::GetGlobalInstance()*/)
{
  return pUtils->GetBoolOption("-help") || pUtils->GetBoolOption("--help") || pUtils->GetBoolOption("-h") || pUtils->GetBoolOption("-?");
}

plResult plCommandLineOption::RequireOptions(plStringView sRequiredOptions, plString* pMissingOption /*= nullptr*/, const plCommandLineUtils* pUtils /*= plCommandLineUtils::GetGlobalInstance()*/)
{
  plStringBuilder tmp;
  plStringBuilder allOpts = sRequiredOptions;
  plHybridArray<plStringView, 16> options;
  allOpts.Split(false, options, ";");

  for (auto opt : options)
  {
    opt.Trim(" ");

    if (pUtils->GetOptionIndex(opt.GetData(tmp)) < 0)
    {
      if (pMissingOption)
      {
        *pMissingOption = opt;
      }

      return PL_FAILURE;
    }
  }

  if (pMissingOption)
  {
    pMissingOption->Clear();
  }

  return PL_SUCCESS;
}

bool plCommandLineOption::LogAvailableOptions(LogAvailableModes mode, plStringView sGroupFilter0 /*= {} */, const plCommandLineUtils* pUtils /*= plCommandLineUtils::GetGlobalInstance()*/)
{
  if (mode == LogAvailableModes::IfHelpRequested)
  {
    if (!IsHelpRequested(pUtils))
      return false;
  }

  plMap<plString, plHybridArray<plCommandLineOption*, 16>> sorted;

  plStringBuilder sGroupFilter;
  if (!sGroupFilter0.IsEmpty())
  {
    sGroupFilter.Set(";", sGroupFilter0, ";");
  }

  for (plCommandLineOption* pOpt = plCommandLineOption::GetFirstInstance(); pOpt != nullptr; pOpt = pOpt->GetNextInstance())
  {
    plStringBuilder sGroup;
    pOpt->GetSortingGroup(sGroup);
    sGroup.Prepend(";");
    sGroup.Append(";");

    if (!sGroupFilter.IsEmpty())
    {
      if (sGroupFilter.FindSubString_NoCase(sGroup) == nullptr)
        continue;
    }

    sorted[sGroup].PushBack(pOpt);
  }

  if (plApplication::GetApplicationInstance())
  {
    plLog::Info("");
    plLog::Info("{} command line options:", plApplication::GetApplicationInstance()->GetApplicationName());
  }

  if (sorted.IsEmpty())
  {
    plLog::Info("This application has no documented command line options.");
    return true;
  }

  plStringBuilder sLine;

  for (auto optIt : sorted)
  {
    for (auto pOpt : optIt.Value())
    {
      plStringBuilder sOptions, sParamShort, sParamDefault, sLongDesc;

      sLine.Clear();

      pOpt->GetOptions(sOptions);
      pOpt->GetParamShortDesc(sParamShort);
      pOpt->GetParamDefaultValueDesc(sParamDefault);
      pOpt->GetLongDesc(sLongDesc);

      plHybridArray<plStringView, 4> lines;

      sOptions.Split(false, lines, ";", "|");

      for (auto o : lines)
      {
        sLine.AppendWithSeparator(", ", o);
      }

      if (!sParamShort.IsEmpty())
      {
        sLine.Append(" ", sParamShort);

        if (!sParamDefault.IsEmpty())
        {
          sLine.Append(" = ", sParamDefault);
        }
      }

      plLog::Info("");
      plLog::Info(sLine);

      sLongDesc.Trim(" \t\n\r");
      sLongDesc.Split(true, lines, "\n");

      for (auto o : lines)
      {
        sLine = o;
        sLine.Trim("\t\n\r");
        sLine.Prepend("    ");

        plLog::Info(sLine);
      }
    }

    plLog::Info("");
  }

  plLog::Info("");

  return true;
}


bool plCommandLineOption::LogAvailableOptionsToBuffer(plStringBuilder& out_sBuffer, LogAvailableModes mode, plStringView sGroupFilter /*= {} */, const plCommandLineUtils* pUtils /*= plCommandLineUtils::GetGlobalInstance()*/)
{
  plLogSystemToBuffer log;
  plLogSystemScope ls(&log);

  const bool res = plCommandLineOption::LogAvailableOptions(mode, sGroupFilter, pUtils);

  out_sBuffer = log.m_sBuffer;

  return res;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plCommandLineOptionDoc::plCommandLineOptionDoc(plStringView sSortingGroup, plStringView sArgument, plStringView sParamShortDesc, plStringView sLongDesc, plStringView sDefaultValue, bool bCaseSensitive /*= false*/)
  : plCommandLineOption(sSortingGroup)
{
  m_sArgument = sArgument;
  m_sParamShortDesc = sParamShortDesc;
  m_sParamDefaultValue = sDefaultValue;
  m_sLongDesc = sLongDesc;
  m_bCaseSensitive = bCaseSensitive;
}

void plCommandLineOptionDoc::GetOptions(plStringBuilder& ref_sOut) const
{
  ref_sOut = m_sArgument;
}

void plCommandLineOptionDoc::GetParamShortDesc(plStringBuilder& ref_sOut) const
{
  ref_sOut = m_sParamShortDesc;
}

void plCommandLineOptionDoc::GetParamDefaultValueDesc(plStringBuilder& ref_sOut) const
{
  ref_sOut = m_sParamDefaultValue;
}

void plCommandLineOptionDoc::GetLongDesc(plStringBuilder& ref_sOut) const
{
  ref_sOut = m_sLongDesc;
}

bool plCommandLineOptionDoc::IsOptionSpecified(plStringBuilder* out_pWhich, const plCommandLineUtils* pUtils /*= plCommandLineUtils::GetGlobalInstance()*/) const
{
  plStringBuilder sOptions, tmp;
  plHybridArray<plStringView, 4> eachOption;
  GetSplitOptions(sOptions, eachOption);

  for (auto o : eachOption)
  {
    if (pUtils->GetOptionIndex(o.GetData(tmp), m_bCaseSensitive) >= 0)
    {
      if (out_pWhich)
      {
        *out_pWhich = tmp;
      }

      return true;
    }
  }

  if (out_pWhich)
  {
    *out_pWhich = m_sArgument;
  }

  return false;
}


bool plCommandLineOptionDoc::ShouldLog(LogMode mode, bool bWasSpecified) const
{
  if (mode == LogMode::Never)
    return false;

  if (m_bLoggedOnce && (mode == LogMode::FirstTime || mode == LogMode::FirstTimeIfSpecified))
    return false;

  if (!bWasSpecified && (mode == LogMode::FirstTimeIfSpecified || mode == LogMode::AlwaysIfSpecified))
    return false;

  return true;
}

void plCommandLineOptionDoc::LogOption(plStringView sOption, plStringView sValue, bool bWasSpecified) const
{
  m_bLoggedOnce = true;

  if (bWasSpecified)
  {
    plLog::Info("Option '{}' is set to '{}'", sOption, sValue);
  }
  else
  {
    plLog::Info("Option '{}' is not set, default value is '{}'", sOption, sValue);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plCommandLineOptionBool::plCommandLineOptionBool(plStringView sSortingGroup, plStringView sArgument, plStringView sLongDesc, bool bDefaultValue, bool bCaseSensitive /*= false*/)
  : plCommandLineOptionDoc(sSortingGroup, sArgument, "<bool>", sLongDesc, bDefaultValue ? "true" : "false", bCaseSensitive)
{
  m_bDefaultValue = bDefaultValue;
}

bool plCommandLineOptionBool::GetOptionValue(LogMode logMode, const plCommandLineUtils* pUtils /*= plCommandLineUtils::GetGlobalInstance()*/) const
{
  bool result = m_bDefaultValue;

  plStringBuilder sOption;
  const bool bSpecified = IsOptionSpecified(&sOption, pUtils);

  if (bSpecified)
  {
    result = pUtils->GetBoolOption(sOption, m_bDefaultValue, m_bCaseSensitive);
  }

  if (ShouldLog(logMode, bSpecified))
  {
    LogOption(sOption, result ? "true" : "false", bSpecified);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plCommandLineOptionInt::plCommandLineOptionInt(plStringView sSortingGroup, plStringView sArgument, plStringView sLongDesc, int iDefaultValue, int iMinValue /*= plMath::MinValue<int>()*/, int iMaxValue /*= plMath::MaxValue<int>()*/, bool bCaseSensitive /*= false*/)
  : plCommandLineOptionDoc(sSortingGroup, sArgument, "<int>", sLongDesc, "0", bCaseSensitive)
{
  m_iDefaultValue = iDefaultValue;
  m_iMinValue = iMinValue;
  m_iMaxValue = iMaxValue;

  PL_ASSERT_DEV(m_iMinValue < m_iMaxValue, "Invalid min/max value");
}

void plCommandLineOptionInt::GetParamDefaultValueDesc(plStringBuilder& ref_sOut) const
{
  ref_sOut.SetFormat("{}", m_iDefaultValue);
}


void plCommandLineOptionInt::GetParamShortDesc(plStringBuilder& ref_sOut) const
{
  if (m_iMinValue == plMath::MinValue<int>() && m_iMaxValue == plMath::MaxValue<int>())
  {
    ref_sOut = "<int>";
  }
  else
  {
    ref_sOut.SetFormat("<int> [{} .. {}]", m_iMinValue, m_iMaxValue);
  }
}

int plCommandLineOptionInt::GetOptionValue(LogMode logMode, const plCommandLineUtils* pUtils /*= plCommandLineUtils::GetGlobalInstance()*/) const
{
  int result = m_iDefaultValue;

  plStringBuilder sOption, tmp;
  const bool bSpecified = IsOptionSpecified(&sOption, pUtils);

  if (bSpecified)
  {
    result = pUtils->GetIntOption(sOption, m_iDefaultValue, m_bCaseSensitive);

    if (result < m_iMinValue || result > m_iMaxValue)
    {
      if (ShouldLog(logMode, bSpecified))
      {
        plLog::Warning("Option '{}' selected value '{}' is outside valid range [{} .. {}]. Using default value instead.", sOption, result, m_iMinValue, m_iMaxValue);
      }

      result = m_iDefaultValue;
    }
  }

  if (ShouldLog(logMode, bSpecified))
  {
    tmp.SetFormat("{}", result);
    LogOption(sOption, tmp, bSpecified);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plCommandLineOptionFloat::plCommandLineOptionFloat(plStringView sSortingGroup, plStringView sArgument, plStringView sLongDesc, float fDefaultValue, float fMinValue /*= plMath::MinValue<float>()*/, float fMaxValue /*= plMath::MaxValue<float>()*/, bool bCaseSensitive /*= false*/)
  : plCommandLineOptionDoc(sSortingGroup, sArgument, "<float>", sLongDesc, "0", bCaseSensitive)
{
  m_fDefaultValue = fDefaultValue;
  m_fMinValue = fMinValue;
  m_fMaxValue = fMaxValue;

  PL_ASSERT_DEV(m_fMinValue < m_fMaxValue, "Invalid min/max value");
}

void plCommandLineOptionFloat::GetParamDefaultValueDesc(plStringBuilder& ref_sOut) const
{
  ref_sOut.SetFormat("{}", m_fDefaultValue);
}

void plCommandLineOptionFloat::GetParamShortDesc(plStringBuilder& ref_sOut) const
{
  if (m_fMinValue == plMath::MinValue<float>() && m_fMaxValue == plMath::MaxValue<float>())
  {
    ref_sOut = "<float>";
  }
  else
  {
    ref_sOut.SetFormat("<float> [{} .. {}]", m_fMinValue, m_fMaxValue);
  }
}

float plCommandLineOptionFloat::GetOptionValue(LogMode logMode, const plCommandLineUtils* pUtils /*= plCommandLineUtils::GetGlobalInstance()*/) const
{
  float result = m_fDefaultValue;

  plStringBuilder sOption, tmp;
  const bool bSpecified = IsOptionSpecified(&sOption, pUtils);

  if (bSpecified)
  {
    result = static_cast<float>(pUtils->GetFloatOption(sOption, m_fDefaultValue, m_bCaseSensitive));

    if (result < m_fMinValue || result > m_fMaxValue)
    {
      if (ShouldLog(logMode, bSpecified))
      {
        plLog::Warning("Option '{}' selected value '{}' is outside valid range [{} .. {}]. Using default value instead.", sOption, result, m_fMinValue, m_fMaxValue);
      }

      result = m_fDefaultValue;
    }
  }

  if (ShouldLog(logMode, bSpecified))
  {
    tmp.SetFormat("{}", result);
    LogOption(sOption, tmp, bSpecified);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plCommandLineOptionString::plCommandLineOptionString(plStringView sSortingGroup, plStringView sArgument, plStringView sLongDesc, plStringView sDefaultValue, bool bCaseSensitive /*= false*/)
  : plCommandLineOptionDoc(sSortingGroup, sArgument, "<string>", sLongDesc, sDefaultValue, bCaseSensitive)
{
  m_sDefaultValue = sDefaultValue;
}

plStringView plCommandLineOptionString::GetOptionValue(LogMode logMode, const plCommandLineUtils* pUtils /*= plCommandLineUtils::GetGlobalInstance()*/) const
{
  plStringView result = m_sDefaultValue;

  plStringBuilder sOption;
  const bool bSpecified = IsOptionSpecified(&sOption, pUtils);

  if (bSpecified)
  {
    result = pUtils->GetStringOption(sOption, 0, m_sDefaultValue, m_bCaseSensitive);
  }

  if (ShouldLog(logMode, bSpecified))
  {
    LogOption(sOption, result, bSpecified);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plCommandLineOptionPath::plCommandLineOptionPath(plStringView sSortingGroup, plStringView sArgument, plStringView sLongDesc, plStringView sDefaultValue, bool bCaseSensitive /*= false*/)
  : plCommandLineOptionDoc(sSortingGroup, sArgument, "<path>", sLongDesc, sDefaultValue, bCaseSensitive)
{
  m_sDefaultValue = sDefaultValue;
}

plString plCommandLineOptionPath::GetOptionValue(LogMode logMode, const plCommandLineUtils* pUtils /*= plCommandLineUtils::GetGlobalInstance()*/) const
{
  plString result = m_sDefaultValue;

  plStringBuilder sOption;
  const bool bSpecified = IsOptionSpecified(&sOption, pUtils);

  if (bSpecified)
  {
    result = pUtils->GetAbsolutePathOption(sOption, 0, m_sDefaultValue, m_bCaseSensitive);
  }

  if (ShouldLog(logMode, bSpecified))
  {
    LogOption(sOption, result, bSpecified);
  }

  return result;
}

plCommandLineOptionEnum::plCommandLineOptionEnum(plStringView sSortingGroup, plStringView sArgument, plStringView sLongDesc, plStringView sEnumKeysAndValues, plInt32 iDefaultValue, bool bCaseSensitive /*= false*/)
  : plCommandLineOptionDoc(sSortingGroup, sArgument, "<enum>", sLongDesc, "", bCaseSensitive)
{
  m_iDefaultValue = iDefaultValue;
  m_sEnumKeysAndValues = sEnumKeysAndValues;
}

plInt32 plCommandLineOptionEnum::GetOptionValue(LogMode logMode, const plCommandLineUtils* pUtils /*= plCommandLineUtils::GetGlobalInstance()*/) const
{
  plInt32 result = m_iDefaultValue;

  plStringBuilder sOption;
  const bool bSpecified = IsOptionSpecified(&sOption, pUtils);

  plHybridArray<EnumKeyValue, 16> keysAndValues;
  GetEnumKeysAndValues(keysAndValues);

  if (bSpecified)
  {
    plStringView selected = pUtils->GetStringOption(sOption, 0, "", m_bCaseSensitive);

    for (const auto& e : keysAndValues)
    {
      if (e.m_Key.IsEqual_NoCase(selected))
      {
        result = e.m_iValue;
        goto found;
      }
    }

    if (ShouldLog(logMode, bSpecified))
    {
      plLog::Warning("Option '{}' selected value '{}' is unknown. Using default value instead.", sOption, selected);
    }
  }

found:

  if (ShouldLog(logMode, bSpecified))
  {
    plStringBuilder opt;

    for (const auto& e : keysAndValues)
    {
      if (e.m_iValue == result)
      {
        opt = e.m_Key;
        break;
      }
    }

    LogOption(sOption, opt, bSpecified);
  }

  return result;
}

void plCommandLineOptionEnum::GetParamShortDesc(plStringBuilder& ref_sOut) const
{
  plHybridArray<EnumKeyValue, 16> keysAndValues;
  GetEnumKeysAndValues(keysAndValues);

  for (const auto& e : keysAndValues)
  {
    ref_sOut.AppendWithSeparator(" | ", e.m_Key);
  }

  ref_sOut.Prepend("<");
  ref_sOut.Append(">");
}

void plCommandLineOptionEnum::GetParamDefaultValueDesc(plStringBuilder& ref_sOut) const
{
  plHybridArray<EnumKeyValue, 16> keysAndValues;
  GetEnumKeysAndValues(keysAndValues);

  for (const auto& e : keysAndValues)
  {
    if (m_iDefaultValue == e.m_iValue)
    {
      ref_sOut = e.m_Key;
      return;
    }
  }
}

void plCommandLineOptionEnum::GetEnumKeysAndValues(plDynamicArray<EnumKeyValue>& out_keysAndValues) const
{
  plStringBuilder tmp = m_sEnumKeysAndValues;

  plHybridArray<plStringView, 16> enums;
  tmp.Split(false, enums, ";", "|");

  out_keysAndValues.SetCount(enums.GetCount());

  plInt32 eVal = 0;
  for (plUInt32 e = 0; e < enums.GetCount(); ++e)
  {
    plStringView eName;

    if (const char* eq = enums[e].FindSubString("="))
    {
      eName = plStringView(enums[e].GetStartPointer(), eq);

      PL_VERIFY(plConversionUtils::StringToInt(eq + 1, eVal).Succeeded(), "Invalid enum declaration");
    }
    else
    {
      eName = enums[e];
    }

    eName.Trim(" \n\r\t=");

    const char* pStart = m_sEnumKeysAndValues.GetStartPointer();
    pStart += (plInt64)eName.GetStartPointer();
    pStart -= (plInt64)tmp.GetData();

    out_keysAndValues[e].m_iValue = eVal;
    out_keysAndValues[e].m_Key = plStringView(pStart, eName.GetElementCount());

    eVal++;
  }
}


