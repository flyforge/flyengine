#include <RendererCore/RendererCorePCH.h>

#include <Core/World/GameObject.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>

plAnimGraph::plAnimGraph()
{
  Clear();
}

plAnimGraph::~plAnimGraph() = default;

void plAnimGraph::Clear()
{
  plMemoryUtils::ZeroFillArray(m_uiInputPinCounts);
  plMemoryUtils::ZeroFillArray(m_uiPinInstanceDataOffset);
  m_From.Clear();
  m_Nodes.Clear();
  m_bPreparedForUse = true;
  m_InstanceDataAllocator.ClearDescs();

  for (auto& r : m_OutputPinToInputPinMapping)
  {
    r.Clear();
  }
}

plAnimGraphNode* plAnimGraph::AddNode(plUniquePtr<plAnimGraphNode>&& pNode)
{
  m_bPreparedForUse = false;

  m_Nodes.PushBack(std::move(pNode));
  return m_Nodes.PeekBack().Borrow();
}

void plAnimGraph::AddConnection(const plAnimGraphNode* pSrcNode, plStringView sSrcPinName, plAnimGraphNode* pDstNode, plStringView sDstPinName)
{
  // TODO: assert pSrcNode and pDstNode exist

  m_bPreparedForUse = false;
  plStringView sIdx;

  plAbstractMemberProperty* pPinPropSrc = (plAbstractMemberProperty*)pSrcNode->GetDynamicRTTI()->FindPropertyByName(sSrcPinName);

  auto& to = m_From[pSrcNode].m_To.ExpandAndGetRef();
  to.m_sSrcPinName = sSrcPinName;
  to.m_pDstNode = pDstNode;
  to.m_sDstPinName = sDstPinName;
  to.m_pSrcPin = (plAnimGraphPin*)pPinPropSrc->GetPropertyPointer(pSrcNode);

  if (const char* szIdx = sDstPinName.FindSubString("["))
  {
    sIdx = plStringView(szIdx + 1, sDstPinName.GetEndPointer() - 1);
    sDstPinName = plStringView(sDstPinName.GetStartPointer(), szIdx);

    plAbstractArrayProperty* pPinPropDst = (plAbstractArrayProperty*)pDstNode->GetDynamicRTTI()->FindPropertyByName(sDstPinName);
    const plDynamicPinAttribute* pDynPinAttr = pPinPropDst->GetAttributeByType<plDynamicPinAttribute>();

    const plTypedMemberProperty<plUInt8>* pPinSizeProp = (const plTypedMemberProperty<plUInt8>*)pDstNode->GetDynamicRTTI()->FindPropertyByName(pDynPinAttr->GetProperty());
    plUInt8 uiArraySize = pPinSizeProp->GetValue(pDstNode);
    pPinPropDst->SetCount(pDstNode, uiArraySize);

    plUInt32 uiIdx;
    plConversionUtils::StringToUInt(sIdx, uiIdx).AssertSuccess();

    to.m_pDstPin = (plAnimGraphPin*)pPinPropDst->GetValuePointer(pDstNode, uiIdx);
  }
  else
  {
    plAbstractMemberProperty* pPinPropDst = (plAbstractMemberProperty*)pDstNode->GetDynamicRTTI()->FindPropertyByName(sDstPinName);

    to.m_pDstPin = (plAnimGraphPin*)pPinPropDst->GetPropertyPointer(pDstNode);
  }
}

void plAnimGraph::PreparePinMapping()
{
  plUInt16 uiOutputPinCounts[plAnimGraphPin::Type::ENUM_COUNT];
  plMemoryUtils::ZeroFillArray(uiOutputPinCounts);

  for (const auto& consFrom : m_From)
  {
    for (const ConnectionTo& to : consFrom.Value().m_To)
    {
      uiOutputPinCounts[to.m_pSrcPin->GetPinType()]++;
    }
  }

  for (plUInt32 i = 0; i < plAnimGraphPin::ENUM_COUNT; ++i)
  {
    m_OutputPinToInputPinMapping[i].Clear();
    m_OutputPinToInputPinMapping[i].SetCount(uiOutputPinCounts[i]);
  }
}

void plAnimGraph::AssignInputPinIndices()
{
  plMemoryUtils::ZeroFillArray(m_uiInputPinCounts);

  for (auto& consFrom : m_From)
  {
    for (ConnectionTo& to : consFrom.Value().m_To)
    {
      // there may be multiple connections to this pin
      // only assign the index the first time we see a connection to this pin
      // otherwise only count up the number of connections

      if (to.m_pDstPin->m_iPinIndex == -1)
      {
        to.m_pDstPin->m_iPinIndex = m_uiInputPinCounts[to.m_pDstPin->GetPinType()]++;
      }

      ++to.m_pDstPin->m_uiNumConnections;
    }
  }
}

