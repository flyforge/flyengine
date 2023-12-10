#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualShader/VsCodeGenerator.h>

static plString ToShaderString(const plVariant& value)
{
  plStringBuilder temp;

  switch (value.GetType())
  {
    case plVariantType::String:
    {
      temp = value.Get<plString>();
    }
    break;

    case plVariantType::Color:
    case plVariantType::ColorGamma:
    {
      plColor v = value.ConvertTo<plColor>();
      temp.Format("float4({0}, {1}, {2}, {3})", v.r, v.g, v.b, v.a);
    }
    break;

    case plVariantType::Vector4:
    {
      plVec4 v = value.Get<plVec4>();
      temp.Format("float4({0}, {1}, {2}, {3})", v.x, v.y, v.z, v.w);
    }
    break;

    case plVariantType::Vector3:
    {
      plVec3 v = value.Get<plVec3>();
      temp.Format("float3({0}, {1}, {2})", v.x, v.y, v.z);
    }
    break;

    case plVariantType::Vector2:
    {
      plVec2 v = value.Get<plVec2>();
      temp.Format("float2({0}, {1})", v.x, v.y);
    }
    break;

    case plVariantType::Float:
    case plVariantType::Int32:
    case plVariantType::Bool:
    {
      temp.Format("{0}", value);
    }
    break;

    case plVariantType::Time:
    {
      float v = value.Get<plTime>().GetSeconds();
      temp.Format("{0}", v);
    }
    break;

    case plVariantType::Angle:
    {
      float v = value.Get<plAngle>().GetRadian();
      temp.Format("{0}", v);
    }
    break;

    default:
      temp = "<Invalid Type>";
      break;
  }

  return temp;
}

plVisualShaderCodeGenerator::plVisualShaderCodeGenerator()
{
  m_pNodeManager = nullptr;
  m_pTypeRegistry = nullptr;
  m_pNodeBaseRtti = nullptr;
  m_pMainNode = nullptr;
}

void plVisualShaderCodeGenerator::DetermineConfigFileDependencies(const plDocumentNodeManager* pNodeManager, plSet<plString>& out_cfgFiles)
{
  out_cfgFiles.Clear();

  m_pNodeManager = pNodeManager;
  m_pTypeRegistry = plVisualShaderTypeRegistry::GetSingleton();
  m_pNodeBaseRtti = m_pTypeRegistry->GetNodeBaseType();

  if (GatherAllNodes(pNodeManager->GetRootObject()).Failed())
    return;

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto pDesc = m_pTypeRegistry->GetDescriptorForType(it.Key()->GetType());

    out_cfgFiles.Insert(pDesc->m_sCfgFile);
  }
}

plStatus plVisualShaderCodeGenerator::GatherAllNodes(const plDocumentObject* pRootObj)
{
  if (pRootObj->GetType()->IsDerivedFrom(m_pNodeBaseRtti))
  {
    NodeState& ns = m_Nodes[pRootObj];
    ns.m_uiNodeId = m_Nodes.GetCount(); // ID 0 is reserved
    ns.m_bCodeGenerated = false;
    ns.m_bInProgress = false;

    auto pDesc = m_pTypeRegistry->GetDescriptorForType(pRootObj->GetType());

    if (pDesc == nullptr)
      return plStatus("Node type of root node is unknown");

    if (pDesc->m_NodeType == plVisualShaderNodeType::Main)
    {
      if (m_pMainNode != nullptr)
        return plStatus("Shader has multiple output nodes");

      m_pMainNode = pRootObj;
    }
  }

  const auto& children = pRootObj->GetChildren();
  for (plUInt32 i = 0; i < children.GetCount(); ++i)
  {
    PLASMA_SUCCEED_OR_RETURN(GatherAllNodes(children[i]));
  }

  return plStatus(PLASMA_SUCCESS);
}

