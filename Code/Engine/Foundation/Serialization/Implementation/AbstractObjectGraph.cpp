#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/ApplyNativePropertyChangesContext.h>
#include <Foundation/Serialization/RttiConverter.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plObjectChangeType, 1)
  PL_ENUM_CONSTANTS(plObjectChangeType::NodeAdded, plObjectChangeType::NodeRemoved)
  PL_ENUM_CONSTANTS(plObjectChangeType::PropertySet, plObjectChangeType::PropertyInserted, plObjectChangeType::PropertyRemoved)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_TYPE(plAbstractObjectNode, plNoBase, 1, plRTTIDefaultAllocator<plAbstractObjectNode>)
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_TYPE(plDiffOperation, plNoBase, 1, plRTTIDefaultAllocator<plDiffOperation>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ENUM_MEMBER_PROPERTY("Operation", plObjectChangeType, m_Operation),
    PL_MEMBER_PROPERTY("Node", m_Node),
    PL_MEMBER_PROPERTY("Property", m_sProperty),
    PL_MEMBER_PROPERTY("Index", m_Index),
    PL_MEMBER_PROPERTY("Value", m_Value),
  }
    PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

plAbstractObjectGraph::~plAbstractObjectGraph()
{
  Clear();
}

void plAbstractObjectGraph::Clear()
{
  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    PL_DEFAULT_DELETE(it.Value());
  }
  m_Nodes.Clear();
  m_NodesByName.Clear();
  m_Strings.Clear();
}


plAbstractObjectNode* plAbstractObjectGraph::Clone(plAbstractObjectGraph& ref_cloneTarget, const plAbstractObjectNode* pRootNode, FilterFunction filter) const
{
  ref_cloneTarget.Clear();

  if (pRootNode == nullptr)
  {
    for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
    {
      if (filter.IsValid())
      {
        ref_cloneTarget.CopyNodeIntoGraph(it.Value(), filter);
      }
      else
      {
        ref_cloneTarget.CopyNodeIntoGraph(it.Value());
      }
    }
    return nullptr;
  }
  else
  {
    PL_ASSERT_DEV(pRootNode->GetOwner() == this, "The given root node must be part of this document");
    plSet<plUuid> reachableNodes;
    FindTransitiveHull(pRootNode->GetGuid(), reachableNodes);

    for (const plUuid& guid : reachableNodes)
    {
      if (auto* pNode = GetNode(guid))
      {
        if (filter.IsValid())
        {
          ref_cloneTarget.CopyNodeIntoGraph(pNode, filter);
        }
        else
        {
          ref_cloneTarget.CopyNodeIntoGraph(pNode);
        }
      }
    }

    return ref_cloneTarget.GetNode(pRootNode->GetGuid());
  }
}

plStringView plAbstractObjectGraph::RegisterString(plStringView sString)
{
  auto it = m_Strings.Insert(sString);
  PL_ASSERT_DEV(it.IsValid(), "");
  return it.Key();
}

plAbstractObjectNode* plAbstractObjectGraph::GetNode(const plUuid& guid)
{
  return m_Nodes.GetValueOrDefault(guid, nullptr);
}

const plAbstractObjectNode* plAbstractObjectGraph::GetNode(const plUuid& guid) const
{
  return const_cast<plAbstractObjectGraph*>(this)->GetNode(guid);
}

const plAbstractObjectNode* plAbstractObjectGraph::GetNodeByName(plStringView sName) const
{
  return const_cast<plAbstractObjectGraph*>(this)->GetNodeByName(sName);
}

plAbstractObjectNode* plAbstractObjectGraph::GetNodeByName(plStringView sName)
{
  return m_NodesByName.GetValueOrDefault(sName, nullptr);
}

plAbstractObjectNode* plAbstractObjectGraph::AddNode(const plUuid& guid, plStringView sType, plUInt32 uiTypeVersion, plStringView sNodeName)
{
  PL_ASSERT_DEV(!m_Nodes.Contains(guid), "object {0} must not yet exist", guid);
  if (!sNodeName.IsEmpty())
  {
    sNodeName = RegisterString(sNodeName);
  }
  else
  {
    sNodeName = {};
  }

  plAbstractObjectNode* pNode = PL_DEFAULT_NEW(plAbstractObjectNode);
  pNode->m_Guid = guid;
  pNode->m_pOwner = this;
  pNode->m_sType = RegisterString(sType);
  pNode->m_uiTypeVersion = uiTypeVersion;
  pNode->m_sNodeName = sNodeName;

  m_Nodes[guid] = pNode;

  if (!sNodeName.IsEmpty())
  {
    m_NodesByName[sNodeName] = pNode;
  }

  return pNode;
}

