#include <TexConv/TexConvPCH.h>

#include <TexConv/TexConv.h>

plResult plTexConv::ParseUIntOption(plStringView sOption, plInt32 iMinValue, plInt32 iMaxValue, plUInt32& ref_uiResult) const
{
  const auto pCmd = plCommandLineUtils::GetGlobalInstance();
  const plUInt32 uiDefault = ref_uiResult;

  const plInt32 val = pCmd->GetIntOption(sOption, ref_uiResult);

  if (!plMath::IsInRange(val, iMinValue, iMaxValue))
  {
    plLog::Error("'{}' value {} is out of valid range [{}; {}]", sOption, val, iMinValue, iMaxValue);
    return PLASMA_FAILURE;
  }

  ref_uiResult = static_cast<plUInt32>(val);

  if (ref_uiResult == uiDefault)
  {
    plLog::Info("Using default '{}': '{}'.", sOption, ref_uiResult);
    return PLASMA_SUCCESS;
  }

  plLog::Info("Selected '{}': '{}'.", sOption, ref_uiResult);

  return PLASMA_SUCCESS;
}

plResult plTexConv::ParseStringOption(plStringView sOption, const plDynamicArray<KeyEnumValuePair>& allowed, plInt32& ref_iResult) const
{
  const auto pCmd = plCommandLineUtils::GetGlobalInstance();
  const plStringBuilder sValue = pCmd->GetStringOption(sOption, 0);

  if (sValue.IsEmpty())
  {
    ref_iResult = allowed[0].m_iEnumValue;

    plLog::Info("Using default '{}': '{}'", sOption, allowed[0].m_sKey);
    return PLASMA_SUCCESS;
  }

  for (plUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    if (sValue.IsEqual_NoCase(allowed[i].m_sKey))
    {
      ref_iResult = allowed[i].m_iEnumValue;

      plLog::Info("Selected '{}': '{}'", sOption, allowed[i].m_sKey);
      return PLASMA_SUCCESS;
    }
  }

  plLog::Error("Unknown value for option '{}': '{}'.", sOption, sValue);

  PrintOptionValues(sOption, allowed);

  return PLASMA_FAILURE;
}

void plTexConv::PrintOptionValues(plStringView sOption, const plDynamicArray<KeyEnumValuePair>& allowed) const
{
  plLog::Info("Valid values for option '{}' are:", sOption);

  for (plUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    plLog::Info("  {}", allowed[i].m_sKey);
  }
}

void plTexConv::PrintOptionValuesHelp(plStringView sOption, const plDynamicArray<KeyEnumValuePair>& allowed) const
{
  plStringBuilder out(sOption, " ");

  for (plUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    if (i > 0)
      out.Append(" | ");

    out.Append(allowed[i].m_sKey);
  }

  plLog::Info(out);
}

bool plTexConv::ParseFile(plStringView sOption, plString& ref_sResult) const
{
  const auto pCmd = plCommandLineUtils::GetGlobalInstance();
  ref_sResult = pCmd->GetAbsolutePathOption(sOption);

  if (!ref_sResult.IsEmpty())
  {
    plLog::Info("'{}' file: '{}'", sOption, ref_sResult);
    return true;
  }
  else
  {
    plLog::Info("No '{}' file specified.", sOption);
    return false;
  }
}
