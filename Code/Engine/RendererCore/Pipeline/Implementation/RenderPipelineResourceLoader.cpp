#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Serialization/BinarySerializer.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>

////////////////////////////////////////////////////////////////////////
// plDocumentNodeManager Internal
////////////////////////////////////////////////////////////////////////

struct RenderPipelineResourceLoaderConnectionInternal
{
  plUuid m_Source;
  plUuid m_Target;
  plString m_SourcePin;
  plString m_TargetPin;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, RenderPipelineResourceLoaderConnectionInternal);

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(RenderPipelineResourceLoaderConnectionInternal, plNoBase, 1, plRTTIDefaultAllocator<RenderPipelineResourceLoaderConnectionInternal>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Connection::Source", m_Source),
    PLASMA_MEMBER_PROPERTY("Connection::Target", m_Target),
    PLASMA_MEMBER_PROPERTY("Connection::SourcePin", m_SourcePin),    
    PLASMA_MEMBER_PROPERTY("Connection::TargetPin", m_TargetPin),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

void plRenderPipelineRttiConverterContext::Clear()
{
  plRttiConverterContext::Clear();

  m_pRenderPipeline = nullptr;
}

plInternal::NewInstance<void> plRenderPipelineRttiConverterContext::CreateObject(const plUuid& guid, const plRTTI* pRtti)
{
  PLASMA_ASSERT_DEBUG(pRtti != nullptr, "Object type is unknown");

  if (pRtti->IsDerivedFrom<plRenderPipelinePass>())
  {
    if (!pRtti->GetAllocator()->CanAllocate())
    {
      plLog::Error("Failed to create plRenderPipelinePass because '{0}' cannot allocate!", pRtti->GetTypeName());
      return nullptr;
    }

    auto pass = pRtti->GetAllocator()->Allocate<plRenderPipelinePass>();
    m_pRenderPipeline->AddPass(pass);

    RegisterObject(guid, pRtti, pass);
    return pass;
  }
  else if (pRtti->IsDerivedFrom<plExtractor>())
  {
    if (!pRtti->GetAllocator()->CanAllocate())
    {
      plLog::Error("Failed to create plExtractor because '{0}' cannot allocate!", pRtti->GetTypeName());
      return nullptr;
    }

    auto extractor = pRtti->GetAllocator()->Allocate<plExtractor>();
    m_pRenderPipeline->AddExtractor(extractor);

    RegisterObject(guid, pRtti, extractor);
    return extractor;
  }
  else
  {
    return plRttiConverterContext::CreateObject(guid, pRtti);
  }
}

void plRenderPipelineRttiConverterContext::DeleteObject(const plUuid& guid)
{
  plRttiConverterObject object = GetObjectByGUID(guid);
  const plRTTI* pRtti = object.m_pType;
  PLASMA_ASSERT_DEBUG(pRtti != nullptr, "Object does not exist!");
  if (pRtti->IsDerivedFrom<plRenderPipelinePass>())
  {
    plRenderPipelinePass* pPass = static_cast<plRenderPipelinePass*>(object.m_pObject);

    UnregisterObject(guid);
    m_pRenderPipeline->RemovePass(pPass);
  }
  else if (pRtti->IsDerivedFrom<plExtractor>())
  {
    plExtractor* pExtractor = static_cast<plExtractor*>(object.m_pObject);

    UnregisterObject(guid);
    m_pRenderPipeline->RemoveExtractor(pExtractor);
  }
  else
  {
    plRttiConverterContext::DeleteObject(guid);
  }
}

// static
plInternal::NewInstance<plRenderPipeline> plRenderPipelineResourceLoader::CreateRenderPipeline(const plRenderPipelineResourceDescriptor& desc)
{
  auto pPipeline = PLASMA_DEFAULT_NEW(plRenderPipeline);
  plRenderPipelineRttiConverterContext context;
  context.m_pRenderPipeline = pPipeline;

  plRawMemoryStreamReader memoryReader(desc.m_SerializedPipeline);

  plAbstractObjectGraph graph;
  plAbstractGraphBinarySerializer::Read(memoryReader, &graph);

  plRttiConverterReader rttiConverter(&graph, &context);

  auto& nodes = graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto pNode = it.Value();
    const plRTTI* pType = plRTTI::FindTypeByName(pNode->GetType());
    if (pType && pType->IsDerivedFrom<plRenderPipelinePass>())
    {
      auto pPass = rttiConverter.CreateObjectFromNode(pNode);
      if (!pPass)
      {
        plLog::Error("Failed to deserialize plRenderPipelinePass!");
      }
    }
    else if (pType && pType->IsDerivedFrom<plExtractor>())
    {
      auto pExtractor = rttiConverter.CreateObjectFromNode(pNode);
      if (!pExtractor)
      {
        plLog::Error("Failed to deserialize plExtractor!");
      }
    }
  }

  auto pType = plGetStaticRTTI<RenderPipelineResourceLoaderConnectionInternal>();
  plStringBuilder tmp;

  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    const plUuid& guid = pNode->GetGuid();

    if ((pNode->GetNodeName() != "Connection"))
      continue;

    RenderPipelineResourceLoaderConnectionInternal data;
    rttiConverter.ApplyPropertiesToObject(pNode, pType, &data);

    auto objectSource = context.GetObjectByGUID(data.m_Source);
    if (objectSource.m_pObject == nullptr || !objectSource.m_pType->IsDerivedFrom<plRenderPipelinePass>())
    {
      plLog::Error("Failed to retrieve connection target '{0}' with pin '{1}'", plConversionUtils::ToString(guid, tmp), data.m_TargetPin);
      continue;
    }

    auto objectTarget = context.GetObjectByGUID(data.m_Target);
    if (objectTarget.m_pObject == nullptr || !objectTarget.m_pType->IsDerivedFrom<plRenderPipelinePass>())
    {
      plLog::Error("Failed to retrieve connection target '{0}' with pin '{1}'", plConversionUtils::ToString(guid, tmp), data.m_TargetPin);
      continue;
    }

    plRenderPipelinePass* pSource = static_cast<plRenderPipelinePass*>(objectSource.m_pObject);
    plRenderPipelinePass* pTarget = static_cast<plRenderPipelinePass*>(objectTarget.m_pObject);

    if (!pPipeline->Connect(pSource, data.m_SourcePin, pTarget, data.m_TargetPin))
    {
      plLog::Error("Failed to connect '{0}'::'{1}' to '{2}'::'{3}'!", pSource->GetName(), data.m_SourcePin, pTarget->GetName(), data.m_TargetPin);
    }
  }

  return pPipeline;
}