void plAbstractObjectGraph::RemoveNode(const plUuid& guid)
{
  auto it = m_Nodes.Find(guid);

  if (it.IsValid())
  {
    plAbstractObjectNode* pNode = it.Value();
    if (!pNode->m_sNodeName.IsEmpty())
      m_NodesByName.Remove(pNode->m_sNodeName);

    m_Nodes.Remove(guid);
    PL_DEFAULT_DELETE(pNode);
  }
}

void plAbstractObjectNode::AddProperty(plStringView sName, const plVariant& value)
{
  auto& prop = m_Properties.ExpandAndGetRef();
  prop.m_sPropertyName = m_pOwner->RegisterString(sName);
  prop.m_Value = value;
}

void plAbstractObjectNode::ChangeProperty(plStringView sName, const plVariant& value)
{
  for (plUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sName)
    {
      m_Properties[i].m_Value = value;
      return;
    }
  }

  PL_REPORT_FAILURE("Property '{0}' is unknown", sName);
}

void plAbstractObjectNode::RenameProperty(plStringView sOldName, plStringView sNewName)
{
  for (plUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sOldName)
    {
      m_Properties[i].m_sPropertyName = m_pOwner->RegisterString(sNewName);
      return;
    }
  }
}

void plAbstractObjectNode::ClearProperties()
{
  m_Properties.Clear();
}

plResult plAbstractObjectNode::InlineProperty(plStringView sName)
{
  for (plUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    Property& prop = m_Properties[i];
    if (prop.m_sPropertyName == sName)
    {
      if (!prop.m_Value.IsA<plUuid>())
        return PL_FAILURE;

      plUuid guid = prop.m_Value.Get<plUuid>();
      plAbstractObjectNode* pNode = m_pOwner->GetNode(guid);
      if (!pNode)
        return PL_FAILURE;

      class InlineContext : public plRttiConverterContext
      {
      public:
        void RegisterObject(const plUuid& guid, const plRTTI* pRtti, void* pObject) override
        {
          m_SubTree.PushBack(guid);
        }
        plHybridArray<plUuid, 1> m_SubTree;
      };

      InlineContext context;
      plRttiConverterReader reader(m_pOwner, &context);
      void* pObject = reader.CreateObjectFromNode(pNode);
      if (!pObject)
        return PL_FAILURE;

      prop.m_Value.MoveTypedObject(pObject, plRTTI::FindTypeByName(pNode->GetType()));

      // Delete old objects.
      for (plUuid& uuid : context.m_SubTree)
      {
        m_pOwner->RemoveNode(uuid);
      }
      return PL_SUCCESS;
    }
  }
  return PL_FAILURE;
}

void plAbstractObjectNode::RemoveProperty(plStringView sName)
{
  for (plUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sName)
    {
      m_Properties.RemoveAtAndSwap(i);
      return;
    }
  }
}

void plAbstractObjectNode::SetType(plStringView sType)
{
  m_sType = m_pOwner->RegisterString(sType);
}

const plAbstractObjectNode::Property* plAbstractObjectNode::FindProperty(plStringView sName) const
{
  for (plUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sName)
    {
      return &m_Properties[i];
    }
  }

  return nullptr;
}

plAbstractObjectNode::Property* plAbstractObjectNode::FindProperty(plStringView sName)
{
  for (plUInt32 i = 0; i < m_Properties.GetCount(); ++i)
  {
    if (m_Properties[i].m_sPropertyName == sName)
    {
      return &m_Properties[i];
    }
  }

  return nullptr;
}

void plAbstractObjectGraph::ReMapNodeGuids(const plUuid& seedGuid, bool bRemapInverse /*= false*/)
{
  plHybridArray<plAbstractObjectNode*, 16> nodes;
  nodes.Reserve(m_Nodes.GetCount());
  plHashTable<plUuid, plUuid> guidMap;
  guidMap.Reserve(m_Nodes.GetCount());

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    plUuid newGuid = it.Key();

    if (bRemapInverse)
      newGuid.RevertCombinationWithSeed(seedGuid);
    else
      newGuid.CombineWithSeed(seedGuid);

    guidMap[it.Key()] = newGuid;

    nodes.PushBack(it.Value());
  }

  m_Nodes.Clear();

  // go through all nodes to remap guids
  for (auto* pNode : nodes)
  {
    pNode->m_Guid = guidMap[pNode->m_Guid];

    // check every property
    for (auto& prop : pNode->m_Properties)
    {
      RemapVariant(prop.m_Value, guidMap);
    }
    m_Nodes[pNode->m_Guid] = pNode;
  }
}