plUInt16 plVisualShaderCodeGenerator::DeterminePinId(const plDocumentObject* pOwner, const plPin& pin) const
{
  const auto pins = m_pNodeManager->GetOutputPins(pOwner);

  for (plUInt32 i = 0; i < pins.GetCount(); ++i)
  {
    if (pins[i] == &pin)
      return i;
  }

  return 0xFFFF;
}

plStatus plVisualShaderCodeGenerator::GenerateVisualShader(const plDocumentNodeManager* pNodeManager, plStringBuilder& out_sCheckPerms)
{
  out_sCheckPerms.Clear();

  PLASMA_ASSERT_DEBUG(m_pNodeManager == nullptr, "Shader Generator cannot be used twice");

  m_pNodeManager = pNodeManager;
  m_pTypeRegistry = plVisualShaderTypeRegistry::GetSingleton();
  m_pNodeBaseRtti = m_pTypeRegistry->GetNodeBaseType();

  PLASMA_SUCCEED_OR_RETURN(GatherAllNodes(m_pNodeManager->GetRootObject()));

  if (m_Nodes.IsEmpty())
    return plStatus("Visual Shader graph is empty");

  if (m_pMainNode == nullptr)
    return plStatus("Visual Shader does not contain an output node");

  PLASMA_SUCCEED_OR_RETURN(GenerateNode(m_pMainNode));

  const plStringBuilder sMaterialCBDefine("#define VSE_CONSTANTS ", m_sShaderMaterialCB);

  m_sFinalShaderCode.Set("[PLATFORMS]\nALL\n\n");
  m_sFinalShaderCode.Append("[PERMUTATIONS]\n\n", m_sShaderPermutations, "\n");
  m_sFinalShaderCode.Append("[MATERIALPARAMETER]\n\n", m_sShaderMaterialParam, "\n");
  m_sFinalShaderCode.Append("[RENDERSTATE]\n\n", m_sShaderRenderState, "\n");
  m_sFinalShaderCode.Append("[VERTEXSHADER]\n\n", sMaterialCBDefine, "\n\n");
  m_sFinalShaderCode.Append(m_sShaderVertexDefines, "\n", m_sShaderVertex, "\n");
  m_sFinalShaderCode.Append("[GEOMETRYSHADER]\n\n", sMaterialCBDefine, "\n\n");
  m_sFinalShaderCode.Append(m_sShaderGeometryDefines, "\n", m_sShaderGeometry, "\n");
  m_sFinalShaderCode.Append("[PIXELSHADER]\n\n", sMaterialCBDefine, "\n\n");
  m_sFinalShaderCode.Append(m_sShaderPixelDefines, "\n", m_sShaderPixelIncludes, "\n");
  m_sFinalShaderCode.Append(m_sShaderPixelConstants, "\n", m_sShaderPixelSamplers, "\n", m_sShaderPixelBody, "\n");

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto pDesc = m_pTypeRegistry->GetDescriptorForType(it.Key()->GetType());
    out_sCheckPerms.Append("\n", pDesc->m_sCheckPermutations);
  }

  return plStatus(PLASMA_SUCCESS);
}

