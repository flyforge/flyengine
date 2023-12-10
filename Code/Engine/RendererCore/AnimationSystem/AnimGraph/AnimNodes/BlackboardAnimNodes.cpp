#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/BlackboardAnimNodes.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSetBlackboardNumberAnimNode, 1, plRTTIDefaultAllocator<plSetBlackboardNumberAnimNode>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),
    PLASMA_MEMBER_PROPERTY("Number", m_fNumber),

    PLASMA_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("InNumber", m_InNumber)->AddAttributes(new plHiddenAttribute()),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Set Number: '{BlackboardEntry}' to {Number}"),
    new plCategoryAttribute("Blackboard"),
    new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Red)),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plSetBlackboardNumberAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;
  stream << m_fNumber;

  PLASMA_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InNumber.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plSetBlackboardNumberAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;
  stream >> m_fNumber;

  PLASMA_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InNumber.Deserialize(stream));

  return PLASMA_SUCCESS;
}

void plSetBlackboardNumberAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* plSetBlackboardNumberAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void plSetBlackboardNumberAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  if (!m_InActivate.IsTriggered(ref_graph))
    return;

  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  pBlackboard->SetEntryValue(m_sBlackboardEntry, m_InNumber.GetNumber(ref_graph, m_fNumber)).IgnoreResult();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGetBlackboardNumberAnimNode, 1, plRTTIDefaultAllocator<plGetBlackboardNumberAnimNode>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),

    PLASMA_MEMBER_PROPERTY("OutNumber", m_OutNumber)->AddAttributes(new plHiddenAttribute()),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Blackboard"),
    new plTitleAttribute("Get Number: '{BlackboardEntry}'"),
    new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Lime)),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plGetBlackboardNumberAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;

  PLASMA_SUCCEED_OR_RETURN(m_OutNumber.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plGetBlackboardNumberAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;

  PLASMA_SUCCEED_OR_RETURN(m_OutNumber.Deserialize(stream));

  return PLASMA_SUCCESS;
}

void plGetBlackboardNumberAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* plGetBlackboardNumberAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void plGetBlackboardNumberAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  plVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

  if (!value.IsValid() || !value.IsNumber())
  {
    plLog::Warning("AnimController::GetBlackboardNumber: '{}' doesn't exist or isn't a number type.", m_sBlackboardEntry);
    return;
  }

  m_OutNumber.SetNumber(ref_graph, value.ConvertTo<double>());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCompareBlackboardNumberAnimNode, 1, plRTTIDefaultAllocator<plCompareBlackboardNumberAnimNode>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),
    PLASMA_MEMBER_PROPERTY("ReferenceValue", m_fReferenceValue),
    PLASMA_ENUM_MEMBER_PROPERTY("Comparison", plComparisonOperator, m_Comparison),

    PLASMA_MEMBER_PROPERTY("OutOnTrue", m_OutOnTrue)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("OutOnFalse", m_OutOnFalse)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("OutIsTrue", m_OutIsTrue)->AddAttributes(new plHiddenAttribute()),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Blackboard"),
    new plTitleAttribute("Check: '{BlackboardEntry}' {Comparison} {ReferenceValue}"),
    new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Lime)),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plCompareBlackboardNumberAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(2);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;
  stream << m_fReferenceValue;
  stream << m_Comparison;

  PLASMA_SUCCEED_OR_RETURN(m_OutOnTrue.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutOnFalse.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutIsTrue.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plCompareBlackboardNumberAnimNode::DeserializeNode(plStreamReader& stream)
{
  const auto version = stream.ReadVersion(2);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;
  stream >> m_fReferenceValue;
  stream >> m_Comparison;

  PLASMA_SUCCEED_OR_RETURN(m_OutOnTrue.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutOnFalse.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutIsTrue.Deserialize(stream));

  return PLASMA_SUCCESS;
}

void plCompareBlackboardNumberAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* plCompareBlackboardNumberAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void plCompareBlackboardNumberAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  const plVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

  if (!value.IsValid() || !value.IsNumber())
  {
    plLog::Warning("AnimController::CompareBlackboardNumber: '{}' doesn't exist or isn't a number type.", m_sBlackboardEntry);
    return;
  }

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  const double fValue = value.ConvertTo<double>();
  const bool bIsTrueNow = plComparisonOperator::Compare(m_Comparison, fValue, m_fReferenceValue);
  const plInt8 iIsTrueNow = bIsTrueNow ? 1 : 0;

  m_OutIsTrue.SetBool(ref_graph, bIsTrueNow);

  // we use a tri-state bool here to ensure that OnTrue or OnFalse get fired right away
  if (pInstance->m_iIsTrue != iIsTrueNow)
  {
    pInstance->m_iIsTrue = iIsTrueNow;

    if (bIsTrueNow)
    {
      m_OutOnTrue.SetTriggered(ref_graph);
    }
    else
    {
      m_OutOnFalse.SetTriggered(ref_graph);
    }
  }
}

