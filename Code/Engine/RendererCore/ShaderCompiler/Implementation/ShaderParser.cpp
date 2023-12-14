#include <RendererCore/RendererCorePCH.h>

#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

using namespace plTokenParseUtils;

namespace
{
  static plHashTable<plStringView, const plRTTI*> s_NameToTypeTable;
  static plHashTable<plStringView, plEnum<plGALShaderDescriptorType>> s_NameToDescriptorTable;
  static plHashTable<plStringView, plEnum<plGALShaderTextureType>> s_NameToTextureTable;

  void InitializeTables()
  {
    if (!s_NameToTypeTable.IsEmpty())
      return;

    s_NameToTypeTable.Insert("float", plGetStaticRTTI<float>());
    s_NameToTypeTable.Insert("float2", plGetStaticRTTI<plVec2>());
    s_NameToTypeTable.Insert("float3", plGetStaticRTTI<plVec3>());
    s_NameToTypeTable.Insert("float4", plGetStaticRTTI<plVec4>());
    s_NameToTypeTable.Insert("int", plGetStaticRTTI<int>());
    s_NameToTypeTable.Insert("int2", plGetStaticRTTI<plVec2I32>());
    s_NameToTypeTable.Insert("int3", plGetStaticRTTI<plVec3I32>());
    s_NameToTypeTable.Insert("int4", plGetStaticRTTI<plVec4I32>());
    s_NameToTypeTable.Insert("uint", plGetStaticRTTI<plUInt32>());
    s_NameToTypeTable.Insert("uint2", plGetStaticRTTI<plVec2U32>());
    s_NameToTypeTable.Insert("uint3", plGetStaticRTTI<plVec3U32>());
    s_NameToTypeTable.Insert("uint4", plGetStaticRTTI<plVec4U32>());
    s_NameToTypeTable.Insert("bool", plGetStaticRTTI<bool>());
    s_NameToTypeTable.Insert("Color", plGetStaticRTTI<plColor>());
    /// \todo Are we going to support linear UB colors ?
    s_NameToTypeTable.Insert("Texture2D", plGetStaticRTTI<plString>());
    s_NameToTypeTable.Insert("Texture3D", plGetStaticRTTI<plString>());
    s_NameToTypeTable.Insert("TextureCube", plGetStaticRTTI<plString>());

    s_NameToDescriptorTable.Insert("cbuffer"_plsv, plGALShaderDescriptorType::ConstantBuffer);
    s_NameToDescriptorTable.Insert("ConstantBuffer"_plsv, plGALShaderDescriptorType::ConstantBuffer);
    s_NameToDescriptorTable.Insert("SamplerState"_plsv, plGALShaderDescriptorType::Sampler);
    s_NameToDescriptorTable.Insert("SamplerComparisonState"_plsv, plGALShaderDescriptorType::Sampler);
    s_NameToDescriptorTable.Insert("Texture1D"_plsv, plGALShaderDescriptorType::Texture);
    s_NameToDescriptorTable.Insert("Texture1DArray"_plsv, plGALShaderDescriptorType::Texture);
    s_NameToDescriptorTable.Insert("Texture2D"_plsv, plGALShaderDescriptorType::Texture);
    s_NameToDescriptorTable.Insert("Texture2DArray"_plsv, plGALShaderDescriptorType::Texture);
    s_NameToDescriptorTable.Insert("Texture2DMS"_plsv, plGALShaderDescriptorType::Texture);
    s_NameToDescriptorTable.Insert("Texture2DMSArray"_plsv, plGALShaderDescriptorType::Texture);
    s_NameToDescriptorTable.Insert("Texture3D"_plsv, plGALShaderDescriptorType::Texture);
    s_NameToDescriptorTable.Insert("TextureCube"_plsv, plGALShaderDescriptorType::Texture);
    s_NameToDescriptorTable.Insert("TextureCubeArray"_plsv, plGALShaderDescriptorType::Texture);
    s_NameToDescriptorTable.Insert("Buffer"_plsv, plGALShaderDescriptorType::TexelBuffer);
    s_NameToDescriptorTable.Insert("StructuredBuffer"_plsv, plGALShaderDescriptorType::StructuredBuffer);
    s_NameToDescriptorTable.Insert("ByteAddressBuffer"_plsv, plGALShaderDescriptorType::StructuredBuffer);
    s_NameToDescriptorTable.Insert("RWTexture1D"_plsv, plGALShaderDescriptorType::TextureRW);
    s_NameToDescriptorTable.Insert("RWTexture1DArray"_plsv, plGALShaderDescriptorType::TextureRW);
    s_NameToDescriptorTable.Insert("RWTexture2D"_plsv, plGALShaderDescriptorType::TextureRW);
    s_NameToDescriptorTable.Insert("RWTexture2DArray"_plsv, plGALShaderDescriptorType::TextureRW);
    s_NameToDescriptorTable.Insert("RWTexture3D"_plsv, plGALShaderDescriptorType::TextureRW);
    s_NameToDescriptorTable.Insert("RWBuffer"_plsv, plGALShaderDescriptorType::TexelBufferRW);
    s_NameToDescriptorTable.Insert("RWStructuredBuffer"_plsv, plGALShaderDescriptorType::StructuredBufferRW);
    s_NameToDescriptorTable.Insert("RWByteAddressBuffer"_plsv, plGALShaderDescriptorType::StructuredBufferRW);
    s_NameToDescriptorTable.Insert("AppendStructuredBuffer"_plsv, plGALShaderDescriptorType::StructuredBufferRW);
    s_NameToDescriptorTable.Insert("ConsumeStructuredBuffer"_plsv, plGALShaderDescriptorType::StructuredBufferRW);
    
    s_NameToTextureTable.Insert("Texture1D"_plsv, plGALShaderTextureType::Texture1D);
    s_NameToTextureTable.Insert("Texture1DArray"_plsv, plGALShaderTextureType::Texture1DArray);
    s_NameToTextureTable.Insert("Texture2D"_plsv, plGALShaderTextureType::Texture2D);
    s_NameToTextureTable.Insert("Texture2DArray"_plsv, plGALShaderTextureType::Texture2DArray);
    s_NameToTextureTable.Insert("Texture2DMS"_plsv, plGALShaderTextureType::Texture2DMS);
    s_NameToTextureTable.Insert("Texture2DMSArray"_plsv, plGALShaderTextureType::Texture2DMSArray);
    s_NameToTextureTable.Insert("Texture3D"_plsv, plGALShaderTextureType::Texture3D);
    s_NameToTextureTable.Insert("TextureCube"_plsv, plGALShaderTextureType::TextureCube);
    s_NameToTextureTable.Insert("TextureCubeArray"_plsv, plGALShaderTextureType::TextureCubeArray);
    s_NameToTextureTable.Insert("RWTexture1D"_plsv, plGALShaderTextureType::Texture1D);
    s_NameToTextureTable.Insert("RWTexture1DArray"_plsv, plGALShaderTextureType::Texture1DArray);
    s_NameToTextureTable.Insert("RWTexture2D"_plsv, plGALShaderTextureType::Texture2D);
    s_NameToTextureTable.Insert("RWTexture2DArray"_plsv, plGALShaderTextureType::Texture2DArray);
    s_NameToTextureTable.Insert("RWTexture3D"_plsv, plGALShaderTextureType::Texture3D);
  }