plStatus plVisualShaderCodeGenerator::GenerateNode(const plDocumentObject* pNode)
{
  NodeState& state = m_Nodes[pNode];

  if (state.m_bInProgress)
    return plStatus("The shader graph has a circular dependency.");

  if (state.m_bCodeGenerated)
    return plStatus(PLASMA_SUCCESS);

  state.m_bCodeGenerated = true;
  state.m_bInProgress = true;

  PLASMA_SCOPE_EXIT(state.m_bInProgress = false);

  const plVisualShaderNodeDescriptor* pDesc = m_pTypeRegistry->GetDescriptorForType(pNode->GetType());

  PLASMA_SUCCEED_OR_RETURN(GenerateInputPinCode(m_pNodeManager->GetInputPins(pNode)));

  plStringBuilder sConstantsCode, sPsBodyCode, sMaterialParamCode, sPixelSamplersCode, sVsBodyCode, sGsBodyCode, sMaterialCB, sPermutations,
    sRenderStates, sPixelDefines, sPixelIncludes, sVertexDefines, sGeometryDefines;

  sConstantsCode = pDesc->m_sShaderCodePixelConstants;
  sPsBodyCode = pDesc->m_sShaderCodePixelBody;
  sMaterialParamCode = pDesc->m_sShaderCodeMaterialParams;
  sPixelSamplersCode = pDesc->m_sShaderCodePixelSamplers;
  sVsBodyCode = pDesc->m_sShaderCodeVertexShader;
  sGsBodyCode = pDesc->m_sShaderCodeGeometryShader;
  sMaterialCB = pDesc->m_sShaderCodeMaterialCB;
  sPermutations = pDesc->m_sShaderCodePermutations;
  sRenderStates = pDesc->m_sShaderCodeRenderState;
  sPixelDefines = pDesc->m_sShaderCodePixelDefines;
  sPixelIncludes = pDesc->m_sShaderCodePixelIncludes;

  PLASMA_SUCCEED_OR_RETURN(ReplaceInputPinsByCode(pNode, pDesc, sPsBodyCode, sPixelDefines));
  PLASMA_SUCCEED_OR_RETURN(ReplaceInputPinsByCode(pNode, pDesc, sVsBodyCode, sVertexDefines));
  PLASMA_SUCCEED_OR_RETURN(ReplaceInputPinsByCode(pNode, pDesc, sGsBodyCode, sGeometryDefines));

  PLASMA_SUCCEED_OR_RETURN(CheckPropertyValues(pNode, pDesc));
  PLASMA_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sConstantsCode));
  PLASMA_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sVsBodyCode));
  PLASMA_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sGsBodyCode));
  PLASMA_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sPsBodyCode));
  PLASMA_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sMaterialParamCode));
  PLASMA_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sPixelDefines));
  PLASMA_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sMaterialCB));
  PLASMA_SUCCEED_OR_RETURN(InsertPropertyValues(pNode, pDesc, sPixelSamplersCode));

  SetPinDefines(pNode, sPermutations);
  SetPinDefines(pNode, sRenderStates);
  SetPinDefines(pNode, sVsBodyCode);
  SetPinDefines(pNode, sGsBodyCode);
  SetPinDefines(pNode, sMaterialParamCode);
  SetPinDefines(pNode, sPixelDefines);
  SetPinDefines(pNode, sPixelIncludes);
  SetPinDefines(pNode, sPsBodyCode);
  SetPinDefines(pNode, sConstantsCode);
  SetPinDefines(pNode, sPixelSamplersCode);
  SetPinDefines(pNode, sMaterialCB);

  {
    AppendStringIfUnique(m_sShaderPermutations, sPermutations);
    AppendStringIfUnique(m_sShaderRenderState, sRenderStates);
    AppendStringIfUnique(m_sShaderVertexDefines, sVertexDefines);
    AppendStringIfUnique(m_sShaderVertex, sVsBodyCode);
    AppendStringIfUnique(m_sShaderGeometryDefines, sGeometryDefines);
    AppendStringIfUnique(m_sShaderGeometry, sGsBodyCode);
    AppendStringIfUnique(m_sShaderMaterialParam, sMaterialParamCode);
    AppendStringIfUnique(m_sShaderPixelDefines, sPixelDefines);
    AppendStringIfUnique(m_sShaderPixelIncludes, sPixelIncludes);
    AppendStringIfUnique(m_sShaderPixelBody, sPsBodyCode);
    AppendStringIfUnique(m_sShaderPixelConstants, sConstantsCode);
    AppendStringIfUnique(m_sShaderPixelSamplers, sPixelSamplersCode);
    AppendStringIfUnique(m_sShaderMaterialCB, sMaterialCB);
  }

  return plStatus(PLASMA_SUCCESS);
}

