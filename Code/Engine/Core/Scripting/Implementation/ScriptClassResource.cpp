#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClassResource.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plScriptClassResource, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
PL_RESOURCE_IMPLEMENT_COMMON_CODE(plScriptClassResource);
// clang-format on

plScriptClassResource::plScriptClassResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

plScriptClassResource::~plScriptClassResource() = default;

plSharedPtr<plScriptRTTI> plScriptClassResource::CreateScriptType(plStringView sName, const plRTTI* pBaseType, plScriptRTTI::FunctionList&& functions, plScriptRTTI::MessageHandlerList&& messageHandlers)
{
  plScriptRTTI::FunctionList sortedFunctions;
  for (auto pFuncProp : pBaseType->GetFunctions())
  {
    auto pBaseClassFuncAttr = pFuncProp->GetAttributeByType<plScriptBaseClassFunctionAttribute>();
    if (pBaseClassFuncAttr == nullptr)
      continue;

    plStringView sBaseClassFuncName = pFuncProp->GetPropertyName();
    sBaseClassFuncName.TrimWordStart("Reflection_");

    plUInt16 uiIndex = pBaseClassFuncAttr->GetIndex();
    sortedFunctions.EnsureCount(uiIndex + 1);

    for (plUInt32 i = 0; i < functions.GetCount(); ++i)
    {
      auto& pScriptFuncProp = functions[i];
      if (pScriptFuncProp == nullptr)
        continue;

      if (sBaseClassFuncName == pScriptFuncProp->GetPropertyName())
      {
        sortedFunctions[uiIndex] = std::move(pScriptFuncProp);
        functions.RemoveAtAndSwap(i);
        break;
      }
    }
  }

  m_pType = PL_SCRIPT_NEW(plScriptRTTI, sName, pBaseType, std::move(sortedFunctions), std::move(messageHandlers));
  return m_pType;
}

void plScriptClassResource::DeleteScriptType()
{
  m_pType = nullptr;
}

plSharedPtr<plScriptCoroutineRTTI> plScriptClassResource::CreateScriptCoroutineType(plStringView sScriptClassName, plStringView sFunctionName, plUniquePtr<plRTTIAllocator>&& pAllocator)
{
  plStringBuilder sCoroutineTypeName;
  sCoroutineTypeName.Set(sScriptClassName, "::", sFunctionName, "<Coroutine>");

  plSharedPtr<plScriptCoroutineRTTI> pCoroutineType = PL_SCRIPT_NEW(plScriptCoroutineRTTI, sCoroutineTypeName, std::move(pAllocator));
  m_CoroutineTypes.PushBack(pCoroutineType);

  return pCoroutineType;
}

void plScriptClassResource::DeleteAllScriptCoroutineTypes()
{
  m_CoroutineTypes.Clear();
}


PL_STATICLINK_FILE(Core, Core_Scripting_Implementation_ScriptClassResource);