  const plRTTI* GetType(const char* szType)
  {
    InitializeTables();

    const plRTTI* pType = nullptr;
    s_NameToTypeTable.TryGetValue(szType, pType);
    return pType;
  }

  plVariant ParseValue(const TokenStream& tokens, plUInt32& ref_uiCurToken)
  {
    plUInt32 uiValueToken = ref_uiCurToken;

    if (Accept(tokens, ref_uiCurToken, plTokenType::String1, &uiValueToken) || Accept(tokens, ref_uiCurToken, plTokenType::String2, &uiValueToken))
    {
      plStringBuilder sValue = tokens[uiValueToken]->m_DataView;
      sValue.Trim("\"'");

      return plVariant(sValue.GetData());
    }

    if (Accept(tokens, ref_uiCurToken, plTokenType::Integer, &uiValueToken))
    {
      plString sValue = tokens[uiValueToken]->m_DataView;

      plInt64 iValue = 0;
      if (sValue.StartsWith_NoCase("0x"))
      {
        plUInt32 uiValue32 = 0;
        plConversionUtils::ConvertHexStringToUInt32(sValue, uiValue32).IgnoreResult();

        iValue = uiValue32;
      }
      else
      {
        plConversionUtils::StringToInt64(sValue, iValue).IgnoreResult();
      }

      return plVariant(iValue);
    }

    if (Accept(tokens, ref_uiCurToken, plTokenType::Float, &uiValueToken))
    {
      plString sValue = tokens[uiValueToken]->m_DataView;

      double fValue = 0;
      plConversionUtils::StringToFloat(sValue, fValue).IgnoreResult();

      return plVariant(fValue);
    }

    if (Accept(tokens, ref_uiCurToken, "true", &uiValueToken) || Accept(tokens, ref_uiCurToken, "false", &uiValueToken))
    {
      bool bValue = tokens[uiValueToken]->m_DataView == "true";
      return plVariant(bValue);
    }

    auto& dataView = tokens[ref_uiCurToken]->m_DataView;
    if (tokens[ref_uiCurToken]->m_iType == plTokenType::Identifier && plStringUtils::IsValidIdentifierName(dataView.GetStartPointer(), dataView.GetEndPointer()))
    {
      // complex type constructor
      const plRTTI* pType = nullptr;
      if (!s_NameToTypeTable.TryGetValue(dataView, pType))
      {
        plLog::Error("Invalid type name '{}'", dataView);
        return plVariant();
      }

      ++ref_uiCurToken;
      Accept(tokens, ref_uiCurToken, "(");

      plHybridArray<plVariant, 8> constructorArgs;

      while (!Accept(tokens, ref_uiCurToken, ")"))
      {
        plVariant value = ParseValue(tokens, ref_uiCurToken);
        if (value.IsValid())
        {
          constructorArgs.PushBack(value);
        }
        else
        {
          plLog::Error("Invalid arguments for constructor '{}'", pType->GetTypeName());
          return PLASMA_FAILURE;
        }

        Accept(tokens, ref_uiCurToken, ",");
      }

      // find matching constructor
      auto functions = pType->GetFunctions();
      for (auto pFunc : functions)
      {
        if (pFunc->GetFunctionType() == plFunctionType::Constructor && pFunc->GetArgumentCount() == constructorArgs.GetCount())
        {
          plHybridArray<plVariant, 8> convertedArgs;
          bool bAllArgsValid = true;

          for (plUInt32 uiArg = 0; uiArg < pFunc->GetArgumentCount(); ++uiArg)
          {
            const plRTTI* pArgType = pFunc->GetArgumentType(uiArg);
            plResult conversionResult = PLASMA_FAILURE;
            convertedArgs.PushBack(constructorArgs[uiArg].ConvertTo(pArgType->GetVariantType(), &conversionResult));
            if (conversionResult.Failed())
            {
              bAllArgsValid = false;
              break;
            }
          }

          if (bAllArgsValid)
          {
            plVariant result;
            pFunc->Execute(nullptr, convertedArgs, result);

            if (result.IsValid())
            {
              return result;
            }
          }
        }
      }
    }

    return plVariant();
  }

