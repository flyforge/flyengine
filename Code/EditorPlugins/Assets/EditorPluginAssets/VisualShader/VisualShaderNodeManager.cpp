#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>

//////////////////////////////////////////////////////////////////////////
// plVisualShaderPin
//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plVisualShaderPin, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plVisualShaderPin::plVisualShaderPin(Type type, const plVisualShaderPinDescriptor* pDescriptor, const plDocumentObject* pObject)
  : plPin(type, pDescriptor->m_sName, pDescriptor->m_Color, pObject)
{
  m_pDescriptor = pDescriptor;
}

const plRTTI* plVisualShaderPin::GetDataType() const
{
  return m_pDescriptor->m_pDataType;
}

const plString& plVisualShaderPin::GetTooltip() const
{
  return m_pDescriptor->m_sTooltip;
}

//////////////////////////////////////////////////////////////////////////
// plVisualShaderNodeManager
//////////////////////////////////////////////////////////////////////////

bool plVisualShaderNodeManager::InternalIsNode(const plDocumentObject* pObject) const
{
  return pObject->GetType()->IsDerivedFrom(plVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType());
}

void plVisualShaderNodeManager::InternalCreatePins(const plDocumentObject* pObject, NodeInternal& ref_node)
{
  const auto* pDesc = plVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pObject->GetType());

  if (pDesc == nullptr)
    return;

  ref_node.m_Inputs.Reserve(pDesc->m_InputPins.GetCount());
  ref_node.m_Outputs.Reserve(pDesc->m_OutputPins.GetCount());

  for (const auto& pin : pDesc->m_InputPins)
  {
    auto pPin = PLASMA_DEFAULT_NEW(plVisualShaderPin, plPin::Type::Input, &pin, pObject);
    ref_node.m_Inputs.PushBack(pPin);
  }

  for (const auto& pin : pDesc->m_OutputPins)
  {
    auto pPin = PLASMA_DEFAULT_NEW(plVisualShaderPin, plPin::Type::Output, &pin, pObject);
    ref_node.m_Outputs.PushBack(pPin);
  }
}

void plVisualShaderNodeManager::GetCreateableTypes(plHybridArray<const plRTTI*, 32>& ref_types) const
{
  const plRTTI* pNodeBaseType = plVisualShaderTypeRegistry::GetSingleton()->GetNodeBaseType();

  plRTTI::ForEachDerivedType(
    pNodeBaseType,
    [&](const plRTTI* pRtti) { ref_types.PushBack(pRtti); },
    plRTTI::ForEachOptions::ExcludeAbstract);
}

plStatus plVisualShaderNodeManager::InternalCanConnect(const plPin& source, const plPin& target, CanConnectResult& out_result) const
{
  const plVisualShaderPin& pinSource = plStaticCast<const plVisualShaderPin&>(source);
  const plVisualShaderPin& pinTarget = plStaticCast<const plVisualShaderPin&>(target);

  const plRTTI* pSamplerType = plVisualShaderTypeRegistry::GetSingleton()->GetPinSamplerType();
  const plRTTI* pStringType = plGetStaticRTTI<plString>();

  if ((pinSource.GetDataType() == pSamplerType && pinTarget.GetDataType() != pSamplerType) || (pinSource.GetDataType() != pSamplerType && pinTarget.GetDataType() == pSamplerType))
  {
    out_result = CanConnectResult::ConnectNever;
    return plStatus("Pin of type 'sampler' cannot be connected with a pin of a different type.");
  }

  if ((pinSource.GetDataType() == pStringType && pinTarget.GetDataType() != pStringType) || (pinSource.GetDataType() != pStringType && pinTarget.GetDataType() == pStringType))
  {
    out_result = CanConnectResult::ConnectNever;
    return plStatus("Pin of type 'string' cannot be connected with a pin of a different type.");
  }

  if (WouldConnectionCreateCircle(source, target))
  {
    out_result = CanConnectResult::ConnectNever;
    return plStatus("Connecting these pins would create a circle in the shader graph.");
  }

  out_result = CanConnectResult::ConnectNto1;
  return plStatus(PLASMA_SUCCESS);
}

plStringView plVisualShaderNodeManager::GetTypeCategory(const plRTTI* pRtti) const
{
  const plVisualShaderNodeDescriptor* pDesc = plVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pRtti);

  if (pDesc == nullptr)
    return {};

  return pDesc->m_sCategory;
}


plStatus plVisualShaderNodeManager::InternalCanAdd(const plRTTI* pRtti, const plDocumentObject* pParent, plStringView sParentProperty, const plVariant& index) const
{
  auto pDesc = plVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pRtti);

  if (pDesc)
  {
    if (pDesc->m_NodeType == plVisualShaderNodeType::Main && CountNodesOfType(plVisualShaderNodeType::Main) > 0)
    {
      return plStatus("The shader may only contain a single output node");
    }

    /// \todo This is an arbitrary limit and it does not count how many nodes reference the same texture
    static const plUInt32 uiMaxTextures = 16;
    if (pDesc->m_NodeType == plVisualShaderNodeType::Texture && CountNodesOfType(plVisualShaderNodeType::Texture) >= uiMaxTextures)
    {
      return plStatus(plFmt("The maximum number of texture nodes is {0}", uiMaxTextures));
    }
  }

  return plStatus(PLASMA_SUCCESS);
}

plUInt32 plVisualShaderNodeManager::CountNodesOfType(plVisualShaderNodeType::Enum type) const
{
  plUInt32 count = 0;

  const plVisualShaderTypeRegistry* pRegistry = plVisualShaderTypeRegistry::GetSingleton();

  const auto& children = GetRootObject()->GetChildren();
  for (plUInt32 i = 0; i < children.GetCount(); ++i)
  {
    auto pDesc = pRegistry->GetDescriptorForType(children[i]->GetType());

    if (pDesc && pDesc->m_NodeType == type)
    {
      ++count;
    }
  }

  return count;
}
