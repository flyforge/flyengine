#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/SerializationContext.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>

////////////////////////////////////////////////////////////////////////
// plDocumentNodeManager Internal
////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plRenderPipelineResourceLoaderConnection, plNoBase, 1, plRTTIDefaultAllocator<plRenderPipelineResourceLoaderConnection>)
{
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

plResult plRenderPipelineResourceLoaderConnection::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream << m_uiSource;
  inout_stream << m_uiTarget;
  inout_stream << m_sSourcePin;
  inout_stream << m_sTargetPin;

  return PL_SUCCESS;
}

plResult plRenderPipelineResourceLoaderConnection::Deserialize(plStreamReader& inout_stream)
{
  PL_VERIFY(plTypeVersionReadContext::GetContext()->GetTypeVersion(plGetStaticRTTI<plRenderPipelineResourceLoaderConnection>()) == 1, "Unknown version");

  inout_stream >> m_uiSource;
  inout_stream >> m_uiTarget;
  inout_stream >> m_sSourcePin;
  inout_stream >> m_sTargetPin;

  return PL_SUCCESS;
}

constexpr plTypeVersion s_RenderPipelineDescriptorVersion = 1;

// static
plInternal::NewInstance<plRenderPipeline> plRenderPipelineResourceLoader::CreateRenderPipeline(const plRenderPipelineResourceDescriptor& desc)
{
  auto pPipeline = PL_DEFAULT_NEW(plRenderPipeline);

  plRawMemoryStreamReader inout_stream(desc.m_SerializedPipeline);

  const auto uiVersion = inout_stream.ReadVersion(s_RenderPipelineDescriptorVersion);
  PL_IGNORE_UNUSED(uiVersion);

  plStringDeduplicationReadContext stringDeduplicationReadContext(inout_stream);
  plTypeVersionReadContext typeVersionReadContext(inout_stream);

  plStringBuilder sTypeName;

  plHybridArray<plRenderPipelinePass*, 16> passes;

  // Passes
  {
    plUInt32 uiNumPasses = 0;
    inout_stream >> uiNumPasses;

    for (plUInt32 i = 0; i < uiNumPasses; ++i)
    {
      inout_stream >> sTypeName;
      if (const plRTTI* pType = plRTTI::FindTypeByName(sTypeName))
      {
        plUniquePtr<plRenderPipelinePass> pPass = pType->GetAllocator()->Allocate<plRenderPipelinePass>();
        pPass->Deserialize(inout_stream).AssertSuccess("");
        passes.PushBack(pPass.Borrow());
        pPipeline->AddPass(std::move(pPass));
      }
      else
      {
        plLog::Error("Unknown render pipeline pass type '{}'", sTypeName);
        return nullptr;
      }
    }
  }

  // Extractors
  {
    plUInt32 uiNumExtractors = 0;
    inout_stream >> uiNumExtractors;

    for (plUInt32 i = 0; i < uiNumExtractors; ++i)
    {
      inout_stream >> sTypeName;
      if (const plRTTI* pType = plRTTI::FindTypeByName(sTypeName))
      {
        plUniquePtr<plExtractor> pExtractor = pType->GetAllocator()->Allocate<plExtractor>();
        pExtractor->Deserialize(inout_stream).AssertSuccess("");
        pPipeline->AddExtractor(std::move(pExtractor));
      }
      else
      {
        plLog::Error("Unknown render pipeline extractor type '{}'", sTypeName);
        return nullptr;
      }
    }
  }

  // Connections
  {
    plUInt32 uiNumConnections = 0;
    inout_stream >> uiNumConnections;

    for (plUInt32 i = 0; i < uiNumConnections; ++i)
    {
      plRenderPipelineResourceLoaderConnection data;
      data.Deserialize(inout_stream).AssertSuccess("Failed to deserialize render pipeline connection");

      plRenderPipelinePass* pSource = passes[data.m_uiSource];
      plRenderPipelinePass* pTarget = passes[data.m_uiTarget];

      if (!pPipeline->Connect(pSource, data.m_sSourcePin, pTarget, data.m_sTargetPin))
      {
        plLog::Error("Failed to connect '{0}'::'{1}' to '{2}'::'{3}'!", pSource->GetName(), data.m_sSourcePin, pTarget->GetName(), data.m_sTargetPin);
      }
    }
  }
  return pPipeline;
}