  plResult ParseAttribute(const TokenStream& tokens, plUInt32& ref_uiCurToken, plShaderParser::ParameterDefinition& out_parameterDefinition)
  {
    if (!Accept(tokens, ref_uiCurToken, "@"))
    {
      return PLASMA_FAILURE;
    }

    plUInt32 uiTypeToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, plTokenType::Identifier, &uiTypeToken))
    {
      return PLASMA_FAILURE;
    }

    plShaderParser::AttributeDefinition& attributeDef = out_parameterDefinition.m_Attributes.ExpandAndGetRef();
    attributeDef.m_sType = tokens[uiTypeToken]->m_DataView;

    Accept(tokens, ref_uiCurToken, "(");

    while (!Accept(tokens, ref_uiCurToken, ")"))
    {
      plVariant value = ParseValue(tokens, ref_uiCurToken);
      if (value.IsValid())
      {
        attributeDef.m_Values.PushBack(value);
      }
      else
      {
        plLog::Error("Invalid arguments for attribute '{}'", attributeDef.m_sType);
        return PLASMA_FAILURE;
      }

      Accept(tokens, ref_uiCurToken, ",");
    }

    return PLASMA_SUCCESS;
  }

  plResult ParseParameter(const TokenStream& tokens, plUInt32& ref_uiCurToken, plShaderParser::ParameterDefinition& out_parameterDefinition)
  {
    plUInt32 uiTypeToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, plTokenType::Identifier, &uiTypeToken))
    {
      return PLASMA_FAILURE;
    }

    out_parameterDefinition.m_sType = tokens[uiTypeToken]->m_DataView;
    out_parameterDefinition.m_pType = GetType(out_parameterDefinition.m_sType);

    plUInt32 uiNameToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, plTokenType::Identifier, &uiNameToken))
    {
      return PLASMA_FAILURE;
    }

    out_parameterDefinition.m_sName = tokens[uiNameToken]->m_DataView;

    while (!Accept(tokens, ref_uiCurToken, ";"))
    {
      if (ParseAttribute(tokens, ref_uiCurToken, out_parameterDefinition).Failed())
      {
        return PLASMA_FAILURE;
      }
    }

    return PLASMA_SUCCESS;
  }

  plResult ParseEnum(const TokenStream& tokens, plUInt32& ref_uiCurToken, plShaderParser::EnumDefinition& out_enumDefinition, bool bCheckPrefix)
  {
    if (!Accept(tokens, ref_uiCurToken, "enum"))
    {
      return PLASMA_FAILURE;
    }

    plUInt32 uiNameToken = ref_uiCurToken;
    if (!Accept(tokens, ref_uiCurToken, plTokenType::Identifier, &uiNameToken))
    {
      return PLASMA_FAILURE;
    }

    out_enumDefinition.m_sName = tokens[uiNameToken]->m_DataView;
    plStringBuilder sEnumPrefix(out_enumDefinition.m_sName, "_");

    if (!Accept(tokens, ref_uiCurToken, "{"))
    {
      plLog::Error("Opening bracket expected for enum definition.");
      return PLASMA_FAILURE;
    }

    plUInt32 uiDefaultValue = 0;
    plUInt32 uiCurrentValue = 0;

    while (true)
    {
      plUInt32 uiValueNameToken = ref_uiCurToken;
      if (!Accept(tokens, ref_uiCurToken, plTokenType::Identifier, &uiValueNameToken))
      {
        return PLASMA_FAILURE;
      }

      plStringView sValueName = tokens[uiValueNameToken]->m_DataView;

      if (Accept(tokens, ref_uiCurToken, "="))
      {
        plUInt32 uiValueToken = ref_uiCurToken;
        Accept(tokens, ref_uiCurToken, plTokenType::Integer, &uiValueToken);

        plInt32 iValue = 0;
        if (plConversionUtils::StringToInt(tokens[uiValueToken]->m_DataView.GetStartPointer(), iValue).Succeeded() && iValue >= 0)
        {
          uiCurrentValue = iValue;
        }
        else
        {
          plLog::Error("Invalid enum value '{0}'. Only positive numbers are allowed.", tokens[uiValueToken]->m_DataView);
        }
      }

      if (sValueName.IsEqual_NoCase("default"))
      {
        uiDefaultValue = uiCurrentValue;
      }
      else
      {
        if (bCheckPrefix && !sValueName.StartsWith(sEnumPrefix))
        {
          plLog::Error("Enum value does not start with the expected enum name as prefix: '{0}'", sEnumPrefix);
        }

        auto& ev = out_enumDefinition.m_Values.ExpandAndGetRef();

        const plStringBuilder sFinalName = sValueName;
        ev.m_sValueName.Assign(sFinalName.GetData());
        ev.m_iValueValue = static_cast<plInt32>(uiCurrentValue);
      }

      if (Accept(tokens, ref_uiCurToken, ","))
      {
        ++uiCurrentValue;
      }
      else
      {
        break;
      }

      if (Accept(tokens, ref_uiCurToken, "}"))
        goto after_braces;
    }

    if (!Accept(tokens, ref_uiCurToken, "}"))
    {
      plLog::Error("Closing bracket expected for enum definition.");
      return PLASMA_FAILURE;
    }

  after_braces:

    out_enumDefinition.m_uiDefaultValue = uiDefaultValue;

    Accept(tokens, ref_uiCurToken, ";");

    return PLASMA_SUCCESS;
  }

  void SkipWhitespace(plStringView& s)
  {
    while (s.IsValid() && plStringUtils::IsWhiteSpace(s.GetCharacter()))
    {
      ++s;
    }
  }
} // namespace