void plAbstractObjectGraph::ReMapNodeGuidsToMatchGraph(plAbstractObjectNode* pRoot, const plAbstractObjectGraph& rhsGraph, const plAbstractObjectNode* pRhsRoot)
{
  plHashTable<plUuid, plUuid> guidMap;
  PL_ASSERT_DEV(pRoot->GetType() == pRhsRoot->GetType(), "Roots must have the same type to be able re-map guids!");

  ReMapNodeGuidsToMatchGraphRecursive(guidMap, pRoot, rhsGraph, pRhsRoot);

  // go through all nodes to remap remaining occurrences of remapped guids
  for (auto it : m_Nodes)
  {
    // check every property
    for (auto& prop : it.Value()->m_Properties)
    {
      RemapVariant(prop.m_Value, guidMap);
    }
    m_Nodes[it.Value()->m_Guid] = it.Value();
  }
}

void plAbstractObjectGraph::ReMapNodeGuidsToMatchGraphRecursive(plHashTable<plUuid, plUuid>& guidMap, plAbstractObjectNode* lhs, const plAbstractObjectGraph& rhsGraph, const plAbstractObjectNode* rhs)
{
  if (lhs->GetType() != rhs->GetType())
  {
    // Types differ, remapping ends as this is a removal and add of a new object.
    return;
  }

  if (lhs->GetGuid() != rhs->GetGuid())
  {
    guidMap[lhs->GetGuid()] = rhs->GetGuid();
    m_Nodes.Remove(lhs->GetGuid());
    lhs->m_Guid = rhs->GetGuid();
    m_Nodes.Insert(rhs->GetGuid(), lhs);
  }

  for (plAbstractObjectNode::Property& prop : lhs->m_Properties)
  {
    if (prop.m_Value.IsA<plUuid>() && prop.m_Value.Get<plUuid>().IsValid())
    {
      // if the guid is an owned object in the graph, remap to rhs.
      auto it = m_Nodes.Find(prop.m_Value.Get<plUuid>());
      if (it.IsValid())
      {
        if (const plAbstractObjectNode::Property* rhsProp = rhs->FindProperty(prop.m_sPropertyName))
        {
          if (rhsProp->m_Value.IsA<plUuid>() && rhsProp->m_Value.Get<plUuid>().IsValid())
          {
            if (const plAbstractObjectNode* rhsPropNode = rhsGraph.GetNode(rhsProp->m_Value.Get<plUuid>()))
            {
              ReMapNodeGuidsToMatchGraphRecursive(guidMap, it.Value(), rhsGraph, rhsPropNode);
            }
          }
        }
      }
    }
    // Arrays may be of owner guids and could be remapped.
    else if (prop.m_Value.IsA<plVariantArray>())
    {
      const plVariantArray& values = prop.m_Value.Get<plVariantArray>();
      for (plUInt32 i = 0; i < values.GetCount(); i++)
      {
        auto& subValue = values[i];
        if (subValue.IsA<plUuid>() && subValue.Get<plUuid>().IsValid())
        {
          // if the guid is an owned object in the graph, remap to array element.
          auto it = m_Nodes.Find(subValue.Get<plUuid>());
          if (it.IsValid())
          {
            if (const plAbstractObjectNode::Property* rhsProp = rhs->FindProperty(prop.m_sPropertyName))
            {
              if (rhsProp->m_Value.IsA<plVariantArray>())
              {
                const plVariantArray& rhsValues = rhsProp->m_Value.Get<plVariantArray>();
                if (i < rhsValues.GetCount())
                {
                  const auto& rhsElemValue = rhsValues[i];
                  if (rhsElemValue.IsA<plUuid>() && rhsElemValue.Get<plUuid>().IsValid())
                  {
                    if (const plAbstractObjectNode* rhsPropNode = rhsGraph.GetNode(rhsElemValue.Get<plUuid>()))
                    {
                      ReMapNodeGuidsToMatchGraphRecursive(guidMap, it.Value(), rhsGraph, rhsPropNode);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    // Maps may be of owner guids and could be remapped.
    else if (prop.m_Value.IsA<plVariantDictionary>())
    {
      const plVariantDictionary& values = prop.m_Value.Get<plVariantDictionary>();
      for (auto lhsIt = values.GetIterator(); lhsIt.IsValid(); ++lhsIt)
      {
        auto& subValue = lhsIt.Value();
        if (subValue.IsA<plUuid>() && subValue.Get<plUuid>().IsValid())
        {
          // if the guid is an owned object in the graph, remap to map element.
          auto it = m_Nodes.Find(subValue.Get<plUuid>());
          if (it.IsValid())
          {
            if (const plAbstractObjectNode::Property* rhsProp = rhs->FindProperty(prop.m_sPropertyName))
            {
              if (rhsProp->m_Value.IsA<plVariantDictionary>())
              {
                const plVariantDictionary& rhsValues = rhsProp->m_Value.Get<plVariantDictionary>();
                if (rhsValues.Contains(lhsIt.Key()))
                {
                  const auto& rhsElemValue = *rhsValues.GetValue(lhsIt.Key());
                  if (rhsElemValue.IsA<plUuid>() && rhsElemValue.Get<plUuid>().IsValid())
                  {
                    if (const plAbstractObjectNode* rhsPropNode = rhsGraph.GetNode(rhsElemValue.Get<plUuid>()))
                    {
                      ReMapNodeGuidsToMatchGraphRecursive(guidMap, it.Value(), rhsGraph, rhsPropNode);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}


void plAbstractObjectGraph::FindTransitiveHull(const plUuid& rootGuid, plSet<plUuid>& ref_reachableNodes) const
{
  ref_reachableNodes.Clear();
  plSet<plUuid> inProgress;
  inProgress.Insert(rootGuid);

  while (!inProgress.IsEmpty())
  {
    plUuid current = *inProgress.GetIterator();
    auto it = m_Nodes.Find(current);
    if (it.IsValid())
    {
      const plAbstractObjectNode* pNode = it.Value();
      for (auto& prop : pNode->m_Properties)
      {
        if (prop.m_Value.IsA<plUuid>())
        {
          const plUuid& guid = prop.m_Value.Get<plUuid>();
          if (!ref_reachableNodes.Contains(guid))
          {
            inProgress.Insert(guid);
          }
        }
        // Arrays may be of uuids
        else if (prop.m_Value.IsA<plVariantArray>())
        {
          const plVariantArray& values = prop.m_Value.Get<plVariantArray>();
          for (auto& subValue : values)
          {
            if (subValue.IsA<plUuid>())
            {
              const plUuid& guid = subValue.Get<plUuid>();
              if (!ref_reachableNodes.Contains(guid))
              {
                inProgress.Insert(guid);
              }
            }
          }
        }
        else if (prop.m_Value.IsA<plVariantDictionary>())
        {
          const plVariantDictionary& values = prop.m_Value.Get<plVariantDictionary>();
          for (auto& subValue : values)
          {
            if (subValue.Value().IsA<plUuid>())
            {
              const plUuid& guid = subValue.Value().Get<plUuid>();
              if (!ref_reachableNodes.Contains(guid))
              {
                inProgress.Insert(guid);
              }
            }
          }
        }
      }
    }
    // Even if 'current' is not in the graph add it anyway to early out if it is found again.
    ref_reachableNodes.Insert(current);
    inProgress.Remove(current);
  }
}

void plAbstractObjectGraph::PruneGraph(const plUuid& rootGuid)
{
  plSet<plUuid> reachableNodes;
  FindTransitiveHull(rootGuid, reachableNodes);

  // Determine nodes to be removed by subtracting valid ones from all nodes.
  plSet<plUuid> removeSet;
  for (auto it = GetAllNodes().GetIterator(); it.IsValid(); ++it)
  {
    removeSet.Insert(it.Key());
  }
  removeSet.Difference(reachableNodes);

  // Remove nodes.
  for (const plUuid& guid : removeSet)
  {
    RemoveNode(guid);
  }
}

void plAbstractObjectGraph::ModifyNodeViaNativeCounterpart(plAbstractObjectNode* pRootNode, plDelegate<void(void*, const plRTTI*)> callback)
{
  PL_ASSERT_DEV(pRootNode->GetOwner() == this, "Node must be from this graph.");

  // Clone sub graph
  plAbstractObjectGraph origGraph;
  plAbstractObjectNode* pOrigRootNode = nullptr;
  {
    pOrigRootNode = Clone(origGraph, pRootNode);
  }

  // Create native object
  plRttiConverterContext context;
  plRttiConverterReader convRead(&origGraph, &context);
  void* pNativeRoot = convRead.CreateObjectFromNode(pOrigRootNode);
  const plRTTI* pType = plRTTI::FindTypeByName(pOrigRootNode->GetType());
  PL_SCOPE_EXIT(pType->GetAllocator()->Deallocate(pNativeRoot););

  // Make changes to native object
  if (callback.IsValid())
  {
    callback(pNativeRoot, pType);
  }

  // Create native object graph
  plAbstractObjectGraph graph;
  {
    // The plApplyNativePropertyChangesContext takes care of generating guids for native pointers that match those
    // of the object manager.
    plApplyNativePropertyChangesContext nativeChangesContext(context, origGraph);
    plRttiConverterWriter rttiConverter(&graph, &nativeChangesContext, true, true);
    nativeChangesContext.RegisterObject(pOrigRootNode->GetGuid(), pType, pNativeRoot);
    rttiConverter.AddObjectToGraph(pType, pNativeRoot, "Object");
  }

  // Create diff from native to cloned sub-graph and then apply the diff to the original graph.
  plDeque<plAbstractGraphDiffOperation> diffResult;
  graph.CreateDiffWithBaseGraph(origGraph, diffResult);

  ApplyDiff(diffResult);
}

plAbstractObjectNode* plAbstractObjectGraph::CopyNodeIntoGraph(const plAbstractObjectNode* pNode)
{
  auto pNewNode = AddNode(pNode->GetGuid(), pNode->GetType(), pNode->GetTypeVersion(), pNode->GetNodeName());

  for (const auto& props : pNode->GetProperties())
  {
    pNewNode->AddProperty(props.m_sPropertyName, props.m_Value);
  }

  return pNewNode;
}

plAbstractObjectNode* plAbstractObjectGraph::CopyNodeIntoGraph(const plAbstractObjectNode* pNode, FilterFunction& ref_filter)
{
  auto pNewNode = AddNode(pNode->GetGuid(), pNode->GetType(), pNode->GetTypeVersion(), pNode->GetNodeName());

  if (ref_filter.IsValid())
  {
    for (const auto& props : pNode->GetProperties())
    {
      if (!ref_filter(pNode, &props))
        continue;

      pNewNode->AddProperty(props.m_sPropertyName, props.m_Value);
    }
  }
  else
  {
    for (const auto& props : pNode->GetProperties())
      pNewNode->AddProperty(props.m_sPropertyName, props.m_Value);
  }

  return pNewNode;
}

void plAbstractObjectGraph::CreateDiffWithBaseGraph(const plAbstractObjectGraph& base, plDeque<plAbstractGraphDiffOperation>& out_diffResult) const
{
  out_diffResult.Clear();

  // check whether any nodes have been deleted
  {
    for (auto itNodeBase = base.GetAllNodes().GetIterator(); itNodeBase.IsValid(); ++itNodeBase)
    {
      if (GetNode(itNodeBase.Key()) == nullptr)
      {
        // does not exist in this graph -> has been deleted from base
        plAbstractGraphDiffOperation op;
        op.m_Node = itNodeBase.Key();
        op.m_Operation = plAbstractGraphDiffOperation::Op::NodeRemoved;
        op.m_sProperty = itNodeBase.Value()->m_sType;
        op.m_Value = itNodeBase.Value()->m_sNodeName;

        out_diffResult.PushBack(op);
      }
    }
  }

  // check whether any nodes have been added
  {
    for (auto itNodeThis = GetAllNodes().GetIterator(); itNodeThis.IsValid(); ++itNodeThis)
    {
      if (base.GetNode(itNodeThis.Key()) == nullptr)
      {
        // does not exist in base graph -> has been added
        plAbstractGraphDiffOperation op;
        op.m_Node = itNodeThis.Key();
        op.m_Operation = plAbstractGraphDiffOperation::Op::NodeAdded;
        op.m_sProperty = itNodeThis.Value()->m_sType;
        op.m_Value = itNodeThis.Value()->m_sNodeName;

        out_diffResult.PushBack(op);

        // set all properties
        for (const auto& prop : itNodeThis.Value()->GetProperties())
        {
          op.m_Operation = plAbstractGraphDiffOperation::Op::PropertyChanged;
          op.m_sProperty = prop.m_sPropertyName;
          op.m_Value = prop.m_Value;

          out_diffResult.PushBack(op);
        }
      }
    }
  }

  // check whether any properties have been modified
  {
    for (auto itNodeThis = GetAllNodes().GetIterator(); itNodeThis.IsValid(); ++itNodeThis)
    {
      const auto pBaseNode = base.GetNode(itNodeThis.Key());

      if (pBaseNode == nullptr)
        continue;

      for (const plAbstractObjectNode::Property& prop : itNodeThis.Value()->GetProperties())
      {
        bool bDifferent = true;

        for (const plAbstractObjectNode::Property& baseProp : pBaseNode->GetProperties())
        {
          if (baseProp.m_sPropertyName == prop.m_sPropertyName)
          {
            if (baseProp.m_Value == prop.m_Value)
            {
              bDifferent = false;
              break;
            }

            bDifferent = true;
            break;
          }
        }

        if (bDifferent)
        {
          plAbstractGraphDiffOperation op;
          op.m_Node = itNodeThis.Key();
          op.m_Operation = plAbstractGraphDiffOperation::Op::PropertyChanged;
          op.m_sProperty = prop.m_sPropertyName;
          op.m_Value = prop.m_Value;

          out_diffResult.PushBack(op);
        }
      }
    }
  }
}


void plAbstractObjectGraph::ApplyDiff(plDeque<plAbstractGraphDiffOperation>& ref_diff)
{
  for (const auto& op : ref_diff)
  {
    switch (op.m_Operation)
    {
      case plAbstractGraphDiffOperation::Op::NodeAdded:
      {
        AddNode(op.m_Node, op.m_sProperty, op.m_uiTypeVersion, op.m_Value.Get<plString>());
      }
      break;

      case plAbstractGraphDiffOperation::Op::NodeRemoved:
      {
        RemoveNode(op.m_Node);
      }
      break;

      case plAbstractGraphDiffOperation::Op::PropertyChanged:
      {
        auto* pNode = GetNode(op.m_Node);
        if (pNode)
        {
          auto* pProp = pNode->FindProperty(op.m_sProperty);

          if (!pProp)
            pNode->AddProperty(op.m_sProperty, op.m_Value);
          else
            pProp->m_Value = op.m_Value;
        }
      }
      break;
    }
  }
}


void plAbstractObjectGraph::MergeDiffs(const plDeque<plAbstractGraphDiffOperation>& lhs, const plDeque<plAbstractGraphDiffOperation>& rhs, plDeque<plAbstractGraphDiffOperation>& ref_out) const
{
  struct Prop
  {
    Prop() = default;
    Prop(plUuid node, plStringView sProperty)
      : m_Node(node)
      , m_sProperty(sProperty)
    {
    }
    plUuid m_Node;
    plStringView m_sProperty;

    bool operator<(const Prop& rhs) const
    {
      if (m_Node == rhs.m_Node)
        return m_sProperty < rhs.m_sProperty;

      return m_Node < rhs.m_Node;
    }

    bool operator==(const Prop& rhs) const { return m_Node == rhs.m_Node && m_sProperty == rhs.m_sProperty; }
  };

  plMap<Prop, plHybridArray<const plAbstractGraphDiffOperation*, 2>> propChanges;
  plSet<plUuid> removed;
  plMap<plUuid, plUInt32> added;
  for (const plAbstractGraphDiffOperation& op : lhs)
  {
    if (op.m_Operation == plAbstractGraphDiffOperation::Op::NodeRemoved)
    {
      removed.Insert(op.m_Node);
      ref_out.PushBack(op);
    }
    else if (op.m_Operation == plAbstractGraphDiffOperation::Op::NodeAdded)
    {
      added[op.m_Node] = ref_out.GetCount();
      ref_out.PushBack(op);
    }
    else if (op.m_Operation == plAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      auto it = propChanges.FindOrAdd(Prop(op.m_Node, op.m_sProperty));
      it.Value().PushBack(&op);
    }
  }
  for (const plAbstractGraphDiffOperation& op : rhs)
  {
    if (op.m_Operation == plAbstractGraphDiffOperation::Op::NodeRemoved)
    {
      if (!removed.Contains(op.m_Node))
        ref_out.PushBack(op);
    }
    else if (op.m_Operation == plAbstractGraphDiffOperation::Op::NodeAdded)
    {
      if (added.Contains(op.m_Node))
      {
        plAbstractGraphDiffOperation& leftOp = ref_out[added[op.m_Node]];
        leftOp.m_sProperty = op.m_sProperty; // Take type from rhs.
      }
      else
      {
        ref_out.PushBack(op);
      }
    }
    else if (op.m_Operation == plAbstractGraphDiffOperation::Op::PropertyChanged)
    {
      auto it = propChanges.FindOrAdd(Prop(op.m_Node, op.m_sProperty));
      it.Value().PushBack(&op);
    }
  }

  for (auto it = propChanges.GetIterator(); it.IsValid(); ++it)
  {
    const Prop& key = it.Key();
    const plHybridArray<const plAbstractGraphDiffOperation*, 2>& value = it.Value();

    if (value.GetCount() == 1)
    {
      ref_out.PushBack(*value[0]);
    }
    else
    {
      const plAbstractGraphDiffOperation& leftProp = *value[0];
      const plAbstractGraphDiffOperation& rightProp = *value[1];

      if (leftProp.m_Value.GetType() == plVariantType::VariantArray && rightProp.m_Value.GetType() == plVariantType::VariantArray)
      {
        const plVariantArray& leftArray = leftProp.m_Value.Get<plVariantArray>();
        const plVariantArray& rightArray = rightProp.m_Value.Get<plVariantArray>();

        const plAbstractObjectNode* pNode = GetNode(key.m_Node);
        if (pNode)
        {
          plStringBuilder sTemp(key.m_sProperty);
          const plAbstractObjectNode::Property* pProperty = pNode->FindProperty(sTemp);
          if (pProperty && pProperty->m_Value.GetType() == plVariantType::VariantArray)
          {
            // Do 3-way array merge
            const plVariantArray& baseArray = pProperty->m_Value.Get<plVariantArray>();
            plVariantArray res;
            MergeArrays(baseArray, leftArray, rightArray, res);
            ref_out.PushBack(rightProp);
            ref_out.PeekBack().m_Value = res;
          }
          else
          {
            ref_out.PushBack(rightProp);
          }
        }
        else
        {
          ref_out.PushBack(rightProp);
        }
      }
      else
      {
        ref_out.PushBack(rightProp);
      }
    }
  }
}

void plAbstractObjectGraph::RemapVariant(plVariant& value, const plHashTable<plUuid, plUuid>& guidMap)
{
  plStringBuilder tmp;

  // if the property is a guid, we check if we need to remap it
  if (value.IsA<plUuid>())
  {
    const plUuid& guid = value.Get<plUuid>();

    // if we find the guid in our map, replace it by the new guid
    if (auto* found = guidMap.GetValue(guid))
    {
      value = *found;
    }
  }
  else if (value.IsA<plString>() && plConversionUtils::IsStringUuid(value.Get<plString>()))
  {
    const plUuid guid = plConversionUtils::ConvertStringToUuid(value.Get<plString>());

    // if we find the guid in our map, replace it by the new guid
    if (auto* found = guidMap.GetValue(guid))
    {
      value = plConversionUtils::ToString(*found, tmp).GetData();
    }
  }
  // Arrays may be of uuids
  else if (value.IsA<plVariantArray>())
  {
    const plVariantArray& values = value.Get<plVariantArray>();
    bool bNeedToRemap = false;
    for (auto& subValue : values)
    {
      if (subValue.IsA<plUuid>() && guidMap.Contains(subValue.Get<plUuid>()))
      {
        bNeedToRemap = true;
        break;
      }
      else if (subValue.IsA<plString>() && plConversionUtils::IsStringUuid(subValue.Get<plString>()))
      {
        bNeedToRemap = true;
        break;
      }
      else if (subValue.IsA<plVariantArray>())
      {
        bNeedToRemap = true;
        break;
      }
    }

    if (bNeedToRemap)
    {
      plVariantArray newValues = values;
      for (auto& subValue : newValues)
      {
        RemapVariant(subValue, guidMap);
      }
      value = newValues;
    }
  }
  // Maps may be of uuids
  else if (value.IsA<plVariantDictionary>())
  {
    const plVariantDictionary& values = value.Get<plVariantDictionary>();
    bool bNeedToRemap = false;
    for (auto it = values.GetIterator(); it.IsValid(); ++it)
    {
      const plVariant& subValue = it.Value();

      if (subValue.IsA<plUuid>() && guidMap.Contains(subValue.Get<plUuid>()))
      {
        bNeedToRemap = true;
        break;
      }
      else if (subValue.IsA<plString>() && plConversionUtils::IsStringUuid(subValue.Get<plString>()))
      {
        bNeedToRemap = true;
        break;
      }
    }

    if (bNeedToRemap)
    {
      plVariantDictionary newValues = values;
      for (auto it = newValues.GetIterator(); it.IsValid(); ++it)
      {
        RemapVariant(it.Value(), guidMap);
      }
      value = newValues;
    }
  }
}

void plAbstractObjectGraph::MergeArrays(const plDynamicArray<plVariant>& baseArray, const plDynamicArray<plVariant>& leftArray, const plDynamicArray<plVariant>& rightArray, plDynamicArray<plVariant>& out) const
{
  // Find element type.
  plVariantType::Enum type = plVariantType::Invalid;
  if (!baseArray.IsEmpty())
    type = baseArray[0].GetType();
  if (type != plVariantType::Invalid && !leftArray.IsEmpty())
    type = leftArray[0].GetType();
  if (type != plVariantType::Invalid && !rightArray.IsEmpty())
    type = rightArray[0].GetType();

  if (type == plVariantType::Invalid)
    return;

  // For now, assume non-uuid types are arrays, uuids are sets.
  if (type != plVariantType::Uuid)
  {
    // Any size changes?
    plUInt32 uiSize = baseArray.GetCount();
    if (leftArray.GetCount() != baseArray.GetCount())
      uiSize = leftArray.GetCount();
    if (rightArray.GetCount() != baseArray.GetCount())
      uiSize = rightArray.GetCount();

    out.SetCount(uiSize);
    for (plUInt32 i = 0; i < uiSize; i++)
    {
      if (i < baseArray.GetCount())
        out[i] = baseArray[i];
    }

    plUInt32 uiCountLeft = plMath::Min(uiSize, leftArray.GetCount());
    for (plUInt32 i = 0; i < uiCountLeft; i++)
    {
      if (leftArray[i] != baseArray[i])
        out[i] = leftArray[i];
    }

    plUInt32 uiCountRight = plMath::Min(uiSize, rightArray.GetCount());
    for (plUInt32 i = 0; i < uiCountRight; i++)
    {
      if (rightArray[i] != baseArray[i])
        out[i] = rightArray[i];
    }
    return;
  }

  // Move distance is NP-complete so try greedy algorithm
  struct Element
  {
    Element(const plVariant* pValue = nullptr, plInt32 iBaseIndex = -1, plInt32 iLeftIndex = -1, plInt32 iRightIndex = -1)
      : m_pValue(pValue)
      , m_iBaseIndex(iBaseIndex)
      , m_iLeftIndex(iLeftIndex)
      , m_iRightIndex(iRightIndex)
      , m_fIndex(plMath::MaxValue<float>())
    {
    }
    bool IsDeleted() const { return m_iBaseIndex != -1 && (m_iLeftIndex == -1 || m_iRightIndex == -1); }
    bool operator<(const Element& rhs) const { return m_fIndex < rhs.m_fIndex; }

    const plVariant* m_pValue;
    plInt32 m_iBaseIndex;
    plInt32 m_iLeftIndex;
    plInt32 m_iRightIndex;
    float m_fIndex;
  };
  plDynamicArray<Element> baseOrder;
  baseOrder.Reserve(leftArray.GetCount() + rightArray.GetCount());

  // First, add up all unique elements and their position in each array.
  for (plInt32 i = 0; i < (plInt32)baseArray.GetCount(); i++)
  {
    baseOrder.PushBack(Element(&baseArray[i], i));
    baseOrder.PeekBack().m_fIndex = (float)i;
  }

  plDynamicArray<plInt32> leftOrder;
  leftOrder.SetCountUninitialized(leftArray.GetCount());
  for (plInt32 i = 0; i < (plInt32)leftArray.GetCount(); i++)
  {
    const plVariant& val = leftArray[i];
    bool bFound = false;
    for (plInt32 j = 0; j < (plInt32)baseOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[j];
      if (elem.m_iLeftIndex == -1 && *elem.m_pValue == val)
      {
        elem.m_iLeftIndex = i;
        leftOrder[i] = j;
        bFound = true;
        break;
      }
    }

    if (!bFound)
    {
      // Added element.
      leftOrder[i] = (plInt32)baseOrder.GetCount();
      baseOrder.PushBack(Element(&leftArray[i], -1, i));
    }
  }

  plDynamicArray<plInt32> rightOrder;
  rightOrder.SetCountUninitialized(rightArray.GetCount());
  for (plInt32 i = 0; i < (plInt32)rightArray.GetCount(); i++)
  {
    const plVariant& val = rightArray[i];
    bool bFound = false;
    for (plInt32 j = 0; j < (plInt32)baseOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[j];
      if (elem.m_iRightIndex == -1 && *elem.m_pValue == val)
      {
        elem.m_iRightIndex = i;
        rightOrder[i] = j;
        bFound = true;
        break;
      }
    }

    if (!bFound)
    {
      // Added element.
      rightOrder[i] = (plInt32)baseOrder.GetCount();
      baseOrder.PushBack(Element(&rightArray[i], -1, -1, i));
    }
  }

  // Re-order greedy
  float fLastElement = -0.5f;
  for (plInt32 i = 0; i < (plInt32)leftOrder.GetCount(); i++)
  {
    Element& currentElem = baseOrder[leftOrder[i]];
    if (currentElem.IsDeleted())
      continue;

    float fLowestSubsequent = plMath::MaxValue<float>();
    for (plInt32 j = i + 1; j < (plInt32)leftOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[leftOrder[j]];
      if (elem.IsDeleted())
        continue;

      if (elem.m_iBaseIndex < fLowestSubsequent)
      {
        fLowestSubsequent = (float)elem.m_iBaseIndex;
      }
    }

    if (currentElem.m_fIndex >= fLowestSubsequent)
    {
      currentElem.m_fIndex = (fLowestSubsequent + fLastElement) / 2.0f;
    }

    fLastElement = currentElem.m_fIndex;
  }

  fLastElement = -0.5f;
  for (plInt32 i = 0; i < (plInt32)rightOrder.GetCount(); i++)
  {
    Element& currentElem = baseOrder[rightOrder[i]];
    if (currentElem.IsDeleted())
      continue;

    float fLowestSubsequent = plMath::MaxValue<float>();
    for (plInt32 j = i + 1; j < (plInt32)rightOrder.GetCount(); j++)
    {
      Element& elem = baseOrder[rightOrder[j]];
      if (elem.IsDeleted())
        continue;

      if (elem.m_iBaseIndex < fLowestSubsequent)
      {
        fLowestSubsequent = (float)elem.m_iBaseIndex;
      }
    }

    if (currentElem.m_fIndex >= fLowestSubsequent)
    {
      currentElem.m_fIndex = (fLowestSubsequent + fLastElement) / 2.0f;
    }

    fLastElement = currentElem.m_fIndex;
  }


  // Sort
  baseOrder.Sort();
  out.Reserve(baseOrder.GetCount());
  for (const Element& elem : baseOrder)
  {
    if (!elem.IsDeleted())
    {
      out.PushBack(*elem.m_pValue);
    }
  }
}

PL_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_AbstractObjectGraph);
