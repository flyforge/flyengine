#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAsset.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphQt.h>
#include <Foundation/Math/ColorScheme.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/PoseResultAnimNode.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/SampleFrameAnimNode.h>
#include <ToolsFoundation/NodeObject/NodeCommandAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimationGraphAssetDocument, 5, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimationGraphNodePin, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimationGraphAssetProperties, 1, plRTTIDefaultAllocator<plAnimationGraphAssetProperties>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY("IncludeGraphs", m_IncludeGraphs)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Keyframe_Graph")),
    PLASMA_ARRAY_MEMBER_PROPERTY("AnimationClipMapping", m_AnimationClipMapping),
  }
    PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, AnimationGraph)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plQtNodeScene::GetNodeFactory().RegisterCreator(plGetStaticRTTI<plAnimGraphNode>(), [](const plRTTI* pRtti)->plQtNode* { return new plQtAnimationGraphNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plQtNodeScene::GetNodeFactory().UnregisterCreator(plGetStaticRTTI<plAnimGraphNode>());
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

bool plAnimationGraphNodeManager::InternalIsNode(const plDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom<plAnimGraphNode>();
}

void plAnimationGraphNodeManager::InternalCreatePins(const plDocumentObject* pObject, NodeInternal& ref_node)
{
  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom<plAnimGraphNode>())
    return;

  plHybridArray<const plAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  const plColor triggerPinColor = plColorScheme::DarkUI(plColorScheme::Yellow);
  const plColor numberPinColor = plColorScheme::DarkUI(plColorScheme::Lime);
  const plColor boolPinColor = plColorScheme::LightUI(plColorScheme::Lime);
  const plColor weightPinColor = plColorScheme::DarkUI(plColorScheme::Teal);
  const plColor localPosePinColor = plColorScheme::DarkUI(plColorScheme::Blue);
  const plColor modelPosePinColor = plColorScheme::DarkUI(plColorScheme::Grape);
  // EXTEND THIS if a new type is introduced

  plHybridArray<plString, 16> pinNames;

  for (auto pProp : properties)
  {
    if (!pProp->GetSpecificType()->IsDerivedFrom<plAnimGraphPin>())
      continue;

    pinNames.Clear();

    if (pProp->GetCategory() == plPropertyCategory::Array)
    {
      if (const plDynamicPinAttribute* pDynPin = pProp->GetAttributeByType<plDynamicPinAttribute>())
      {
        GetDynamicPinNames(pObject, pDynPin->GetProperty(), pProp->GetPropertyName(), pinNames);
      }
    }
    else if (pProp->GetCategory() == plPropertyCategory::Member)
    {
      pinNames.PushBack(pProp->GetPropertyName());
    }

    for (plUInt32 i = 0; i < pinNames.GetCount(); ++i)
    {
      const auto& pinName = pinNames[i];

      if (pProp->GetSpecificType()->IsDerivedFrom<plAnimGraphTriggerInputPin>())
      {
        auto pPin = PLASMA_DEFAULT_NEW(plAnimationGraphNodePin, plPin::Type::Input, pinName, triggerPinColor, pObject);
        pPin->m_DataType = plAnimGraphPin::Trigger;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<plAnimGraphTriggerOutputPin>())
      {
        auto pPin = PLASMA_DEFAULT_NEW(plAnimationGraphNodePin, plPin::Type::Output, pinName, triggerPinColor, pObject);
        pPin->m_DataType = plAnimGraphPin::Trigger;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<plAnimGraphNumberInputPin>())
      {
        auto pPin = PLASMA_DEFAULT_NEW(plAnimationGraphNodePin, plPin::Type::Input, pinName, numberPinColor, pObject);
        pPin->m_DataType = plAnimGraphPin::Number;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<plAnimGraphNumberOutputPin>())
      {
        auto pPin = PLASMA_DEFAULT_NEW(plAnimationGraphNodePin, plPin::Type::Output, pinName, numberPinColor, pObject);
        pPin->m_DataType = plAnimGraphPin::Number;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<plAnimGraphBoolInputPin>())
      {
        auto pPin = PLASMA_DEFAULT_NEW(plAnimationGraphNodePin, plPin::Type::Input, pinName, boolPinColor, pObject);
        pPin->m_DataType = plAnimGraphPin::Bool;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<plAnimGraphBoolOutputPin>())
      {
        auto pPin = PLASMA_DEFAULT_NEW(plAnimationGraphNodePin, plPin::Type::Output, pinName, boolPinColor, pObject);
        pPin->m_DataType = plAnimGraphPin::Bool;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<plAnimGraphBoneWeightsInputPin>())
      {
        auto pPin = PLASMA_DEFAULT_NEW(plAnimationGraphNodePin, plPin::Type::Input, pinName, weightPinColor, pObject);
        pPin->m_DataType = plAnimGraphPin::BoneWeights;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<plAnimGraphBoneWeightsOutputPin>())
      {
        auto pPin = PLASMA_DEFAULT_NEW(plAnimationGraphNodePin, plPin::Type::Output, pinName, weightPinColor, pObject);
        pPin->m_DataType = plAnimGraphPin::BoneWeights;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<plAnimGraphLocalPoseInputPin>())
      {
        auto pPin = PLASMA_DEFAULT_NEW(plAnimationGraphNodePin, plPin::Type::Input, pinName, localPosePinColor, pObject);
        pPin->m_DataType = plAnimGraphPin::LocalPose;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<plAnimGraphLocalPoseMultiInputPin>())
      {
        auto pPin = PLASMA_DEFAULT_NEW(plAnimationGraphNodePin, plPin::Type::Input, pinName, localPosePinColor, pObject);
        pPin->m_DataType = plAnimGraphPin::LocalPose;
        pPin->m_bMultiInputPin = true;
        pPin->m_Shape = plPin::Shape::RoundRect;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<plAnimGraphLocalPoseOutputPin>())
      {
        auto pPin = PLASMA_DEFAULT_NEW(plAnimationGraphNodePin, plPin::Type::Output, pinName, localPosePinColor, pObject);
        pPin->m_DataType = plAnimGraphPin::LocalPose;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<plAnimGraphModelPoseInputPin>())
      {
        auto pPin = PLASMA_DEFAULT_NEW(plAnimationGraphNodePin, plPin::Type::Input, pinName, modelPosePinColor, pObject);
        pPin->m_DataType = plAnimGraphPin::ModelPose;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<plAnimGraphModelPoseOutputPin>())
      {
        auto pPin = PLASMA_DEFAULT_NEW(plAnimationGraphNodePin, plPin::Type::Output, pinName, modelPosePinColor, pObject);
        pPin->m_DataType = plAnimGraphPin::ModelPose;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else
      {
        // EXTEND THIS if a new type is introduced
        PLASMA_ASSERT_NOT_IMPLEMENTED;
      }
    }
  }
}

void plAnimationGraphNodeManager::GetCreateableTypes(plHybridArray<const plRTTI*, 32>& ref_types) const
{
  plSet<const plRTTI*> typeSet;
  plReflectionUtils::GatherTypesDerivedFromClass(plGetStaticRTTI<plAnimGraphNode>(), typeSet);

  ref_types.Clear();
  for (auto pType : typeSet)
  {
    if (pType->GetTypeFlags().IsAnySet(plTypeFlags::Abstract))
      continue;

    ref_types.PushBack(pType);
  }
}

plStatus plAnimationGraphNodeManager::InternalCanConnect(const plPin& source, const plPin& target, CanConnectResult& out_result) const
{
  const plAnimationGraphNodePin& sourcePin = plStaticCast<const plAnimationGraphNodePin&>(source);
  const plAnimationGraphNodePin& targetPin = plStaticCast<const plAnimationGraphNodePin&>(target);

  out_result = CanConnectResult::ConnectNever;

  if (sourcePin.m_DataType != targetPin.m_DataType)
    return plStatus("Can't connect pins of different data types");

  if (sourcePin.GetType() == targetPin.GetType())
    return plStatus("Can only connect input pins with output pins.");

  switch (sourcePin.m_DataType)
  {
    case plAnimGraphPin::Trigger:
      out_result = CanConnectResult::ConnectNtoN;
      break;

    case plAnimGraphPin::Number:
      out_result = CanConnectResult::ConnectNto1;
      break;

    case plAnimGraphPin::Bool:
      out_result = CanConnectResult::ConnectNto1;
      break;

    case plAnimGraphPin::BoneWeights:
      out_result = CanConnectResult::ConnectNto1;
      break;

    case plAnimGraphPin::LocalPose:
      if (targetPin.m_bMultiInputPin)
        out_result = CanConnectResult::ConnectNtoN;
      else
        out_result = CanConnectResult::ConnectNto1;
      break;

    case plAnimGraphPin::ModelPose:
      out_result = CanConnectResult::ConnectNto1;
      break;

      // EXTEND THIS if a new type is introduced
      PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (out_result != CanConnectResult::ConnectNever && WouldConnectionCreateCircle(source, target))
  {
    out_result = CanConnectResult::ConnectNever;
    return plStatus("Connecting these pins would create a circle in the graph.");
  }

  return plStatus(PLASMA_SUCCESS);
}

bool plAnimationGraphNodeManager::InternalIsDynamicPinProperty(const plDocumentObject* pObject, const plAbstractProperty* pProp) const
{
  return pProp->GetAttributeByType<plDynamicPinAttribute>() != nullptr;
}

plAnimationGraphAssetDocument::plAnimationGraphAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument<plAnimationGraphAssetProperties>(PLASMA_DEFAULT_NEW(plAnimationGraphNodeManager), sDocumentPath, plAssetDocEngineConnection::None)
{
  m_pObjectAccessor = PLASMA_DEFAULT_NEW(plNodeCommandAccessor, GetCommandHistory());
}

plTransformStatus plAnimationGraphAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  const auto* pNodeManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());

  auto pProp = GetProperties();

  {
    stream.WriteVersion(2);
    stream.WriteArray(pProp->m_IncludeGraphs).AssertSuccess();

    const plUInt32 uiNum = pProp->m_AnimationClipMapping.GetCount();
    stream << uiNum;

    for (plUInt32 i = 0; i < uiNum; ++i)
    {
      stream << pProp->m_AnimationClipMapping[i].m_sClipName;
      stream << pProp->m_AnimationClipMapping[i].m_hClip;
    }
  }

  // find all 'nodes'
  plDynamicArray<const plDocumentObject*> allNodes;
  for (auto pNode : pNodeManager->GetRootObject()->GetChildren())
  {
    if (!pNodeManager->IsNode(pNode))
      continue;

    allNodes.PushBack(pNode);
  }

  plAnimGraph animGraph;

  plMap<const plDocumentObject*, plAnimGraphNode*> docNodeToRuntimeNode;

  // create all nodes in the plAnimGraph
  {
    for (const plDocumentObject* pNode : allNodes)
    {
      plAnimGraphNode* pNewNode = animGraph.AddNode(pNode->GetType()->GetAllocator()->Allocate<plAnimGraphNode>());

      // copy all the non-hidden properties
      plToolsSerializationUtils::CopyProperties(pNode, GetObjectManager(), pNewNode, pNewNode->GetDynamicRTTI(), [](const plAbstractProperty* p) { return p->GetAttributeByType<plHiddenAttribute>() == nullptr; });

      docNodeToRuntimeNode[pNode] = pNewNode;
    }
  }

  // add all node connections to the plAnimGraph
  {
    for (plUInt32 nodeIdx = 0; nodeIdx < allNodes.GetCount(); ++nodeIdx)
    {
      const plDocumentObject* pNode = allNodes[nodeIdx];

      const auto outputPins = pNodeManager->GetOutputPins(pNode);

      for (auto& pPin : outputPins)
      {
        for (const plConnection* pCon : pNodeManager->GetConnections(*pPin))
        {
          const plAnimGraphNode* pSrcNode = docNodeToRuntimeNode[pCon->GetSourcePin().GetParent()];
          plAnimGraphNode* pDstNode = docNodeToRuntimeNode[pCon->GetTargetPin().GetParent()];

          animGraph.AddConnection(pSrcNode, pCon->GetSourcePin().GetName(), pDstNode, pCon->GetTargetPin().GetName());
        }
      }
    }
  }

  PLASMA_SUCCEED_OR_RETURN(animGraph.Serialize(stream));

  return plTransformStatus(PLASMA_SUCCESS);
}

void plAnimationGraphAssetDocument::InternalGetMetaDataHash(const plDocumentObject* pObject, plUInt64& inout_uiHash) const
{
  // without this, changing connections only (no property value) may not result in a different asset document hash and therefore no transform

  const plDocumentNodeManager* pManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void plAnimationGraphAssetDocument::AttachMetaDataBeforeSaving(plAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const plDocumentNodeManager* pManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void plAnimationGraphAssetDocument::RestoreMetaDataAfterLoading(const plAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  plDocumentNodeManager* pManager = static_cast<plDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}



void plAnimationGraphAssetDocument::GetSupportedMimeTypesForPasting(plHybridArray<plString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/plEditor.AnimationGraphGraph");
}

bool plAnimationGraphAssetDocument::CopySelectedObjects(plAbstractObjectGraph& out_objectGraph, plStringBuilder& out_MimeType) const
{
  out_MimeType = "application/plEditor.AnimationGraphGraph";

  const plDocumentNodeManager* pManager = static_cast<const plDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool plAnimationGraphAssetDocument::Paste(const plArrayPtr<PasteInfo>& info, const plAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, plStringView sMimeType)
{
  plDocumentNodeManager* pManager = static_cast<plDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, plQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}

plAnimationGraphNodePin::plAnimationGraphNodePin(Type type, const char* szName, const plColorGammaUB& color, const plDocumentObject* pObject)
  : plPin(type, szName, color, pObject)
{
}

plAnimationGraphNodePin::~plAnimationGraphNodePin() = default;
