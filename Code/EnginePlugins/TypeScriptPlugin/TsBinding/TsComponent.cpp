#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/Messages/EventMessage.h>
#include <Core/World/Component.h>
#include <Duktape/duktape.h>
#include <Foundation/Types/ScopeExit.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Component_IsValid(duk_context* pDuk);
static int __CPP_Component_GetUniqueID(duk_context* pDuk);
static int __CPP_Component_GetOwner(duk_context* pDuk);
static int __CPP_Component_SetActiveFlag(duk_context* pDuk);
static int __CPP_Component_IsActive(duk_context* pDuk);
static int __CPP_Component_SendMessage(duk_context* pDuk);
static int __CPP_TsComponent_BroadcastEvent(duk_context* pDuk);
static int __CPP_TsComponent_SetTickInterval(duk_context* pDuk);

plResult plTypeScriptBinding::Init_Component()
{
  m_Duk.RegisterGlobalFunction("__CPP_Component_IsValid", __CPP_Component_IsValid, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Component_GetUniqueID", __CPP_Component_GetUniqueID, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Component_GetOwner", __CPP_Component_GetOwner, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Component_SetActiveFlag", __CPP_Component_SetActiveFlag, 2);
  m_Duk.RegisterGlobalFunction("__CPP_Component_GetActiveFlag", __CPP_Component_IsActive, 1, -1);
  m_Duk.RegisterGlobalFunction("__CPP_Component_IsActive", __CPP_Component_IsActive, 1, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Component_IsActiveAndInitialized", __CPP_Component_IsActive, 1, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Component_IsActiveAndSimulating", __CPP_Component_IsActive, 1, 2);
  m_Duk.RegisterGlobalFunction("__CPP_Component_SendMessage", __CPP_Component_SendMessage, 4, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Component_PostMessage", __CPP_Component_SendMessage, 4, 1);
  m_Duk.RegisterGlobalFunction("__CPP_TsComponent_BroadcastEvent", __CPP_TsComponent_BroadcastEvent, 4);
  m_Duk.RegisterGlobalFunction("__CPP_TsComponent_SetTickInterval", __CPP_TsComponent_SetTickInterval, 2);

  return PLASMA_SUCCESS;
}

plResult plTypeScriptBinding::RegisterComponent(plStringView sTypeName0, plComponentHandle hHandle, plUInt32& out_uiStashIdx, bool bIsNativeComponent)
{
  if (hHandle.IsInvalidated())
    return PLASMA_FAILURE;

  plUInt32& uiStashIdx = m_ComponentToStashIdx[hHandle];

  if (uiStashIdx != 0)
  {
    out_uiStashIdx = uiStashIdx;
    return PLASMA_SUCCESS;
  }

  uiStashIdx = AcquireStashObjIndex();

  plDuktapeHelper duk(m_Duk);
  PLASMA_DUK_VERIFY_STACK(duk, 0);

  duk.PushGlobalObject(); // [ global ]
  PLASMA_DUK_VERIFY_STACK(duk, +1);

  plStringBuilder sTypeName = sTypeName0;

  if (bIsNativeComponent)
  {
    sTypeName.TrimWordStart("pl");

    PLASMA_SUCCEED_OR_RETURN(duk.PushLocalObject("__AllComponents")); // [ global __AllComponents ]
    PLASMA_DUK_VERIFY_STACK(duk, +2);
  }
  else
  {
    const plStringBuilder sCompModule("__", sTypeName);

    PLASMA_SUCCEED_OR_RETURN(duk.PushLocalObject(sCompModule)); // [ global __CompModule ]
    PLASMA_DUK_VERIFY_STACK(duk, +2);
  }

  if (!duk_get_prop_string(duk, -1, sTypeName)) // [ global __CompModule sTypeName ]
  {
    duk.PopStack(3); // [ ]
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, PLASMA_FAILURE, 0);
  }

  duk_new(duk, 0); // [ global __CompModule object ]
  PLASMA_DUK_VERIFY_STACK(duk, +3);

  // store C++ side component handle in obj as property
  {
    plComponentHandle* pBuffer =
      reinterpret_cast<plComponentHandle*>(duk_push_fixed_buffer(duk, sizeof(plComponentHandle))); // [ global __CompModule object buffer ]
    *pBuffer = hHandle;
    duk_put_prop_index(duk, -2, plTypeScriptBindingIndexProperty::ComponentHandle); // [ global __CompModule object ]
  }

  PLASMA_DUK_VERIFY_STACK(duk, +3);

  StoreReferenceInStash(duk, uiStashIdx); // [ global __CompModule object ]
  PLASMA_DUK_VERIFY_STACK(duk, +3);

  duk.PopStack(3); // [ ]
  PLASMA_DUK_VERIFY_STACK(duk, 0);

  out_uiStashIdx = uiStashIdx;

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, PLASMA_SUCCESS, 0);
}