bool plCompareBlackboardNumberAnimNode::GetInstanceDataDesc(plInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCheckBlackboardBoolAnimNode, 1, plRTTIDefaultAllocator<plCheckBlackboardBoolAnimNode>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),

    PLASMA_MEMBER_PROPERTY("OutOnTrue", m_OutOnTrue)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("OutOnFalse", m_OutOnFalse)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("OutBool", m_OutBool)->AddAttributes(new plHiddenAttribute()),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Blackboard"),
    new plTitleAttribute("Check Bool: '{BlackboardEntry}'"),
    new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Lime)),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plCheckBlackboardBoolAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;

  PLASMA_SUCCEED_OR_RETURN(m_OutOnTrue.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutOnFalse.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutBool.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plCheckBlackboardBoolAnimNode::DeserializeNode(plStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;

  PLASMA_SUCCEED_OR_RETURN(m_OutOnTrue.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutOnFalse.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_OutBool.Deserialize(stream));

  return PLASMA_SUCCESS;
}

void plCheckBlackboardBoolAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* plCheckBlackboardBoolAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void plCheckBlackboardBoolAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  const plVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

  if (!value.IsValid() || !value.CanConvertTo<bool>())
  {
    plLog::Warning("AnimController::CheckBlackboardBool: '{}' doesn't exist or isn't a bool type.", m_sBlackboardEntry);
    return;
  }

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  const bool bValue = value.ConvertTo<bool>();
  const plInt8 iIsTrueNow = bValue ? 1 : 0;

  m_OutBool.SetBool(ref_graph, bValue);

  // we use a tri-state bool here to ensure that OnTrue or OnFalse get fired right away
  if (pInstance->m_iIsTrue != iIsTrueNow)
  {
    pInstance->m_iIsTrue = iIsTrueNow;

    if (bValue)
    {
      m_OutOnTrue.SetTriggered(ref_graph);
    }
    else
    {
      m_OutOnFalse.SetTriggered(ref_graph);
    }
  }
}

bool plCheckBlackboardBoolAnimNode::GetInstanceDataDesc(plInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSetBlackboardBoolAnimNode, 1, plRTTIDefaultAllocator<plSetBlackboardBoolAnimNode>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),
    PLASMA_MEMBER_PROPERTY("Bool", m_bBool),

    PLASMA_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new plHiddenAttribute()),
    PLASMA_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new plHiddenAttribute()),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Set Bool: '{BlackboardEntry}' to {Bool}"),
    new plCategoryAttribute("Blackboard"),
    new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Red)),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plSetBlackboardBoolAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;
  stream << m_bBool;

  PLASMA_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InBool.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plSetBlackboardBoolAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;
  stream >> m_bBool;

  PLASMA_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));
  PLASMA_SUCCEED_OR_RETURN(m_InBool.Deserialize(stream));

  return PLASMA_SUCCESS;
}

void plSetBlackboardBoolAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* plSetBlackboardBoolAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void plSetBlackboardBoolAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  if (!m_InActivate.IsTriggered(ref_graph))
    return;

  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  pBlackboard->SetEntryValue(m_sBlackboardEntry, m_InBool.GetBool(ref_graph, m_bBool)).IgnoreResult();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plGetBlackboardBoolAnimNode, 1, plRTTIDefaultAllocator<plGetBlackboardBoolAnimNode>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),

    PLASMA_MEMBER_PROPERTY("OutBool", m_OutBool)->AddAttributes(new plHiddenAttribute()),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Blackboard"),
    new plTitleAttribute("Get Bool: '{BlackboardEntry}'"),
    new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Lime)),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plGetBlackboardBoolAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;

  PLASMA_SUCCEED_OR_RETURN(m_OutBool.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plGetBlackboardBoolAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;

  PLASMA_SUCCEED_OR_RETURN(m_OutBool.Deserialize(stream));

  return PLASMA_SUCCESS;
}

void plGetBlackboardBoolAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* plGetBlackboardBoolAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void plGetBlackboardBoolAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  plVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

  if (!value.IsValid() || !value.CanConvertTo<bool>())
  {
    plLog::Warning("AnimController::GetBlackboardBool: '{}' doesn't exist or can't be converted to bool.", m_sBlackboardEntry);
    return;
  }

  m_OutBool.SetBool(ref_graph, value.ConvertTo<bool>());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plOnBlackboardValueChangedAnimNode, 1, plRTTIDefaultAllocator<plOnBlackboardValueChangedAnimNode>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),

    PLASMA_MEMBER_PROPERTY("OutOnValueChanged", m_OutOnValueChanged)->AddAttributes(new plHiddenAttribute()),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Blackboard"),
    new plTitleAttribute("OnChanged: '{BlackboardEntry}'"),
    new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Lime)),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plOnBlackboardValueChangedAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;

  PLASMA_SUCCEED_OR_RETURN(m_OutOnValueChanged.Serialize(stream));

  return PLASMA_SUCCESS;
}

plResult plOnBlackboardValueChangedAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PLASMA_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;

  PLASMA_SUCCEED_OR_RETURN(m_OutOnValueChanged.Deserialize(stream));

  return PLASMA_SUCCESS;
}

void plOnBlackboardValueChangedAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* plOnBlackboardValueChangedAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void plOnBlackboardValueChangedAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  const plBlackboard::Entry* pEntry = pBlackboard->GetEntry(m_sBlackboardEntry);

  if (pEntry == nullptr)
  {
    plLog::Warning("AnimController::OnBlackboardValueChanged: '{}' doesn't exist.", m_sBlackboardEntry);
    return;
  }

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  if (pInstance->m_uiChangeCounter == pEntry->m_uiChangeCounter)
    return;

  if (pInstance->m_uiChangeCounter != plInvalidIndex)
  {
    m_OutOnValueChanged.SetTriggered(ref_graph);
  }

  pInstance->m_uiChangeCounter = pEntry->m_uiChangeCounter;
}

bool plOnBlackboardValueChangedAnimNode::GetInstanceDataDesc(plInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

//////////////////////////////////////////////////////////////////////////


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_BlackboardAnimNodes);
