#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Shader/ShaderPermutationBinary.h>

struct plShaderStateVersion
{
  enum Enum : plUInt32
  {
    Version0 = 0,
    Version1,
    Version2,
    Version3,

    ENUM_COUNT,
    Current = ENUM_COUNT - 1
  };
};

void plShaderStateResourceDescriptor::Save(plStreamWriter& inout_stream) const
{
  inout_stream << (plUInt32)plShaderStateVersion::Current;

  // Blend State
  {
    inout_stream << m_BlendDesc.m_bAlphaToCoverage;
    inout_stream << m_BlendDesc.m_bIndependentBlend;

    const plUInt8 iBlends = m_BlendDesc.m_bIndependentBlend ? PL_GAL_MAX_RENDERTARGET_COUNT : 1;
    inout_stream << iBlends; // in case PL_GAL_MAX_RENDERTARGET_COUNT ever changes

    for (plUInt32 b = 0; b < iBlends; ++b)
    {
      inout_stream << m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_bBlendingEnabled;
      inout_stream << (plUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOp;
      inout_stream << (plUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOpAlpha;
      inout_stream << (plUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlend;
      inout_stream << (plUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlendAlpha;
      inout_stream << (plUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlend;
      inout_stream << (plUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlendAlpha;
      inout_stream << m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_uiWriteMask;
    }
  }

  // Depth Stencil State
  {
    inout_stream << (plUInt8)m_DepthStencilDesc.m_DepthTestFunc;
    inout_stream << m_DepthStencilDesc.m_bDepthTest;
    inout_stream << m_DepthStencilDesc.m_bDepthWrite;
    inout_stream << m_DepthStencilDesc.m_bSeparateFrontAndBack;
    inout_stream << m_DepthStencilDesc.m_bStencilTest;
    inout_stream << m_DepthStencilDesc.m_uiStencilReadMask;
    inout_stream << m_DepthStencilDesc.m_uiStencilWriteMask;
    inout_stream << (plUInt8)m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp;
    inout_stream << (plUInt8)m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp;
    inout_stream << (plUInt8)m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp;
    inout_stream << (plUInt8)m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc;
    inout_stream << (plUInt8)m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp;
    inout_stream << (plUInt8)m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp;
    inout_stream << (plUInt8)m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp;
    inout_stream << (plUInt8)m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc;
  }

  // Rasterizer State
  {
    inout_stream << m_RasterizerDesc.m_bFrontCounterClockwise;
    inout_stream << m_RasterizerDesc.m_bScissorTest;
    inout_stream << m_RasterizerDesc.m_bWireFrame;
    inout_stream << (plUInt8)m_RasterizerDesc.m_CullMode;
    inout_stream << m_RasterizerDesc.m_fDepthBiasClamp;
    inout_stream << m_RasterizerDesc.m_fSlopeScaledDepthBias;
    inout_stream << m_RasterizerDesc.m_iDepthBias;
    inout_stream << m_RasterizerDesc.m_bConservativeRasterization;
  }
}

void plShaderStateResourceDescriptor::Load(plStreamReader& inout_stream)
{
  plUInt32 uiVersion = 0;
  inout_stream >> uiVersion;

  PL_ASSERT_DEV(uiVersion >= plShaderStateVersion::Version1 && uiVersion <= plShaderStateVersion::Current, "Invalid version {0}", uiVersion);

  // Blend State
  {
    inout_stream >> m_BlendDesc.m_bAlphaToCoverage;
    inout_stream >> m_BlendDesc.m_bIndependentBlend;

    plUInt8 iBlends = 0;
    inout_stream >> iBlends; // in case PL_GAL_MAX_RENDERTARGET_COUNT ever changes

    for (plUInt32 b = 0; b < iBlends; ++b)
    {
      plUInt8 uiTemp;
      inout_stream >> m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_bBlendingEnabled;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOp = (plGALBlendOp::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOpAlpha = (plGALBlendOp::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlend = (plGALBlend::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlendAlpha = (plGALBlend::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlend = (plGALBlend::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlendAlpha = (plGALBlend::Enum)uiTemp;
      inout_stream >> m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_uiWriteMask;
    }
  }

  // Depth Stencil State
  {
    plUInt8 uiTemp = 0;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_DepthTestFunc = (plGALCompareFunc::Enum)uiTemp;
    inout_stream >> m_DepthStencilDesc.m_bDepthTest;
    inout_stream >> m_DepthStencilDesc.m_bDepthWrite;
    inout_stream >> m_DepthStencilDesc.m_bSeparateFrontAndBack;
    inout_stream >> m_DepthStencilDesc.m_bStencilTest;
    inout_stream >> m_DepthStencilDesc.m_uiStencilReadMask;
    inout_stream >> m_DepthStencilDesc.m_uiStencilWriteMask;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp = (plGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp = (plGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp = (plGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc = (plGALCompareFunc::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp = (plGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp = (plGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp = (plGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc = (plGALCompareFunc::Enum)uiTemp;
  }

  // Rasterizer State
  {
    plUInt8 uiTemp = 0;

    if (uiVersion < plShaderStateVersion::Version2)
    {
      bool dummy;
      inout_stream >> dummy;
    }

    inout_stream >> m_RasterizerDesc.m_bFrontCounterClockwise;

    if (uiVersion < plShaderStateVersion::Version2)
    {
      bool dummy;
      inout_stream >> dummy;
      inout_stream >> dummy;
    }

    inout_stream >> m_RasterizerDesc.m_bScissorTest;
    inout_stream >> m_RasterizerDesc.m_bWireFrame;
    inout_stream >> uiTemp;
    m_RasterizerDesc.m_CullMode = (plGALCullMode::Enum)uiTemp;
    inout_stream >> m_RasterizerDesc.m_fDepthBiasClamp;
    inout_stream >> m_RasterizerDesc.m_fSlopeScaledDepthBias;
    inout_stream >> m_RasterizerDesc.m_iDepthBias;

    if (uiVersion >= plShaderStateVersion::Version3)
    {
      inout_stream >> m_RasterizerDesc.m_bConservativeRasterization;
    }
  }
}

plUInt32 plShaderStateResourceDescriptor::CalculateHash() const
{
  return m_BlendDesc.CalculateHash() + m_RasterizerDesc.CalculateHash() + m_DepthStencilDesc.CalculateHash();
}

static const char* InsertNumber(const char* szString, plUInt32 uiNumber, plStringBuilder& ref_sTemp)
{
  ref_sTemp.SetFormat(szString, uiNumber);
  return ref_sTemp.GetData();
}

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
static plSet<plString> s_AllAllowedVariables;
#endif

static bool GetBoolStateVariable(const plMap<plString, plString>& variables, const char* szVariable, bool bDefValue)
{
#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return bDefValue;

  if (it.Value() == "true")
    return true;
  if (it.Value() == "false")
    return false;

  plLog::Error("Shader state variable '{0}' is set to invalid value '{1}'. Should be 'true' or 'false'", szVariable, it.Value());
  return bDefValue;
}

static plInt32 GetEnumStateVariable(
  const plMap<plString, plString>& variables, const plMap<plString, plInt32>& values, const char* szVariable, plInt32 iDefValue)
{
#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return iDefValue;

  auto itVal = values.Find(it.Value());
  if (!itVal.IsValid())
  {
    plStringBuilder valid;
    for (auto vv = values.GetIterator(); vv.IsValid(); ++vv)
    {
      valid.Append(" ", vv.Key());
    }

    plLog::Error("Shader state variable '{0}' is set to invalid value '{1}'. Valid values are:{2}", szVariable, it.Value(), valid);
    return iDefValue;
  }

  return itVal.Value();
}

static float GetFloatStateVariable(const plMap<plString, plString>& variables, const char* szVariable, float fDefValue)
{
#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return fDefValue;

  double result = 0;
  if (plConversionUtils::StringToFloat(it.Value(), result).Failed())
  {
    plLog::Error("Shader state variable '{0}' is not a valid float value: '{1}'.", szVariable, it.Value());
    return fDefValue;
  }

  return (float)result;
}

static plInt32 GetIntStateVariable(const plMap<plString, plString>& variables, const char* szVariable, plInt32 iDefValue)
{
#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return iDefValue;

  plInt32 result = 0;
  if (plConversionUtils::StringToInt(it.Value(), result).Failed())
  {
    plLog::Error("Shader state variable '{0}' is not a valid int value: '{1}'.", szVariable, it.Value());
    return iDefValue;
  }

  return result;
}

// Global variables don't use memory tracking, so these won't reported as memory leaks.
static plMap<plString, plInt32> StateValuesBlend;
static plMap<plString, plInt32> StateValuesBlendOp;
static plMap<plString, plInt32> StateValuesCullMode;
static plMap<plString, plInt32> StateValuesCompareFunc;
static plMap<plString, plInt32> StateValuesStencilOp;

plResult plShaderStateResourceDescriptor::Parse(const char* szSource)
{
  plMap<plString, plString> VariableValues;

  // extract all state assignments
  {
    plStringBuilder sSource = szSource;

    plHybridArray<plStringView, 32> allAssignments;
    plHybridArray<plStringView, 4> components;
    sSource.Split(false, allAssignments, "\n", ";", "\r");

    plStringBuilder temp;
    for (const plStringView& assignment : allAssignments)
    {
      temp = assignment;
      temp.Trim(" \t\r\n;");
      if (temp.IsEmpty())
        continue;

      temp.Split(false, components, " ", "\t", "=", "\r");

      if (components.GetCount() != 2)
      {
        plLog::Error("Malformed shader state assignment: '{0}'", temp);
        continue;
      }

      VariableValues[components[0]] = components[1];
    }
  }

  if (StateValuesBlend.IsEmpty())
  {
    // plGALBlend
    {
      StateValuesBlend["Blend_Zero"] = plGALBlend::Zero;
      StateValuesBlend["Blend_One"] = plGALBlend::One;
      StateValuesBlend["Blend_SrcColor"] = plGALBlend::SrcColor;
      StateValuesBlend["Blend_InvSrcColor"] = plGALBlend::InvSrcColor;
      StateValuesBlend["Blend_SrcAlpha"] = plGALBlend::SrcAlpha;
      StateValuesBlend["Blend_InvSrcAlpha"] = plGALBlend::InvSrcAlpha;
      StateValuesBlend["Blend_DestAlpha"] = plGALBlend::DestAlpha;
      StateValuesBlend["Blend_InvDestAlpha"] = plGALBlend::InvDestAlpha;
      StateValuesBlend["Blend_DestColor"] = plGALBlend::DestColor;
      StateValuesBlend["Blend_InvDestColor"] = plGALBlend::InvDestColor;
      StateValuesBlend["Blend_SrcAlphaSaturated"] = plGALBlend::SrcAlphaSaturated;
      StateValuesBlend["Blend_BlendFactor"] = plGALBlend::BlendFactor;
      StateValuesBlend["Blend_InvBlendFactor"] = plGALBlend::InvBlendFactor;
    }

    // plGALBlendOp
    {
      StateValuesBlendOp["BlendOp_Add"] = plGALBlendOp::Add;
      StateValuesBlendOp["BlendOp_Subtract"] = plGALBlendOp::Subtract;
      StateValuesBlendOp["BlendOp_RevSubtract"] = plGALBlendOp::RevSubtract;
      StateValuesBlendOp["BlendOp_Min"] = plGALBlendOp::Min;
      StateValuesBlendOp["BlendOp_Max"] = plGALBlendOp::Max;
    }

    // plGALCullMode
    {
      StateValuesCullMode["CullMode_None"] = plGALCullMode::None;
      StateValuesCullMode["CullMode_Front"] = plGALCullMode::Front;
      StateValuesCullMode["CullMode_Back"] = plGALCullMode::Back;
    }

    // plGALCompareFunc
    {
      StateValuesCompareFunc["CompareFunc_Never"] = plGALCompareFunc::Never;
      StateValuesCompareFunc["CompareFunc_Less"] = plGALCompareFunc::Less;
      StateValuesCompareFunc["CompareFunc_Equal"] = plGALCompareFunc::Equal;
      StateValuesCompareFunc["CompareFunc_LessEqual"] = plGALCompareFunc::LessEqual;
      StateValuesCompareFunc["CompareFunc_Greater"] = plGALCompareFunc::Greater;
      StateValuesCompareFunc["CompareFunc_NotEqual"] = plGALCompareFunc::NotEqual;
      StateValuesCompareFunc["CompareFunc_GreaterEqual"] = plGALCompareFunc::GreaterEqual;
      StateValuesCompareFunc["CompareFunc_Always"] = plGALCompareFunc::Always;
    }

    // plGALStencilOp
    {
      StateValuesStencilOp["StencilOp_Keep"] = plGALStencilOp::Keep;
      StateValuesStencilOp["StencilOp_Zero"] = plGALStencilOp::Zero;
      StateValuesStencilOp["StencilOp_Replace"] = plGALStencilOp::Replace;
      StateValuesStencilOp["StencilOp_IncrementSaturated"] = plGALStencilOp::IncrementSaturated;
      StateValuesStencilOp["StencilOp_DecrementSaturated"] = plGALStencilOp::DecrementSaturated;
      StateValuesStencilOp["StencilOp_Invert"] = plGALStencilOp::Invert;
      StateValuesStencilOp["StencilOp_Increment"] = plGALStencilOp::Increment;
      StateValuesStencilOp["StencilOp_Decrement"] = plGALStencilOp::Decrement;
    }
  }

  // Retrieve Blend State
  {
    m_BlendDesc.m_bAlphaToCoverage = GetBoolStateVariable(VariableValues, "AlphaToCoverage", m_BlendDesc.m_bAlphaToCoverage);
    m_BlendDesc.m_bIndependentBlend = GetBoolStateVariable(VariableValues, "IndependentBlend", m_BlendDesc.m_bIndependentBlend);

    plStringBuilder s;

    for (plUInt32 i = 0; i < 8; ++i)
    {
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_bBlendingEnabled = GetBoolStateVariable(
        VariableValues, InsertNumber("BlendingEnabled{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_bBlendingEnabled);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_BlendOp = (plGALBlendOp::Enum)GetEnumStateVariable(
        VariableValues, StateValuesBlendOp, InsertNumber("BlendOp{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_BlendOp);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_BlendOpAlpha = (plGALBlendOp::Enum)GetEnumStateVariable(
        VariableValues, StateValuesBlendOp, InsertNumber("BlendOpAlpha{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_BlendOpAlpha);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_DestBlend = (plGALBlend::Enum)GetEnumStateVariable(
        VariableValues, StateValuesBlend, InsertNumber("DestBlend{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_DestBlend);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_DestBlendAlpha = (plGALBlend::Enum)GetEnumStateVariable(
        VariableValues, StateValuesBlend, InsertNumber("DestBlendAlpha{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_DestBlendAlpha);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_SourceBlend = (plGALBlend::Enum)GetEnumStateVariable(
        VariableValues, StateValuesBlend, InsertNumber("SourceBlend{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_SourceBlend);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_SourceBlendAlpha = (plGALBlend::Enum)GetEnumStateVariable(VariableValues, StateValuesBlend,
        InsertNumber("SourceBlendAlpha{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_SourceBlendAlpha);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_uiWriteMask = static_cast<plUInt8>(GetIntStateVariable(VariableValues, InsertNumber("WriteMask{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_uiWriteMask));
    }
  }

  // Retrieve Rasterizer State
  {
    m_RasterizerDesc.m_bFrontCounterClockwise =
      GetBoolStateVariable(VariableValues, "FrontCounterClockwise", m_RasterizerDesc.m_bFrontCounterClockwise);
    m_RasterizerDesc.m_bScissorTest = GetBoolStateVariable(VariableValues, "ScissorTest", m_RasterizerDesc.m_bScissorTest);
    m_RasterizerDesc.m_bConservativeRasterization =
      GetBoolStateVariable(VariableValues, "ConservativeRasterization", m_RasterizerDesc.m_bConservativeRasterization);
    m_RasterizerDesc.m_bWireFrame = GetBoolStateVariable(VariableValues, "WireFrame", m_RasterizerDesc.m_bWireFrame);
    m_RasterizerDesc.m_CullMode =
      (plGALCullMode::Enum)GetEnumStateVariable(VariableValues, StateValuesCullMode, "CullMode", m_RasterizerDesc.m_CullMode);
    m_RasterizerDesc.m_fDepthBiasClamp = GetFloatStateVariable(VariableValues, "DepthBiasClamp", m_RasterizerDesc.m_fDepthBiasClamp);
    m_RasterizerDesc.m_fSlopeScaledDepthBias =
      GetFloatStateVariable(VariableValues, "SlopeScaledDepthBias", m_RasterizerDesc.m_fSlopeScaledDepthBias);
    m_RasterizerDesc.m_iDepthBias = GetIntStateVariable(VariableValues, "DepthBias", m_RasterizerDesc.m_iDepthBias);
  }

  // Retrieve Depth-Stencil State
  {
    m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp = (plGALStencilOp::Enum)GetEnumStateVariable(
      VariableValues, StateValuesStencilOp, "BackFaceDepthFailOp", m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp);
    m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp = (plGALStencilOp::Enum)GetEnumStateVariable(
      VariableValues, StateValuesStencilOp, "BackFaceFailOp", m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp);
    m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp = (plGALStencilOp::Enum)GetEnumStateVariable(
      VariableValues, StateValuesStencilOp, "BackFacePassOp", m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp);
    m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc = (plGALCompareFunc::Enum)GetEnumStateVariable(
      VariableValues, StateValuesCompareFunc, "BackFaceStencilFunc", m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc);

    m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp = (plGALStencilOp::Enum)GetEnumStateVariable(
      VariableValues, StateValuesStencilOp, "FrontFaceDepthFailOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp);
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp = (plGALStencilOp::Enum)GetEnumStateVariable(
      VariableValues, StateValuesStencilOp, "FrontFaceFailOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp);
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp = (plGALStencilOp::Enum)GetEnumStateVariable(
      VariableValues, StateValuesStencilOp, "FrontFacePassOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp);
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc = (plGALCompareFunc::Enum)GetEnumStateVariable(
      VariableValues, StateValuesCompareFunc, "FrontFaceStencilFunc", m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc);

    m_DepthStencilDesc.m_bDepthTest = GetBoolStateVariable(VariableValues, "DepthTest", m_DepthStencilDesc.m_bDepthTest);
    m_DepthStencilDesc.m_bDepthWrite = GetBoolStateVariable(VariableValues, "DepthWrite", m_DepthStencilDesc.m_bDepthWrite);
    m_DepthStencilDesc.m_bSeparateFrontAndBack =
      GetBoolStateVariable(VariableValues, "SeparateFrontAndBack", m_DepthStencilDesc.m_bSeparateFrontAndBack);
    m_DepthStencilDesc.m_bStencilTest = GetBoolStateVariable(VariableValues, "StencilTest", m_DepthStencilDesc.m_bStencilTest);
    m_DepthStencilDesc.m_DepthTestFunc =
      (plGALCompareFunc::Enum)GetEnumStateVariable(VariableValues, StateValuesCompareFunc, "DepthTestFunc", m_DepthStencilDesc.m_DepthTestFunc);
    m_DepthStencilDesc.m_uiStencilReadMask = static_cast<plUInt8>(GetIntStateVariable(VariableValues, "StencilReadMask", m_DepthStencilDesc.m_uiStencilReadMask));
    m_DepthStencilDesc.m_uiStencilWriteMask = static_cast<plUInt8>(GetIntStateVariable(VariableValues, "StencilWriteMask", m_DepthStencilDesc.m_uiStencilWriteMask));
  }

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  // check for invalid variable names
  {
    for (auto it = VariableValues.GetIterator(); it.IsValid(); ++it)
    {
      if (!s_AllAllowedVariables.Contains(it.Key()))
      {
        plLog::Error("The shader state variable '{0}' does not exist.", it.Key());
      }
    }
  }
#endif


  return PL_SUCCESS;
}