void plTypeScriptBinding::DukPutComponentObject(plComponent* pComponent)
{
  if (pComponent == nullptr)
  {
    m_Duk.PushNull(); // [ null ]
  }
  else
  {
    plUInt32 uiStashIdx = 0;
    if (RegisterComponent(pComponent->GetDynamicRTTI()->GetTypeName(), pComponent->GetHandle(), uiStashIdx, true).Failed())
    {
      m_Duk.PushNull(); // [ null ]
      return;
    }

    DukPushStashObject(m_Duk, uiStashIdx);
  }
}

plComponentHandle plTypeScriptBinding::RetrieveComponentHandle(duk_context* pDuk, plInt32 iObjIdx /*= 0 */)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return plComponentHandle();

  plDuktapeHelper duk(pDuk);

  if (duk_get_prop_index(pDuk, iObjIdx, plTypeScriptBindingIndexProperty::ComponentHandle))
  {
    plComponentHandle hComponent = *reinterpret_cast<plComponentHandle*>(duk_get_buffer(pDuk, -1, nullptr));
    duk_pop(pDuk);
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, hComponent, 0);
  }

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, plComponentHandle(), 0);
}

static int __CPP_Component_IsValid(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  if (!duk_get_prop_index(pDuk, 0, plTypeScriptBindingIndexProperty::ComponentHandle))
  {
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(false), 1);
  }

  plComponentHandle hComponent = *reinterpret_cast<plComponentHandle*>(duk_get_buffer(pDuk, -1, nullptr));
  duk_pop(pDuk);

  plComponent* pComponent = nullptr;
  plWorld* pWorld = plTypeScriptBinding::RetrieveWorld(pDuk);

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(pWorld->TryGetComponent(hComponent, pComponent)), 1);
}

static int __CPP_Component_GetUniqueID(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plComponent* pComponent = plTypeScriptBinding::ExpectComponent<plComponent>(pDuk);

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnUInt(pComponent->GetUniqueID()), +1);
}

static int __CPP_Component_GetOwner(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plComponent* pComponent = plTypeScriptBinding::ExpectComponent<plComponent>(pDuk);

  plTypeScriptBinding* pBinding = plTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(pComponent->GetOwner()->GetHandle());

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_Component_SetActiveFlag(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plComponent* pComponent = plTypeScriptBinding::ExpectComponent<plComponent>(pDuk);

  pComponent->SetActiveFlag(duk.GetBoolValue(1, true));

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

static int __CPP_Component_IsActive(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plComponent* pComponent = plTypeScriptBinding::ExpectComponent<plComponent>(pDuk);

  switch (duk.GetFunctionMagicValue())
  {
    case -1:
      PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(pComponent->GetActiveFlag()), +1);

    case 0:
      PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(pComponent->IsActive()), +1);

    case 1:
      PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(pComponent->IsActiveAndInitialized()), +1);

    case 2:
      PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(pComponent->IsActiveAndSimulating()), +1);
  }

  PLASMA_ASSERT_NOT_IMPLEMENTED;
  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(false), +1);
}

static int __CPP_Component_SendMessage(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plComponent* pComponent = plTypeScriptBinding::ExpectComponent<plComponent>(duk, 0 /*this*/);

  plTypeScriptBinding* pBinding = plTypeScriptBinding::RetrieveBinding(duk);

  if (duk.GetFunctionMagicValue() == 0) // SendMessage
  {
    plUniquePtr<plMessage> pMsg = pBinding->MessageFromParameter(pDuk, 1, plTime::MakeZero());
    pComponent->SendMessage(*pMsg);

    if (duk.GetBoolValue(3)) // expect the message to have result values
    {
      // sync msg back to TS
      plTypeScriptBinding::SyncPlasmaObjectToTsObject(pDuk, pMsg->GetDynamicRTTI(), pMsg.Borrow(), 1);
    }
  }
  else // PostMessage
  {
    const plTime delay = plTime::Seconds(duk.GetNumberValue(3));

    plUniquePtr<plMessage> pMsg = pBinding->MessageFromParameter(pDuk, 1, delay);
    pComponent->PostMessage(*pMsg, delay);
  }

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

static int __CPP_TsComponent_BroadcastEvent(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plTypeScriptComponent* pComponent = plTypeScriptBinding::ExpectComponent<plTypeScriptComponent>(duk, 0 /*this*/);

  plTypeScriptBinding* pBinding = plTypeScriptBinding::RetrieveBinding(duk);

  plUniquePtr<plMessage> pMsg = pBinding->MessageFromParameter(pDuk, 1, plTime::MakeZero());
  pComponent->BroadcastEventMsg(plStaticCast<plEventMessage&>(*pMsg));

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

static int __CPP_TsComponent_SetTickInterval(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plTypeScriptComponent* pComponent = plTypeScriptBinding::ExpectComponent<plTypeScriptComponent>(duk, 0 /*this*/);

  const plTime interval = plTime::Seconds(duk.GetFloatValue(1, 0.0f));
  pComponent->SetUpdateInterval(interval);

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}
