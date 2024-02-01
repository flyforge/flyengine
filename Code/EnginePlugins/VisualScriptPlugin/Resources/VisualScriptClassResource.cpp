#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <VisualScriptPlugin/Resources/VisualScriptClassResource.h>
#include <VisualScriptPlugin/Runtime/VisualScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScriptFunctionProperty.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plVisualScriptClassResource, 1, plRTTIDefaultAllocator<plVisualScriptClassResource>)
PL_END_DYNAMIC_REFLECTED_TYPE;
PL_RESOURCE_IMPLEMENT_COMMON_CODE(plVisualScriptClassResource);

PL_BEGIN_SUBSYSTEM_DECLARATION(TypeScript, Resource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ResourceManager" 
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP 
  {
    plResourceManager::RegisterResourceForAssetType("VisualScriptClass", plGetStaticRTTI<plVisualScriptClassResource>());
    plResourceManager::RegisterResourceOverrideType(plGetStaticRTTI<plVisualScriptClassResource>(), [](const plStringBuilder& sResourceID) -> bool  {
        return sResourceID.HasExtension(".plVisualScriptClassBin");
      });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plResourceManager::UnregisterResourceOverrideType(plGetStaticRTTI<plVisualScriptClassResource>());
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

plVisualScriptClassResource::plVisualScriptClassResource() = default;
plVisualScriptClassResource::~plVisualScriptClassResource() = default;

plResourceLoadDesc plVisualScriptClassResource::UnloadData(Unload WhatToUnload)
{
  DeleteScriptType();
  DeleteAllScriptCoroutineTypes();

  plResourceLoadDesc ld;
  ld.m_State = plResourceState::Unloaded;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable = 0;

  return ld;
}

plResourceLoadDesc plVisualScriptClassResource::UpdateContent(plStreamReader* pStream)
{
  plResourceLoadDesc ld;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable = 0;
  ld.m_State = plResourceState::LoadedResourceMissing;

  if (pStream == nullptr)
  {
    return ld;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    plString sAbsFilePath;
    (*pStream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  plAssetFileHeader AssetHash;
  AssetHash.Read(*pStream).IgnoreResult();

  plString sScriptClassName;
  const plRTTI* pBaseClassType = nullptr;
  plScriptRTTI::FunctionList functions;
  plScriptRTTI::MessageHandlerList messageHandlers;
  {
    plStringDeduplicationReadContext stringDedup(*pStream);

    plChunkStreamReader chunk(*pStream);
    chunk.SetEndChunkFileMode(plChunkStreamReader::EndChunkFileMode::JustClose);

    chunk.BeginStream();

    // skip all chunks that we don't know
    while (chunk.GetCurrentChunk().m_bValid)
    {
      if (chunk.GetCurrentChunk().m_sChunkName == "Header")
      {
        plString sBaseClassName;
        chunk >> sBaseClassName;
        chunk >> sScriptClassName;
        pBaseClassType = plRTTI::FindTypeByName(sBaseClassName);
        if (pBaseClassType == nullptr)
        {
          plLog::Error("Invalid base class '{}' for Visual Script Class '{}'", sBaseClassName, sScriptClassName);
          return ld;
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "FunctionGraphs")
      {
        plUInt32 uiNumFunctions;
        chunk >> uiNumFunctions;

        for (plUInt32 i = 0; i < uiNumFunctions; ++i)
        {
          plString sFunctionName;
          plEnum<plVisualScriptNodeDescription::Type> functionType;
          plEnum<plScriptCoroutineCreationMode> coroutineCreationMode;
          chunk >> sFunctionName;
          chunk >> functionType;
          chunk >> coroutineCreationMode;

          plUniquePtr<plVisualScriptGraphDescription> pDesc = PL_SCRIPT_NEW(plVisualScriptGraphDescription);
          if (pDesc->Deserialize(chunk).Failed())
          {
            plLog::Error("Invalid visual script desc");
            return ld;
          }

          if (functionType == plVisualScriptNodeDescription::Type::EntryCall)
          {
            plUniquePtr<plVisualScriptFunctionProperty> pFunctionProperty = PL_SCRIPT_NEW(plVisualScriptFunctionProperty, sFunctionName, std::move(pDesc));
            functions.PushBack(std::move(pFunctionProperty));
          }
          else if (functionType == plVisualScriptNodeDescription::Type::EntryCall_Coroutine)
          {
            plUniquePtr<plVisualScriptCoroutineAllocator> pCoroutineAllocator = PL_SCRIPT_NEW(plVisualScriptCoroutineAllocator, std::move(pDesc));
            auto pCoroutineType = CreateScriptCoroutineType(sScriptClassName, sFunctionName, std::move(pCoroutineAllocator));
            plUniquePtr<plScriptCoroutineFunctionProperty> pFunctionProperty = PL_SCRIPT_NEW(plScriptCoroutineFunctionProperty, sFunctionName, pCoroutineType, coroutineCreationMode);
            functions.PushBack(std::move(pFunctionProperty));
          }
          else if (functionType == plVisualScriptNodeDescription::Type::MessageHandler)
          {
            auto desc = pDesc->GetMessageDesc();
            plUniquePtr<plVisualScriptMessageHandler> pMessageHandler = PL_SCRIPT_NEW(plVisualScriptMessageHandler, desc, std::move(pDesc));
            messageHandlers.PushBack(std::move(pMessageHandler));
          }
          else if (functionType == plVisualScriptNodeDescription::Type::MessageHandler_Coroutine)
          {
            auto desc = pDesc->GetMessageDesc();
            plUniquePtr<plVisualScriptCoroutineAllocator> pCoroutineAllocator = PL_SCRIPT_NEW(plVisualScriptCoroutineAllocator, std::move(pDesc));
            auto pCoroutineType = CreateScriptCoroutineType(sScriptClassName, sFunctionName, std::move(pCoroutineAllocator));
            plUniquePtr<plScriptCoroutineMessageHandler> pMessageHandler = PL_SCRIPT_NEW(plScriptCoroutineMessageHandler, sFunctionName, desc, pCoroutineType, coroutineCreationMode);
            messageHandlers.PushBack(std::move(pMessageHandler));
          }
          else
          {
            plLog::Error("Invalid event handler type '{}' for event handler '{}'", plVisualScriptNodeDescription::Type::GetName(functionType), sFunctionName);
            return ld;
          }
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "ConstantData")
      {
        plSharedPtr<plVisualScriptDataDescription> pConstantDataDesc = PL_SCRIPT_NEW(plVisualScriptDataDescription);
        if (pConstantDataDesc->Deserialize(chunk).Failed())
        {
          return ld;
        }

        plSharedPtr<plVisualScriptDataStorage> pConstantDataStorage = PL_SCRIPT_NEW(plVisualScriptDataStorage, pConstantDataDesc);
        if (pConstantDataStorage->Deserialize(chunk).Succeeded())
        {
          m_pConstantDataStorage = pConstantDataStorage;
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "InstanceData")
      {
        plSharedPtr<plVisualScriptDataDescription> pInstanceDataDesc = PL_SCRIPT_NEW(plVisualScriptDataDescription);
        if (pInstanceDataDesc->Deserialize(chunk).Succeeded())
        {
          m_pInstanceDataDesc = pInstanceDataDesc;
        }

        plSharedPtr<plVisualScriptInstanceDataMapping> pInstanceDataMapping = PL_SCRIPT_NEW(plVisualScriptInstanceDataMapping);
        if (chunk.ReadHashTable(pInstanceDataMapping->m_Content).Succeeded())
        {
          m_pInstanceDataMapping = pInstanceDataMapping;
        }
      }

      chunk.NextChunk();
    }
  }

  CreateScriptType(sScriptClassName, pBaseClassType, std::move(functions), std::move(messageHandlers));

  ld.m_State = plResourceState::Loaded;
  return ld;
}

void plVisualScriptClassResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = (plUInt32)sizeof(plVisualScriptClassResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

plUniquePtr<plScriptInstance> plVisualScriptClassResource::Instantiate(plReflectedClass& inout_owner, plWorld* pWorld) const
{
  return PL_SCRIPT_NEW(plVisualScriptInstance, inout_owner, pWorld, m_pConstantDataStorage, m_pInstanceDataDesc, m_pInstanceDataMapping);
}
