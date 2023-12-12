#pragma once

#include <Foundation/Strings/String.h>
#include <RendererCore/Declarations.h>

class plPropertyAttribute;

class PLASMA_RENDERERCORE_DLL plShaderParser
{
public:
  struct AttributeDefinition
  {
    plString m_sType;
    plHybridArray<plVariant, 8> m_Values;
  };

  struct ParameterDefinition
  {
    const plRTTI* m_pType = nullptr;
    plString m_sType;
    plString m_sName;

    plHybridArray<AttributeDefinition, 4> m_Attributes;
  };

  struct EnumValue
  {
    plHashedString m_sValueName;
    plInt32 m_iValueValue = 0;
  };

  struct EnumDefinition
  {
    plString m_sName;
    plUInt32 m_uiDefaultValue = 0;
    plHybridArray<EnumValue, 16> m_Values;
  };

  static void ParseMaterialParameterSection(
    plStreamReader& inout_stream, plHybridArray<ParameterDefinition, 16>& out_parameter, plHybridArray<EnumDefinition, 4>& out_enumDefinitions);

  static void ParsePermutationSection(
    plStreamReader& inout_stream, plHybridArray<plHashedString, 16>& out_permVars, plHybridArray<plPermutationVar, 16>& out_fixedPermVars);
  static void ParsePermutationSection(
    plStringView sPermutationSection, plHybridArray<plHashedString, 16>& out_permVars, plHybridArray<plPermutationVar, 16>& out_fixedPermVars);

  static void ParsePermutationVarConfig(plStringView sPermutationVarConfig, plVariant& out_defaultValue, EnumDefinition& out_enumDefinition);
};