// static
void plShaderParser::ParseMaterialParameterSection(plStreamReader& inout_stream, plHybridArray<ParameterDefinition, 16>& out_parameter, plHybridArray<EnumDefinition, 4>& out_enumDefinitions)
{
  plString sContent;
  sContent.ReadAll(inout_stream);

  plShaderHelper::plTextSectionizer Sections;
  plShaderHelper::GetShaderSections(sContent.GetData(), Sections);

  plUInt32 uiFirstLine = 0;
  plStringView s = Sections.GetSectionContent(plShaderHelper::plShaderSections::MATERIALPARAMETER, uiFirstLine);

  plTokenizer tokenizer;
  tokenizer.Tokenize(plArrayPtr<const plUInt8>((const plUInt8*)s.GetStartPointer(), s.GetElementCount()), plLog::GetThreadLocalLogSystem());

  TokenStream tokens;
  tokenizer.GetAllLines(tokens);

  plUInt32 uiCurToken = 0;

  while (!Accept(tokens, uiCurToken, plTokenType::EndOfFile))
  {
    EnumDefinition enumDef;
    if (ParseEnum(tokens, uiCurToken, enumDef, false).Succeeded())
    {
      PLASMA_ASSERT_DEV(!enumDef.m_sName.IsEmpty(), "");

      out_enumDefinitions.PushBack(std::move(enumDef));
      continue;
    }

    ParameterDefinition paramDef;
    if (ParseParameter(tokens, uiCurToken, paramDef).Succeeded())
    {
      out_parameter.PushBack(std::move(paramDef));
      continue;
    }

    plLog::Error("Invalid token in material parameter section '{}'", tokens[uiCurToken]->m_DataView);
    break;
  }
}

