#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/RenderPipelineAsset/RenderPipelineContext.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plRenderPipelineContextLoaderConnection, plNoBase, 1, plRTTIDefaultAllocator<plRenderPipelineContextLoaderConnection>)
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


const plRTTI* plRenderPipelineRttiConverterContext::FindTypeByName(plStringView sName) const
{
  if (sName == "DocumentNodeManager_DefaultConnection")
  {
    return plGetStaticRTTI<plRenderPipelineContextLoaderConnection>();
  }
  return plWorldRttiConverterContext::FindTypeByName(sName);
}

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRenderPipelineContext, 1, plRTTIDefaultAllocator<plRenderPipelineContext>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_CONSTANT_PROPERTY("DocumentType", (const char*) "RenderPipeline"),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plRenderPipelineContext::plRenderPipelineContext()
  : plEngineProcessDocumentContext(plEngineProcessDocumentContextFlags::CreateWorld)
{
}

void plRenderPipelineContext::HandleMessage(const plEditorEngineDocumentMsg* pMsg)
{
  plEngineProcessDocumentContext::HandleMessage(pMsg);
}

void plRenderPipelineContext::OnInitialize()
{
}

plEngineProcessViewContext* plRenderPipelineContext::CreateViewContext()
{
  PLASMA_ASSERT_DEV(false, "Should not be called");
  return nullptr;
}

void plRenderPipelineContext::DestroyViewContext(plEngineProcessViewContext* pContext)
{
  PLASMA_ASSERT_DEV(false, "Should not be called");
}

plWorldRttiConverterContext& plRenderPipelineContext::GetContext()
{
  return m_RenderPipelineContext;
}

const plWorldRttiConverterContext& plRenderPipelineContext::GetContext() const
{
  return m_RenderPipelineContext;
}

plStatus plRenderPipelineContext::ExportDocument(const plExportDocumentMsgToEngine* pMsg)
{
  plDynamicArray<plRenderPipelinePass*> passes;
  plDynamicArray<plExtractor*> extractors;
  plDynamicArray<plRenderPipelineResourceLoaderConnection> connections;

  plDynamicArray<plUuid> passUuids;
  plDynamicArray<plRenderPipelineContextLoaderConnection*> toolConnections;

  m_RenderPipelineContext.GetObjectsByType(passes, &passUuids);
  m_RenderPipelineContext.GetObjectsByType(extractors);
  m_RenderPipelineContext.GetObjectsByType(toolConnections);

  plHashTable<plUuid, plUInt32> passUuidToIndex;
  for (plUInt32 i = 0; i < passUuids.GetCount(); ++i)
  {
    passUuidToIndex.Insert(passUuids[i], i);
  }
  connections.SetCount(toolConnections.GetCount());
  for (plUInt32 i = 0; i < toolConnections.GetCount(); i++)
  {
    plRenderPipelineContextLoaderConnection* pConnection = toolConnections[i];
    plRenderPipelineResourceLoaderConnection& engineConnection = connections[i];
    PLASMA_VERIFY(passUuidToIndex.TryGetValue(pConnection->m_Source, engineConnection.m_uiSource), "");
    PLASMA_VERIFY(passUuidToIndex.TryGetValue(pConnection->m_Target, engineConnection.m_uiTarget), "");
    engineConnection.m_sSourcePin = pConnection->m_SourcePin;
    engineConnection.m_sTargetPin = pConnection->m_TargetPin;
  }

  plDefaultMemoryStreamStorage storage;
  {
    // Export Resource Data
    plMemoryStreamWriter writer(&storage);
    PLASMA_SUCCEED_OR_RETURN(plRenderPipelineResourceLoader::ExportPipeline(passes.GetArrayPtr(), extractors.GetArrayPtr(), connections.GetArrayPtr(), writer));
  }

  plDeferredFileWriter file;
  file.SetOutput(pMsg->m_sOutputFile);

  {
    // File Header
    plAssetFileHeader header;
    header.SetFileHashAndVersion(pMsg->m_uiAssetHash, pMsg->m_uiVersion);
    header.Write(file).IgnoreResult();

    plUInt8 uiVersion = 2;
    file << uiVersion;
  }

  {
    // Resource Data
    plUInt32 uiSize = storage.GetStorageSize32();
    file << uiSize;
    if (storage.CopyToStream(file).Failed())
      return plStatus(plFmt("Failed to copy content to file writer for '{}'", pMsg->m_sOutputFile));
  }

  // do the actual file writing
  if (file.Close().Failed())
    return plStatus(plFmt("Writing to '{}' failed.", pMsg->m_sOutputFile));

  return plStatus(PLASMA_SUCCESS);
}