// static
void plRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(const plRenderPipeline* pPipeline, plRenderPipelineResourceDescriptor& desc)
{
  plRenderPipelineRttiConverterContext context;

  plAbstractObjectGraph graph;

  plRttiConverterWriter rttiConverter(&graph, &context, false, true);

  plHybridArray<const plRenderPipelinePass*, 16> passes;
  pPipeline->GetPasses(passes);

  // Need to serialize all passes first so we have guids for each to be referenced in the connections.
  for (auto pPass : passes)
  {
    plUuid guid;
    guid.CreateNewUuid();
    context.RegisterObject(guid, pPass->GetDynamicRTTI(), const_cast<plRenderPipelinePass*>(pPass));
    rttiConverter.AddObjectToGraph(const_cast<plRenderPipelinePass*>(pPass));
  }
  plHybridArray<const plExtractor*, 16> extractors;
  pPipeline->GetExtractors(extractors);
  for (auto pExtractor : extractors)
  {
    plUuid guid;
    guid.CreateNewUuid();
    context.RegisterObject(guid, pExtractor->GetDynamicRTTI(), const_cast<plExtractor*>(pExtractor));
    rttiConverter.AddObjectToGraph(const_cast<plExtractor*>(pExtractor));
  }

  auto pType = plGetStaticRTTI<RenderPipelineResourceLoaderConnectionInternal>();
  auto& nodes = graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    auto objectSoure = context.GetObjectByGUID(pNode->GetGuid());

    if (objectSoure.m_pObject == nullptr || !objectSoure.m_pType->IsDerivedFrom<plRenderPipelinePass>())
    {
      continue;
    }
    plRenderPipelinePass* pSource = static_cast<plRenderPipelinePass*>(objectSoure.m_pObject);

    RenderPipelineResourceLoaderConnectionInternal data;
    data.m_Source = pNode->GetGuid();

    auto outputs = pSource->GetOutputPins();
    for (const plRenderPipelineNodePin* pPinSource : outputs)
    {
      data.m_SourcePin = pSource->GetPinName(pPinSource).GetView();

      const plRenderPipelinePassConnection* pConnection = pPipeline->GetOutputConnection(pSource, pSource->GetPinName(pPinSource));
      if (!pConnection)
        continue;

      for (const plRenderPipelineNodePin* pPinTarget : pConnection->m_Inputs)
      {
        data.m_Target = context.GetObjectGUID(pPinTarget->m_pParent->GetDynamicRTTI(), pPinTarget->m_pParent);
        data.m_TargetPin = pPinTarget->m_pParent->GetPinName(pPinTarget).GetView();

        plUuid connectionGuid;
        connectionGuid.CreateNewUuid();
        context.RegisterObject(connectionGuid, pType, &data);
        rttiConverter.AddObjectToGraph(pType, &data, "Connection");
      }
    }
  }

  plMemoryStreamContainerWrapperStorage<plDynamicArray<plUInt8>> storage(&desc.m_SerializedPipeline);

  plMemoryStreamWriter memoryWriter(&storage);
  plAbstractGraphBinarySerializer::Write(memoryWriter, &graph);
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelineResourceLoader);