plStatus plVisualShaderCodeGenerator::GenerateInputPinCode(plArrayPtr<const plUniquePtr<const plPin>> pins)
{
  for (auto& pPin : pins)
  {
    auto connections = m_pNodeManager->GetConnections(*pPin);
    PLASMA_ASSERT_DEBUG(connections.GetCount() <= 1, "Input pin has {0} connections", connections.GetCount());

    if (connections.IsEmpty())
      continue;

    const plPin& pinSource = connections[0]->GetSourcePin();

    // recursively generate all dependent code
    const plDocumentObject* pOwnerNode = pinSource.GetParent();
    const plStatus resNode = GenerateOutputPinCode(pOwnerNode, pinSource);

    if (resNode.m_Result.Failed())
      return resNode;
  }

  return plStatus(PLASMA_SUCCESS);
}

plStatus plVisualShaderCodeGenerator::GenerateOutputPinCode(const plDocumentObject* pOwnerNode, const plPin& pin)
{
  OutputPinState& ps = m_OutputPins[&pin];

  if (ps.m_bCodeGenerated)
    return plStatus(PLASMA_SUCCESS);

  ps.m_bCodeGenerated = true;

  PLASMA_SUCCEED_OR_RETURN(GenerateNode(pOwnerNode));

  const plVisualShaderNodeDescriptor* pDesc = m_pTypeRegistry->GetDescriptorForType(pOwnerNode->GetType());
  const plUInt16 uiPinID = DeterminePinId(pOwnerNode, pin);

  plStringBuilder sInlineCode = pDesc->m_OutputPins[uiPinID].m_sShaderCodeInline;
  plStringBuilder ignore; // DefineWhenUsingDefaultValue not used for output pins

  PLASMA_SUCCEED_OR_RETURN(ReplaceInputPinsByCode(pOwnerNode, pDesc, sInlineCode, ignore));

  PLASMA_SUCCEED_OR_RETURN(InsertPropertyValues(pOwnerNode, pDesc, sInlineCode));

  // store the result
  ps.m_sCodeAtPin = sInlineCode;

  return plStatus(PLASMA_SUCCESS);
}



plStatus plVisualShaderCodeGenerator::ReplaceInputPinsByCode(
  const plDocumentObject* pOwnerNode, const plVisualShaderNodeDescriptor* pNodeDesc, plStringBuilder& sInlineCode, plStringBuilder& sCodeForPlacingDefines)
{
  auto inputPins = m_pNodeManager->GetInputPins(pOwnerNode);

  plStringBuilder sPinName, sValue;

  for (plUInt32 i0 = inputPins.GetCount(); i0 > 0; --i0)
  {
    const plUInt32 i = i0 - 1;

    sPinName.Format("$in{0}", i);

    auto connections = m_pNodeManager->GetConnections(*inputPins[i]);
    if (connections.IsEmpty())
    {
      if (pNodeDesc->m_InputPins[i].m_bExposeAsProperty)
      {
        plVariant val = pOwnerNode->GetTypeAccessor().GetValue(pNodeDesc->m_InputPins[i].m_sName);
        sValue = ToShaderString(val);
      }
      else
      {
        sValue = pNodeDesc->m_InputPins[i].m_sDefaultValue;

        for (const auto& sDefine : pNodeDesc->m_InputPins[i].m_sDefinesWhenUsingDefaultValue)
        {
          sCodeForPlacingDefines.Append("#if !defined(", sDefine, ")\n");
          sCodeForPlacingDefines.Append("  #define ", sDefine, "\n");
          sCodeForPlacingDefines.Append("#endif\n");
        }
      }

      if (sValue.IsEmpty())
      {
        return plStatus(plFmt("Not all required input pins on a '{0}' node are connected.", pNodeDesc->m_sName));
      }

      // replace all occurrences of the pin identifier with the code that was generate for the connected output pin
      sInlineCode.ReplaceAll(sPinName, sValue);
    }
    else
    {
      const plPin& outputPin = connections[0]->GetSourcePin();

      const OutputPinState& pinState = m_OutputPins[&outputPin];
      PLASMA_ASSERT_DEBUG(pinState.m_bCodeGenerated, "Pin code should have been generated at this point");

      // replace all occurrences of the pin identifier with the code that was generate for the connected output pin
      sInlineCode.ReplaceAll(sPinName, pinState.m_sCodeAtPin);
    }
  }

  return plStatus(PLASMA_SUCCESS);
}


