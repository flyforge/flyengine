#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Scripting/DuktapeContext.h>
#include <Foundation/Configuration/Startup.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>
#include <TypeScriptPlugin/Resources/TypeScriptResource.h>

namespace
{
  class TypeScriptFunctionProperty : public plAbstractFunctionProperty
  {
  public:
    TypeScriptFunctionProperty(const char* szPropertyName)
      : plAbstractFunctionProperty(szPropertyName)
    {
    }

    virtual plFunctionType::Enum GetFunctionType() const override { return plFunctionType::Member; }
    virtual const plRTTI* GetReturnType() const override { return nullptr; }
    virtual plBitflags<plPropertyFlags> GetReturnFlags() const override { return plPropertyFlags::Void; }
    virtual plUInt32 GetArgumentCount() const override { return 0; }
    virtual const plRTTI* GetArgumentType(plUInt32 uiParamIndex) const override { return nullptr; }
    virtual plBitflags<plPropertyFlags> GetArgumentFlags(plUInt32 uiParamIndex) const override { return plPropertyFlags::Void; }

    virtual void Execute(void* pInstance, plArrayPtr<plVariant> arguments, plVariant& ref_returnValue) const override
    {
      auto pTypeScriptInstance = static_cast<plTypeScriptInstance*>(pInstance);
      plTypeScriptBinding& binding = pTypeScriptInstance->GetBinding();

      plDuktapeHelper duk(binding.GetDukTapeContext());

      // TODO: this needs to be more generic to work with other things besides components
      binding.DukPutComponentObject(&pTypeScriptInstance->GetComponent()); // [ comp ]

      if (duk.PrepareMethodCall(GetPropertyName()).Succeeded()) // [ comp func comp ]
      {
        duk.CallPreparedMethod().IgnoreResult(); // [ comp result ]
        duk.PopStack(2);                         // [ ]

        PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
      }
      else
      {
        // remove 'this'   [ comp ]
        duk.PopStack(); // [ ]

        PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
      }
    }
  };
} // namespace

//////////////////////////////////////////////////////////////////////////

plTypeScriptInstance::plTypeScriptInstance(plComponent& inout_owner, plWorld* pWorld, plTypeScriptBinding& inout_binding)
  : plScriptInstance(inout_owner, pWorld)
  , m_Binding(inout_binding)
{
}

void plTypeScriptInstance::ApplyParameters(const plArrayMap<plHashedString, plVariant>& parameters)
{
  plDuktapeHelper duk(m_Binding.GetDukTapeContext());

  m_Binding.DukPutComponentObject(&GetComponent()); // [ comp ]

  for (plUInt32 p = 0; p < parameters.GetCount(); ++p)
  {
    const auto& pair = parameters.GetPair(p);

    plTypeScriptBinding::SetVariantProperty(duk, pair.key.GetString(), -1, pair.value); // [ comp ]
  }

  duk.PopStack(); // [ ]

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptClassResource, 1, plRTTIDefaultAllocator<plTypeScriptClassResource>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
PLASMA_RESOURCE_IMPLEMENT_COMMON_CODE(plTypeScriptClassResource);

PLASMA_BEGIN_SUBSYSTEM_DECLARATION(TypeScript, ClassResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ResourceManager" 
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP 
  {
    plResourceManager::RegisterResourceOverrideType(plGetStaticRTTI<plTypeScriptClassResource>(), [](const plStringBuilder& sResourceID) -> bool  {
        return sResourceID.HasExtension(".plTypeScriptRes");
      });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plResourceManager::UnregisterResourceOverrideType(plGetStaticRTTI<plTypeScriptClassResource>());
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

plTypeScriptClassResource::plTypeScriptClassResource() = default;
plTypeScriptClassResource::~plTypeScriptClassResource() = default;

plResourceLoadDesc plTypeScriptClassResource::UnloadData(Unload WhatToUnload)
{
  DeleteScriptType();

  plResourceLoadDesc ld;
  ld.m_State = plResourceState::Unloaded;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable = 0;

  return ld;
}

plResourceLoadDesc plTypeScriptClassResource::UpdateContent(plStreamReader* pStream)
{
  plResourceLoadDesc ld;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable = 0;

  if (pStream == nullptr)
  {
    ld.m_State = plResourceState::LoadedResourceMissing;
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

  plString sTypeName;
  (*pStream) >> sTypeName;
  (*pStream) >> m_Guid;

  plScriptRTTI::FunctionList functions;
  plScriptRTTI::MessageHandlerList messageHandlers;

  // TODO: this list should be generated during asset transform and stored in the resource
  const char* szFunctionNames[] = {"Initialize", "Deinitialize", "OnActivated", "OnDeactivated", "OnSimulationStarted", "Tick"};

  for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(szFunctionNames); ++i)
  {
    functions.PushBack(PLASMA_DEFAULT_NEW(TypeScriptFunctionProperty, szFunctionNames[i]));
  }

  const plRTTI* pParentType = plGetStaticRTTI<plComponent>();
  CreateScriptType(sTypeName, pParentType, std::move(functions), std::move(messageHandlers));

  ld.m_State = plResourceState::Loaded;

  return ld;
}

void plTypeScriptClassResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = (plUInt32)sizeof(plTypeScriptClassResource);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

plUniquePtr<plScriptInstance> plTypeScriptClassResource::Instantiate(plReflectedClass& owner, plWorld* pWorld) const
{
  auto pComponent = plStaticCast<plComponent*>(&owner);

  // TODO: typescript context needs to be moved to a world module
  auto pTypeScriptComponentManager = static_cast<plTypeScriptComponentManager*>(pWorld->GetManagerForComponentType(plGetStaticRTTI<plTypeScriptComponent>()));
  auto& binding = pTypeScriptComponentManager->GetTsBinding();

  plTypeScriptBinding::TsComponentTypeInfo componentTypeInfo;
  if (binding.LoadComponent(m_Guid, componentTypeInfo).Failed())
  {
    plLog::Error("Failed to load TS component type.");
    return nullptr;
  }

  plUInt32 uiStashIdx = 0;
  if (binding.RegisterComponent(m_pType->GetTypeName(), pComponent->GetHandle(), uiStashIdx, false).Failed())
  {
    plLog::Error("Failed to register TS component type '{}'. Class may not exist under that name.", m_pType->GetTypeName());
    return nullptr;
  }

  return PLASMA_DEFAULT_NEW(plTypeScriptInstance, *pComponent, pWorld, binding);
}