void plAnimGraph::AssignOutputPinIndices()
{
  plInt16 iPinTypeCount[plAnimGraphPin::Type::ENUM_COUNT];
  plMemoryUtils::ZeroFillArray(iPinTypeCount);

  for (auto& consFrom : m_From)
  {
    for (ConnectionTo& to : consFrom.Value().m_To)
    {
      const plUInt8 pinType = to.m_pSrcPin->GetPinType();

      // there may be multiple connections from this pin
      // only assign the index the first time we see a connection from this pin

      if (to.m_pSrcPin->m_iPinIndex == -1)
      {
        to.m_pSrcPin->m_iPinIndex = iPinTypeCount[pinType]++;
      }

      // store the indices of all the destination pins
      m_OutputPinToInputPinMapping[pinType][to.m_pSrcPin->m_iPinIndex].PushBack(to.m_pDstPin->m_iPinIndex);
    }
  }
}

plUInt16 plAnimGraph::ComputeNodePriority(const plAnimGraphNode* pNode, plMap<const plAnimGraphNode*, plUInt16>& inout_Prios, plUInt16& inout_uiOutputPrio) const
{
  auto itPrio = inout_Prios.Find(pNode);
  if (itPrio.IsValid())
  {
    // priority already computed -> return it
    return itPrio.Value();
  }

  const auto itConsFrom = m_From.Find(pNode);

  plUInt16 uiOwnPrio = 0xFFFF;

  if (itConsFrom.IsValid())
  {
    // look at all outgoing priorities and take the smallest dst priority - 1
    for (const ConnectionTo& to : itConsFrom.Value().m_To)
    {
      uiOwnPrio = plMath::Min<plUInt16>(uiOwnPrio, ComputeNodePriority(to.m_pDstNode, inout_Prios, inout_uiOutputPrio) - 1);
    }
  }
  else
  {
    // has no outgoing connections at all -> max priority value
    uiOwnPrio = inout_uiOutputPrio;
    inout_uiOutputPrio -= 64;
  }

  PLASMA_ASSERT_DEBUG(uiOwnPrio != 0xFFFF, "");

  inout_Prios[pNode] = uiOwnPrio;
  return uiOwnPrio;
}

void plAnimGraph::SortNodesByPriority()
{
  // this is important so that we can step all nodes in linear order,
  // and have them generate their output such that it is ready before
  // dependent nodes are stepped

  plUInt16 uiOutputPrio = 0xFFFE;
  plMap<const plAnimGraphNode*, plUInt16> prios;
  for (const auto& pNode : m_Nodes)
  {
    ComputeNodePriority(pNode.Borrow(), prios, uiOutputPrio);
  }

  m_Nodes.Sort([&](const auto& lhs, const auto& rhs) -> bool { return prios[lhs.Borrow()] < prios[rhs.Borrow()]; });
}

void plAnimGraph::PrepareForUse()
{
  if (m_bPreparedForUse)
    return;

  m_bPreparedForUse = true;

  for (auto& consFrom : m_From)
  {
    for (ConnectionTo& to : consFrom.Value().m_To)
    {
      to.m_pSrcPin->m_iPinIndex = -1;
      to.m_pSrcPin->m_uiNumConnections = 0;
      to.m_pDstPin->m_iPinIndex = -1;
      to.m_pDstPin->m_uiNumConnections = 0;
    }
  }

  SortNodesByPriority();
  PreparePinMapping();
  AssignInputPinIndices();
  AssignOutputPinIndices();

  m_InstanceDataAllocator.ClearDescs();
  for (const auto& pNode : m_Nodes)
  {
    plInstanceDataDesc desc;
    if (pNode->GetInstanceDataDesc(desc))
    {
      pNode->m_uiInstanceDataOffset = m_InstanceDataAllocator.AddDesc(desc);
    }
  }

  // EXTEND THIS if a new type is introduced
  {
    plInstanceDataDesc desc;
    desc.m_uiTypeAlignment = PLASMA_ALIGNMENT_OF(plInt8);
    desc.m_uiTypeSize = sizeof(plInt8) * m_uiInputPinCounts[plAnimGraphPin::Type::Trigger];
    m_uiPinInstanceDataOffset[plAnimGraphPin::Type::Trigger] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    plInstanceDataDesc desc;
    desc.m_uiTypeAlignment = PLASMA_ALIGNMENT_OF(double);
    desc.m_uiTypeSize = sizeof(double) * m_uiInputPinCounts[plAnimGraphPin::Type::Number];
    m_uiPinInstanceDataOffset[plAnimGraphPin::Type::Number] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    plInstanceDataDesc desc;
    desc.m_uiTypeAlignment = PLASMA_ALIGNMENT_OF(bool);
    desc.m_uiTypeSize = sizeof(bool) * m_uiInputPinCounts[plAnimGraphPin::Type::Bool];
    m_uiPinInstanceDataOffset[plAnimGraphPin::Type::Bool] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    plInstanceDataDesc desc;
    desc.m_uiTypeAlignment = PLASMA_ALIGNMENT_OF(plUInt16);
    desc.m_uiTypeSize = sizeof(plUInt16) * m_uiInputPinCounts[plAnimGraphPin::Type::BoneWeights];
    m_uiPinInstanceDataOffset[plAnimGraphPin::Type::BoneWeights] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    plInstanceDataDesc desc;
    desc.m_uiTypeAlignment = PLASMA_ALIGNMENT_OF(plUInt16);
    desc.m_uiTypeSize = sizeof(plUInt16) * m_uiInputPinCounts[plAnimGraphPin::Type::ModelPose];
    m_uiPinInstanceDataOffset[plAnimGraphPin::Type::ModelPose] = m_InstanceDataAllocator.AddDesc(desc);
  }
}

