#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/RenderPipelineNode.h>

// PL_CHECK_AT_COMPILETIME(sizeof(plRenderPipelineNodePin) == 4);

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRenderPipelineNode, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plRenderPipelineNodePin, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_ATTRIBUTES
  {
   new plHiddenAttribute(),
  }
  PL_END_ATTRIBUTES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plRenderPipelineNodeInputPin, plRenderPipelineNodePin, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plRenderPipelineNodeOutputPin, plRenderPipelineNodePin, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plRenderPipelineNodePassThrougPin, plRenderPipelineNodePin, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

void plRenderPipelineNode::InitializePins()
{
  m_InputPins.Clear();
  m_OutputPins.Clear();
  m_NameToPin.Clear();

  const plRTTI* pType = GetDynamicRTTI();

  plHybridArray<const plAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() != plPropertyCategory::Member || !pProp->GetSpecificType()->IsDerivedFrom(plGetStaticRTTI<plRenderPipelineNodePin>()))
      continue;

    auto pPinProp = static_cast<const plAbstractMemberProperty*>(pProp);
    plRenderPipelineNodePin* pPin = static_cast<plRenderPipelineNodePin*>(pPinProp->GetPropertyPointer(this));

    pPin->m_pParent = this;
    if (pPin->m_Type == plRenderPipelineNodePin::Type::Unknown)
    {
      PL_REPORT_FAILURE("Pin '{0}' has an invalid type. Do not use plRenderPipelineNodePin directly as member but one of its derived types", pProp->GetPropertyName());
      continue;
    }

    if (pPin->m_Type == plRenderPipelineNodePin::Type::Input || pPin->m_Type == plRenderPipelineNodePin::Type::PassThrough)
    {
      pPin->m_uiInputIndex = static_cast<plUInt8>(m_InputPins.GetCount());
      m_InputPins.PushBack(pPin);
    }
    if (pPin->m_Type == plRenderPipelineNodePin::Type::Output || pPin->m_Type == plRenderPipelineNodePin::Type::PassThrough)
    {
      pPin->m_uiOutputIndex = static_cast<plUInt8>(m_OutputPins.GetCount());
      m_OutputPins.PushBack(pPin);
    }

    plHashedString sHashedName;
    sHashedName.Assign(pProp->GetPropertyName());
    m_NameToPin.Insert(sHashedName, pPin);
  }
}

plHashedString plRenderPipelineNode::GetPinName(const plRenderPipelineNodePin* pPin) const
{
  for (auto it = m_NameToPin.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value() == pPin)
    {
      return it.Key();
    }
  }
  return plHashedString();
}

const plRenderPipelineNodePin* plRenderPipelineNode::GetPinByName(const char* szName) const
{
  plHashedString sHashedName;
  sHashedName.Assign(szName);
  return GetPinByName(sHashedName);
}

const plRenderPipelineNodePin* plRenderPipelineNode::GetPinByName(plHashedString sName) const
{
  const plRenderPipelineNodePin* pin;
  if (m_NameToPin.TryGetValue(sName, pin))
  {
    return pin;
  }

  return nullptr;
}


PL_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelineNode);