// static
void plShaderParser::ParsePermutationSection(plStreamReader& inout_stream, plHybridArray<plHashedString, 16>& out_permVars, plHybridArray<plPermutationVar, 16>& out_fixedPermVars)
{
  plString sContent;
  sContent.ReadAll(inout_stream);

  plShaderHelper::plTextSectionizer Sections;
  plShaderHelper::GetShaderSections(sContent.GetData(), Sections);

  plUInt32 uiFirstLine = 0;
  plStringView sPermutations = Sections.GetSectionContent(plShaderHelper::plShaderSections::PERMUTATIONS, uiFirstLine);
  ParsePermutationSection(sPermutations, out_permVars, out_fixedPermVars);
}

// static
void plShaderParser::ParsePermutationSection(plStringView s, plHybridArray<plHashedString, 16>& out_permVars, plHybridArray<plPermutationVar, 16>& out_fixedPermVars)
{
  out_permVars.Clear();
  out_fixedPermVars.Clear();

  plTokenizer tokenizer;
  tokenizer.Tokenize(plArrayPtr<const plUInt8>((const plUInt8*)s.GetStartPointer(), s.GetElementCount()), plLog::GetThreadLocalLogSystem());

  enum class State
  {
    Idle,
    HasName,
    HasEqual,
    HasValue
  };

  State state = State::Idle;
  plStringBuilder sToken, sVarName;

  for (const auto& token : tokenizer.GetTokens())
  {
    if (token.m_iType == plTokenType::Whitespace || token.m_iType == plTokenType::BlockComment || token.m_iType == plTokenType::LineComment)
      continue;

    if (token.m_iType == plTokenType::String1 || token.m_iType == plTokenType::String2 || token.m_iType == plTokenType::RawString1)
    {
      sToken = token.m_DataView;
      plLog::Error("Strings are not allowed in the permutation section: '{0}'", sToken);
      return;
    }

    if (token.m_iType == plTokenType::Newline || token.m_iType == plTokenType::EndOfFile)
    {
      if (state == State::HasEqual)
      {
        plLog::Error("Missing assignment value in permutation section");
        return;
      }

      if (state == State::HasName)
      {
        out_permVars.ExpandAndGetRef().Assign(sVarName.GetData());
      }

      state = State::Idle;
      continue;
    }

    sToken = token.m_DataView;

    if (token.m_iType == plTokenType::NonIdentifier)
    {
      if (sToken == "=" && state == State::HasName)
      {
        state = State::HasEqual;
        continue;
      }
    }
    else if (token.m_iType == plTokenType::Identifier)
    {
      if (state == State::Idle)
      {
        sVarName = sToken;
        state = State::HasName;
        continue;
      }

      if (state == State::HasEqual)
      {
        auto& res = out_fixedPermVars.ExpandAndGetRef();
        res.m_sName.Assign(sVarName.GetData());
        res.m_sValue.Assign(sToken.GetData());
        state = State::HasValue;
        continue;
      }
    }

    plLog::Error("Invalid permutation section at token '{0}'", sToken);
  }
}