// static
void plRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(const plRenderPipeline* pPipeline, plRenderPipelineResourceDescriptor& ref_desc)
{
  plHybridArray<const plRenderPipelinePass*, 16> passes;
  plHybridArray<const plExtractor*, 16> extractors;
  plHybridArray<plRenderPipelineResourceLoaderConnection, 16> connections;

  plHashTable<const plRenderPipelineNode*, plUInt32> passToIndex;
  pPipeline->GetPasses(passes);
  pPipeline->GetExtractors(extractors);

  passToIndex.Reserve(passes.GetCount());
  for (plUInt32 i = 0; i < passes.GetCount(); i++)
  {
    passToIndex.Insert(passes[i], i);
  }


  for (plUInt32 i = 0; i < passes.GetCount(); i++)
  {
    const plRenderPipelinePass* pSource = passes[i];

    plRenderPipelineResourceLoaderConnection data;
    data.m_uiSource = i;

    auto outputs = pSource->GetOutputPins();
    for (const plRenderPipelineNodePin* pPinSource : outputs)
    {
      data.m_sSourcePin = pSource->GetPinName(pPinSource).GetView();

      const plRenderPipelinePassConnection* pConnection = pPipeline->GetOutputConnection(pSource, pSource->GetPinName(pPinSource));
      if (!pConnection)
        continue;

      for (const plRenderPipelineNodePin* pPinTarget : pConnection->m_Inputs)
      {
        PL_VERIFY(passToIndex.TryGetValue(pPinTarget->m_pParent, data.m_uiTarget), "Failed to resolve render pass to index");
        data.m_sTargetPin = pPinTarget->m_pParent->GetPinName(pPinTarget).GetView();

        connections.PushBack(data);
      }
    }
  }

  plMemoryStreamContainerWrapperStorage<plDynamicArray<plUInt8>> storage(&ref_desc.m_SerializedPipeline);
  plMemoryStreamWriter memoryWriter(&storage);
  ExportPipeline(passes.GetArrayPtr(), extractors.GetArrayPtr(), connections.GetArrayPtr(), memoryWriter).AssertSuccess("Failed to serialize pipeline");
}

plResult plRenderPipelineResourceLoader::ExportPipeline(plArrayPtr<const plRenderPipelinePass* const> passes, plArrayPtr<const plExtractor* const> extractors, plArrayPtr<const plRenderPipelineResourceLoaderConnection> connections, plStreamWriter& ref_streamWriter)
{
  ref_streamWriter.WriteVersion(s_RenderPipelineDescriptorVersion);

  plStringDeduplicationWriteContext stringDeduplicationWriteContext(ref_streamWriter);
  plTypeVersionWriteContext typeVersionWriteContext;
  auto& stream = typeVersionWriteContext.Begin(stringDeduplicationWriteContext.Begin());

  // passes
  {
    const plUInt32 uiNumPasses = passes.GetCount();
    stream << uiNumPasses;

    for (auto& pass : passes)
    {
      auto pPassType = pass->GetDynamicRTTI();
      typeVersionWriteContext.AddType(pPassType);

      stream << pPassType->GetTypeName();
      PL_SUCCEED_OR_RETURN(pass->Serialize(stream));
    }
  }

  // extractors
  {
    const plUInt32 uiNumExtractors = extractors.GetCount();
    stream << uiNumExtractors;

    for (auto& extractor : extractors)
    {
      auto pExtractorType = extractor->GetDynamicRTTI();
      typeVersionWriteContext.AddType(pExtractorType);

      stream << pExtractorType->GetTypeName();
      PL_SUCCEED_OR_RETURN(extractor->Serialize(stream));
    }
  }

  // Connections
  {
    const plUInt32 uiNumConnections = connections.GetCount();
    stream << uiNumConnections;

    typeVersionWriteContext.AddType(plGetStaticRTTI<plRenderPipelineResourceLoaderConnection>());

    for (auto& connection : connections)
    {
      PL_SUCCEED_OR_RETURN(connection.Serialize(stream));
    }
  }

  PL_SUCCEED_OR_RETURN(typeVersionWriteContext.End());
  PL_SUCCEED_OR_RETURN(stringDeduplicationWriteContext.End());

  return PL_SUCCESS;
}

PL_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelineResourceLoader);
