#pragma once

#include <Foundation/Strings/String.h>
#include <RendererCore/ShaderCompiler/Declarations.h>
#include <RendererFoundation/Shader/ShaderByteCode.h>

class plPropertyAttribute;

class PL_RENDERERCORE_DLL plShaderParser
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



  /// \brief Tries to find shader resource declarations inside the shader source.
  ///
  /// Used by the shader compiler implementations to generate resource mappings to sets/slots without creating conflicts across shader stages. For a list of supported resource declarations and possible pitfalls, please refer to https://plengine.net/pages/docs/graphics/shaders/shader-resources.html.
  /// \param sShaderStageSource The shader source to parse.
  /// \param out_Resources The shader resources found inside the source.
  static void ParseShaderResources(plStringView sShaderStageSource, plDynamicArray<plShaderResourceDefinition>& out_resources);

  /// \brief Delegate to creates a new declaration and register binding for a specific shader plShaderResourceDefinition.
  /// \param sPlatform The platform for which the shader is being compiled. Will be one of the values returned by GetSupportedPlatforms.
  /// \param sDeclaration The shader resource declaration without any attributes, e.g. "Texture2D DiffuseTexture"
  /// \param binding The binding that needs to be set on the output out_sDeclaration.
  /// \param out_sDeclaration The new declaration that changes sDeclaration according to the provided 'binding', e.g. "Texture2D DiffuseTexture : register(t0, space5)"
  using CreateResourceDeclaration = plDelegate<void (plStringView, plStringView, const plShaderResourceBinding &, plStringBuilder &)>;

  /// \brief Merges the shader resource bindings of all used shader stages.
  ///
  /// The function can fail if a shader resource of the same name has different signatures in two stages. E.g. the type, slot or set is different. Shader resources must be uniquely identified via name.
  /// \param spd The shader currently being processed.
  /// \param out_bindings A hashmap from shader resource name to shader resource binding. If a binding is used in multiple stages, plShaderResourceBinding::m_Stages will be the combination of all used stages.
  /// \param pLog Log interface to write errors to.
  /// \return Returns failure if the shader stages could not be merged.
  static plResult MergeShaderResourceBindings(const plShaderProgramData& spd, plHashTable<plHashedString, plShaderResourceBinding>& out_bindings, plLogInterface* pLog);

  /// \brief Makes sure that bindings fulfills the basic requirements that plEngine has for resource bindings in a shader, e.g. that each binding has a set / slot set.
  static plResult SanityCheckShaderResourceBindings(const plHashTable<plHashedString, plShaderResourceBinding>& bindings, plLogInterface* pLog);

  /// \brief Creates a new shader source code that patches all shader resources to contain fixed set / slot bindings.
  /// \param sPlatform The platform for which the shader should be patched.
  /// \param sShaderStageSource The original shader source code that should be patched.
  /// \param resources A list of all shader resources that need to be patched within sShaderStageSource.
  /// \param bindings The binding information that each shader resource should have after patching. These bindings must have unique set / slots combinations for each resource.
  /// \param createDeclaration The callback to be called to generate the new shader resource declaration.
  /// \param out_shaderStageSource The new shader source code after patching.
  static void ApplyShaderResourceBindings(plStringView sPlatform, plStringView sShaderStageSource, const plDynamicArray<plShaderResourceDefinition>& resources, const plHashTable<plHashedString, plShaderResourceBinding>& bindings, const CreateResourceDeclaration& createDeclaration, plStringBuilder& out_sShaderStageSource);
};