// static
void plShaderParser::ParsePermutationVarConfig(plStringView s, plVariant& out_defaultValue, EnumDefinition& out_enumDefinition)
{
  SkipWhitespace(s);

  plStringBuilder name;

  if (s.StartsWith("bool"))
  {
    bool bDefaultValue = false;

    const char* szDefaultValue = s.FindSubString("=");
    if (szDefaultValue != nullptr)
    {
      name.SetSubString_FromTo(s.GetStartPointer() + 4, szDefaultValue);

      ++szDefaultValue;
      plConversionUtils::StringToBool(szDefaultValue, bDefaultValue).IgnoreResult();
    }
    else
    {
      name.SetSubString_FromTo(s.GetStartPointer() + 4, s.GetEndPointer());
    }

    name.Trim(" \t\r\n");
    out_enumDefinition.m_sName = name;
    out_defaultValue = bDefaultValue;
  }
  else if (s.StartsWith("enum"))
  {
    plTokenizer tokenizer;
    tokenizer.Tokenize(plArrayPtr<const plUInt8>((const plUInt8*)s.GetStartPointer(), s.GetElementCount()), plLog::GetThreadLocalLogSystem());

    TokenStream tokens;
    tokenizer.GetAllLines(tokens);

    plUInt32 uiCurToken = 0;
    if (ParseEnum(tokens, uiCurToken, out_enumDefinition, true).Failed())
    {
      plLog::Error("Invalid enum PermutationVar definition.");
    }
    else
    {
      PLASMA_ASSERT_DEV(!out_enumDefinition.m_sName.IsEmpty(), "");

      out_defaultValue = out_enumDefinition.m_uiDefaultValue;
    }
  }
  else
  {
    plLog::Error("Unknown permutation var type");
  }
}