void plVisualShaderCodeGenerator::SetPinDefines(const plDocumentObject* pOwnerNode, plStringBuilder& sInlineCode)
{
  plStringBuilder sDefineName;

  {
    auto pins = m_pNodeManager->GetInputPins(pOwnerNode);

    for (plUInt32 i = 0; i < pins.GetCount(); ++i)
    {
      sDefineName.Format("INPUT_PIN_{0}_CONNECTED", i);

      if (m_pNodeManager->HasConnections(*pins[i]) == false)
      {
        sInlineCode.ReplaceAll(sDefineName, "0");
      }
      else
      {
        sInlineCode.ReplaceAll(sDefineName, "1");
      }
    }
  }

  {
    auto pins = m_pNodeManager->GetOutputPins(pOwnerNode);

    for (plUInt32 i = 0; i < pins.GetCount(); ++i)
    {
      sDefineName.Format("OUTPUT_PIN_{0}_CONNECTED", i);

      if (m_pNodeManager->HasConnections(*pins[i]) == false)
      {
        sInlineCode.ReplaceAll(sDefineName, "0");
      }
      else
      {
        sInlineCode.ReplaceAll(sDefineName, "1");
      }
    }
  }
}

void plVisualShaderCodeGenerator::AppendStringIfUnique(plStringBuilder& inout_String, const char* szAppend)
{
  if (inout_String.FindSubString(szAppend) != nullptr)
    return;

  inout_String.Append(szAppend);
}

plStatus plVisualShaderCodeGenerator::CheckPropertyValues(const plDocumentObject* pNode, const plVisualShaderNodeDescriptor* pDesc)
{
  const auto& TypeAccess = pNode->GetTypeAccessor();

  plStringBuilder sPropValue;

  const auto& props = pDesc->m_Properties;
  for (plUInt32 p = 0; p < props.GetCount(); ++p)
  {
    const plVariant value = TypeAccess.GetValue(props[p].m_sName);
    sPropValue = ToShaderString(value);


    const plInt8 iUniqueValueGroup = pDesc->m_UniquePropertyValueGroups[p];
    if (iUniqueValueGroup > 0)
    {
      if (sPropValue.IsEmpty())
      {
        return plStatus(plFmt("A '{0}' node has an empty '{1}' property.", pDesc->m_sName, props[p].m_sName));
      }

      if (!plStringUtils::IsValidIdentifierName(sPropValue))
      {
        return plStatus(plFmt("A '{0}' node has a '{1}' property that is not a valid identifier: '{2}'. Only letters, digits and _ are allowed.",
          pDesc->m_sName, props[p].m_sName, sPropValue));
      }

      auto& set = m_UsedUniqueValues[iUniqueValueGroup];

      if (set.Contains(sPropValue))
      {
        return plStatus(plFmt(
          "A '{0}' node has a '{1}' property that has the same value ('{2}') as another parameter.", pDesc->m_sName, props[p].m_sName, sPropValue));
      }

      set.Insert(sPropValue);
    }
  }

  return plStatus(PLASMA_SUCCESS);
}

plStatus plVisualShaderCodeGenerator::InsertPropertyValues(
  const plDocumentObject* pNode, const plVisualShaderNodeDescriptor* pDesc, plStringBuilder& sString)
{
  const auto& TypeAccess = pNode->GetTypeAccessor();

  plStringBuilder sPropName, sPropValue;

  const auto& props = pDesc->m_Properties;
  for (plUInt32 p0 = props.GetCount(); p0 > 0; --p0)
  {
    const plUInt32 p = p0 - 1;

    sPropName.Format("$prop{0}", p);

    const plVariant value = TypeAccess.GetValue(props[p].m_sName);
    sPropValue = ToShaderString(value);

    sString.ReplaceAll(sPropName, sPropValue);
  }

  return plStatus(PLASMA_SUCCESS);
}