plResult plAnimGraph::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(10);

  const plUInt32 uiNumNodes = m_Nodes.GetCount();
  inout_stream << uiNumNodes;

  plMap<const plAnimGraphNode*, plUInt32> nodeToIdx;

  for (plUInt32 n = 0; n < m_Nodes.GetCount(); ++n)
  {
    const plAnimGraphNode* pNode = m_Nodes[n].Borrow();

    nodeToIdx[pNode] = n;

    inout_stream << pNode->GetDynamicRTTI()->GetTypeName();
    PLASMA_SUCCEED_OR_RETURN(pNode->SerializeNode(inout_stream));
  }

  inout_stream << m_From.GetCount();
  for (auto itFrom : m_From)
  {
    inout_stream << nodeToIdx[itFrom.Key()];

    const auto& toAll = itFrom.Value().m_To;
    inout_stream << toAll.GetCount();

    for (const auto& to : toAll)
    {
      inout_stream << to.m_sSrcPinName;
      inout_stream << nodeToIdx[to.m_pDstNode];
      inout_stream << to.m_sDstPinName;
    }
  }

  return PLASMA_SUCCESS;
}

plResult plAnimGraph::Deserialize(plStreamReader& inout_stream)
{
  Clear();

  const plTypeVersion version = inout_stream.ReadVersion(10);

  if (version < 10)
    return PLASMA_FAILURE;

  plUInt32 uiNumNodes = 0;
  inout_stream >> uiNumNodes;

  plDynamicArray<plAnimGraphNode*> idxToNode;
  idxToNode.SetCount(uiNumNodes);

  plStringBuilder sTypeName;

  for (plUInt32 n = 0; n < uiNumNodes; ++n)
  {
    inout_stream >> sTypeName;
    plUniquePtr<plAnimGraphNode> pNode = plRTTI::FindTypeByName(sTypeName)->GetAllocator()->Allocate<plAnimGraphNode>();
    PLASMA_SUCCEED_OR_RETURN(pNode->DeserializeNode(inout_stream));

    idxToNode[n] = AddNode(std::move(pNode));
  }

  plUInt32 uiNumConnectionsFrom = 0;
  inout_stream >> uiNumConnectionsFrom;

  plStringBuilder sPinSrc, sPinDst;

  for (plUInt32 cf = 0; cf < uiNumConnectionsFrom; ++cf)
  {
    plUInt32 nodeIdx;
    inout_stream >> nodeIdx;
    const plAnimGraphNode* ptrNodeFrom = idxToNode[nodeIdx];

    plUInt32 uiNumConnectionsTo = 0;
    inout_stream >> uiNumConnectionsTo;

    for (plUInt32 ct = 0; ct < uiNumConnectionsTo; ++ct)
    {
      inout_stream >> sPinSrc;

      inout_stream >> nodeIdx;
      plAnimGraphNode* ptrNodeTo = idxToNode[nodeIdx];

      inout_stream >> sPinDst;

      AddConnection(ptrNodeFrom, sPinSrc, ptrNodeTo, sPinDst);
    }
  }

  m_bPreparedForUse = false;
  return PLASMA_SUCCESS;
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraph);