plResult ParseResource(plArrayPtr<const plUInt8> sShaderStageSource, const TokenStream& tokens, plUInt32& ref_uiCurToken, plShaderParser::ResourceDefinition& out_resourceDefinition)
{
  // Match type
  plUInt32 uiTypeToken = ref_uiCurToken;
  if (!Accept(tokens, ref_uiCurToken, plTokenType::Identifier, &uiTypeToken))
  {
    return PLASMA_FAILURE;
  }
  if (!s_NameToDescriptorTable.TryGetValue(tokens[uiTypeToken]->m_DataView, out_resourceDefinition.m_Binding.m_DescriptorType))
    return PLASMA_FAILURE;
  s_NameToTextureTable.TryGetValue(tokens[uiTypeToken]->m_DataView, out_resourceDefinition.m_Binding.m_TextureType);

  // Skip optional template
  TokenMatch templatePattern[] = {"<"_plsv,plTokenType::Identifier, ">"_plsv};
  plHybridArray<plUInt32, 8> acceptedTokens;
  Accept(tokens, ref_uiCurToken, templatePattern, &acceptedTokens);
  
  // Match name
  plUInt32 uiNameToken = ref_uiCurToken;
  if (!Accept(tokens, ref_uiCurToken, plTokenType::Identifier, &uiNameToken))
  {
    return PLASMA_FAILURE;
  }
  out_resourceDefinition.m_Binding.m_sName.Assign(tokens[uiNameToken]->m_DataView);
  plUInt32 uiEndToken = uiNameToken;

  // Match optional array
  TokenMatch arrayPattern[] = {"["_plsv,plTokenType::Integer, "]"_plsv};
  TokenMatch bindlessPattern[] = {"["_plsv, "]"_plsv};
  if (Accept(tokens, ref_uiCurToken, arrayPattern, &acceptedTokens))
  {
    plConversionUtils::StringToUInt(tokens[acceptedTokens[1]]->m_DataView, out_resourceDefinition.m_Binding.m_uiArraySize).AssertSuccess("Tokenizer error");
    uiEndToken = acceptedTokens.PeekBack();
  }
  else if (Accept(tokens, ref_uiCurToken, bindlessPattern, &acceptedTokens))
  {
    out_resourceDefinition.m_Binding.m_uiArraySize = 0;
    uiEndToken = acceptedTokens.PeekBack();
  }
  out_resourceDefinition.m_sDeclaration = plStringView(tokens[uiTypeToken]->m_DataView.GetStartPointer(), tokens[uiEndToken]->m_DataView.GetEndPointer());

  // Match optional register
  TokenMatch slotPattern[] = {":"_plsv,"register"_plsv, "("_plsv,plTokenType::Identifier, ")"_plsv};
  TokenMatch slotAndSetPattern[] = {":"_plsv,"register"_plsv, "("_plsv,plTokenType::Identifier, ","_plsv, plTokenType::Identifier, ")"_plsv};
  if (Accept(tokens, ref_uiCurToken, slotPattern, &acceptedTokens))
  {
    plStringView sSlot = tokens[acceptedTokens[3]]->m_DataView;
    sSlot.Trim("tsub");
    plInt32 iSlot;
    plConversionUtils::StringToInt(sSlot, iSlot).AssertSuccess("Failed to parse slot index of shader resource");
    out_resourceDefinition.m_Binding.m_iSlot = static_cast<plInt16>(iSlot);
    uiEndToken = acceptedTokens.PeekBack();
  }
  else if (Accept(tokens, ref_uiCurToken, slotAndSetPattern, &acceptedTokens))
  {
    plStringView sSlot = tokens[acceptedTokens[3]]->m_DataView;
    sSlot.Trim("tsub");
    plInt32 iSlot;
    plConversionUtils::StringToInt(sSlot, iSlot).AssertSuccess("Failed to parse slot index of shader resource");
    out_resourceDefinition.m_Binding.m_iSlot = static_cast<plInt16>(iSlot);

    plStringView sSet = tokens[acceptedTokens[5]]->m_DataView;
    sSet.TrimWordStart("space"_plsv);
    plInt32 iSet;
    plConversionUtils::StringToInt(sSet, iSet).AssertSuccess("Failed to parse set index of shader resource");
    out_resourceDefinition.m_Binding.m_iSet = static_cast<plInt16>(iSet);
    uiEndToken = acceptedTokens.PeekBack();
  }

  out_resourceDefinition.m_sDeclarationAndRegister = plStringView(tokens[uiTypeToken]->m_DataView.GetStartPointer(), tokens[uiEndToken]->m_DataView.GetEndPointer());
  // Match ; (resource declaration done) or { (constant buffer member declaration starts)
  if (!Accept(tokens, ref_uiCurToken, ";"_plsv) && !Accept(tokens, ref_uiCurToken, "{"_plsv))
    return PLASMA_FAILURE;

  return PLASMA_SUCCESS;
}

