#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class PL_RENDERERCORE_DLL plAnimGraph
{
  PL_DISALLOW_COPY_AND_ASSIGN(plAnimGraph);

public:
  plAnimGraph();
  ~plAnimGraph();

  void Clear();

  plAnimGraphNode* AddNode(plUniquePtr<plAnimGraphNode>&& pNode);
  void AddConnection(const plAnimGraphNode* pSrcNode, plStringView sSrcPinName, plAnimGraphNode* pDstNode, plStringView sDstPinName);

  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);

  const plInstanceDataAllocator& GetInstanceDataAlloator() const { return m_InstanceDataAllocator; }
  plArrayPtr<const plUniquePtr<plAnimGraphNode>> GetNodes() const { return m_Nodes; }

  void PrepareForUse();

private:
  friend class plAnimGraphInstance;

  struct ConnectionTo
  {
    plString m_sSrcPinName;
    const plAnimGraphNode* m_pDstNode = nullptr;
    plString m_sDstPinName;
    plAnimGraphPin* m_pSrcPin = nullptr;
    plAnimGraphPin* m_pDstPin = nullptr;
  };

  struct ConnectionsTo
  {
    plHybridArray<ConnectionTo, 2> m_To;
  };

  void SortNodesByPriority();
  void PreparePinMapping();
  void AssignInputPinIndices();
  void AssignOutputPinIndices();
  plUInt16 ComputeNodePriority(const plAnimGraphNode* pNode, plMap<const plAnimGraphNode*, plUInt16>& inout_Prios, plUInt16& inout_uiOutputPrio) const;

  bool m_bPreparedForUse = true;
  plUInt32 m_uiInputPinCounts[plAnimGraphPin::Type::ENUM_COUNT];
  plUInt32 m_uiPinInstanceDataOffset[plAnimGraphPin::Type::ENUM_COUNT];
  plMap<const plAnimGraphNode*, ConnectionsTo> m_From;

  plDynamicArray<plUniquePtr<plAnimGraphNode>> m_Nodes;
  plDynamicArray<plHybridArray<plUInt16, 1>> m_OutputPinToInputPinMapping[plAnimGraphPin::ENUM_COUNT];
  plInstanceDataAllocator m_InstanceDataAllocator;

  friend class plAnimGraphTriggerOutputPin;
  friend class plAnimGraphNumberOutputPin;
  friend class plAnimGraphBoolOutputPin;
  friend class plAnimGraphBoneWeightsOutputPin;
  friend class plAnimGraphLocalPoseOutputPin;
  friend class plAnimGraphModelPoseOutputPin;
};