void plShaderParser::ParseShaderResources(plStringView sShaderStageSource, plDynamicArray<plShaderParser::ResourceDefinition>& out_Resources)
{
  InitializeTables();

  plTokenizer tokenizer;
  tokenizer.SetTreatHashSignAsLineComment(true);
  tokenizer.TokenizeReference(plArrayPtr<const plUInt8>((const plUInt8*)sShaderStageSource.GetStartPointer(), sShaderStageSource.GetElementCount()), plLog::GetThreadLocalLogSystem());

  TokenStream tokens;
  tokenizer.GetAllLines(tokens);

  plUInt32 uiCurToken = 0;

  while (!Accept(tokens, uiCurToken, plTokenType::EndOfFile))
  {
    ResourceDefinition resourceDef;
    if (ParseResource(tokenizer.GetData(), tokens, uiCurToken, resourceDef).Succeeded())
    {
      out_Resources.PushBack(std::move(resourceDef));
      continue;
    }
    ++uiCurToken;
  }
}

void plShaderParser::ApplyShaderResourceBindings(plStringView sShaderStageSource, const plDynamicArray<ResourceDefinition>& resources, const plHashTable<plHashedString, plShaderResourceBinding>& bindings, const CreateResourceDeclaration& createDeclaration, plStringBuilder& out_shaderStageSource)
{
  plDeque<plString> partStorage;
  plHybridArray<plStringView, 16> parts;

  plStringBuilder sDeclaration;
  const char* szStart = sShaderStageSource.GetStartPointer();
  for (int i = 0; i < resources.GetCount(); ++i)
  {
    parts.PushBack(plStringView(szStart, resources[i].m_sDeclarationAndRegister.GetStartPointer()));

    plShaderResourceBinding* pBinding = nullptr;
    PLASMA_ASSERT_DEV(bindings.TryGetValue(resources[i].m_Binding.m_sName, pBinding), "Every resource should be present in the map.");
    PLASMA_ASSERT_DEV(pBinding->m_iSlot >= 0 && pBinding->m_iSet >= 0, "Unbound shader resource binding found: '{}', slot: {}, set: {}", pBinding->m_sName, pBinding->m_iSlot, pBinding->m_iSet);
    createDeclaration(resources[i].m_sDeclaration, *pBinding, sDeclaration);
    plString& sStorage = partStorage.ExpandAndGetRef();
    sStorage = sDeclaration;
    parts.PushBack(sStorage);
    szStart = resources[i].m_sDeclarationAndRegister.GetEndPointer();
  }
  parts.PushBack(plStringView(szStart, sShaderStageSource.GetEndPointer()));

  plUInt32 uiSize = 0;
  for (const plStringView& sPart : parts)
    uiSize += sPart.GetElementCount();

  out_shaderStageSource.Clear();
  out_shaderStageSource.Reserve(uiSize);

  for (const plStringView& sPart : parts)
  {
    out_shaderStageSource.Append(sPart);
  }
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderParser);
