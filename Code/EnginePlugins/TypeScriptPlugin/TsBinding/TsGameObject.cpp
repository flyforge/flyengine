#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <Duktape/duktape.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_GameObject_IsValid(duk_context* pDuk);
static int __CPP_GameObject_SetX_Vec3(duk_context* pDuk);
static int __CPP_GameObject_GetX_Vec3(duk_context* pDuk);
static int __CPP_GameObject_SetX_Float(duk_context* pDuk);
static int __CPP_GameObject_GetX_Float(duk_context* pDuk);
static int __CPP_GameObject_SetX_Quat(duk_context* pDuk);
static int __CPP_GameObject_GetX_Quat(duk_context* pDuk);
static int __CPP_GameObject_SetX_Bool(duk_context* pDuk);
static int __CPP_GameObject_GetX_Bool(duk_context* pDuk);
static int __CPP_GameObject_FindChildByName(duk_context* pDuk);
static int __CPP_GameObject_FindChildByPath(duk_context* pDuk);
static int __CPP_GameObject_TryGetComponentOfBaseTypeName(duk_context* pDuk);
static int __CPP_GameObject_TryGetComponentOfBaseTypeNameHash(duk_context* pDuk);
static int __CPP_GameObject_TryGetScriptComponent(duk_context* pDuk);
static int __CPP_GameObject_SearchForChildByNameSequence(duk_context* pDuk);
static int __CPP_GameObject_SendMessage(duk_context* pDuk);
static int __CPP_GameObject_SendEventMessage(duk_context* pDuk);
static int __CPP_GameObject_SetString(duk_context* pDuk);
static int __CPP_GameObject_GetString(duk_context* pDuk);
static int __CPP_GameObject_SetTeamID(duk_context* pDuk);
static int __CPP_GameObject_GetTeamID(duk_context* pDuk);
static int __CPP_GameObject_ChangeTags(duk_context* pDuk);
static int __CPP_GameObject_CheckTags(duk_context* pDuk);
static int __CPP_GameObject_GetParent(duk_context* pDuk);
static int __CPP_GameObject_SetX_GameObject(duk_context* pDuk);
static int __CPP_GameObject_GetChildren(duk_context* pDuk);

namespace GameObject_X
{
  enum Enum
  {
    LocalPosition,
    GlobalPosition,
    LocalScaling,
    GlobalScaling,
    LocalUniformScaling,
    LocalRotation,
    GlobalRotation,
    GlobalDirForwards,
    GlobalDirRight,
    GlobalDirUp,
    LinearVelocity,
    ActiveFlag,
    Active,
    ChildCount,
    SetParent,
    AddChild,
    DetachChild,
  };
}

plResult plTypeScriptBinding::Init_GameObject()
{
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_IsValid", __CPP_GameObject_IsValid, 1);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetLocalPosition", __CPP_GameObject_SetX_Vec3, 2, GameObject_X::LocalPosition);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetLocalPosition", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::LocalPosition);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetGlobalPosition", __CPP_GameObject_SetX_Vec3, 2, GameObject_X::GlobalPosition);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalPosition", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::GlobalPosition);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetLocalScaling", __CPP_GameObject_SetX_Vec3, 2, GameObject_X::LocalScaling);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetLocalScaling", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::LocalScaling);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetGlobalScaling", __CPP_GameObject_SetX_Vec3, 2, GameObject_X::GlobalScaling);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalScaling", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::GlobalScaling);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetLocalUniformScaling", __CPP_GameObject_SetX_Float, 2, GameObject_X::LocalUniformScaling);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetLocalUniformScaling", __CPP_GameObject_GetX_Float, 1, GameObject_X::LocalUniformScaling);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetLocalRotation", __CPP_GameObject_SetX_Quat, 2, GameObject_X::LocalRotation);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetLocalRotation", __CPP_GameObject_GetX_Quat, 1, GameObject_X::LocalRotation);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetGlobalRotation", __CPP_GameObject_SetX_Quat, 2, GameObject_X::GlobalRotation);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalRotation", __CPP_GameObject_GetX_Quat, 1, GameObject_X::GlobalRotation);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetActiveFlag", __CPP_GameObject_SetX_Bool, 2, GameObject_X::ActiveFlag);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetActiveFlag", __CPP_GameObject_GetX_Bool, 1, GameObject_X::ActiveFlag);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_IsActive", __CPP_GameObject_GetX_Bool, 1, GameObject_X::Active);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_FindChildByName", __CPP_GameObject_FindChildByName, 3);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_FindChildByPath", __CPP_GameObject_FindChildByPath, 2);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_TryGetComponentOfBaseTypeName", __CPP_GameObject_TryGetComponentOfBaseTypeName, 2);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_TryGetComponentOfBaseTypeNameHash", __CPP_GameObject_TryGetComponentOfBaseTypeNameHash, 2);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_TryGetScriptComponent", __CPP_GameObject_TryGetScriptComponent, 2);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SearchForChildByNameSequence", __CPP_GameObject_SearchForChildByNameSequence, 3);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SendMessage", __CPP_GameObject_SendMessage, 5, 0);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_PostMessage", __CPP_GameObject_SendMessage, 5, 1);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SendEventMessage", __CPP_GameObject_SendEventMessage, 5, 0);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_PostEventMessage", __CPP_GameObject_SendEventMessage, 5, 1);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalDirForwards", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::GlobalDirForwards);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalDirRight", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::GlobalDirRight);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalDirUp", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::GlobalDirUp);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetVelocity", __CPP_GameObject_GetX_Vec3, 1, GameObject_X::LinearVelocity);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetName", __CPP_GameObject_SetString, 2, 0);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetName", __CPP_GameObject_GetString, 1, 0);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetGlobalKey", __CPP_GameObject_SetString, 2, 1);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetGlobalKey", __CPP_GameObject_GetString, 1, 1);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetTeamID", __CPP_GameObject_SetTeamID, 2);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetTeamID", __CPP_GameObject_GetTeamID, 1);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetParent", __CPP_GameObject_GetParent, 1);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_SetParent", __CPP_GameObject_SetX_GameObject, 3, GameObject_X::SetParent);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_AddChild", __CPP_GameObject_SetX_GameObject, 3, GameObject_X::AddChild);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_DetachChild", __CPP_GameObject_SetX_GameObject, 3, GameObject_X::DetachChild);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetChildCount", __CPP_GameObject_GetX_Float, 1, GameObject_X::ChildCount);
  m_Duk.RegisterGlobalFunction("__CPP_GameObject_GetChildren", __CPP_GameObject_GetChildren, 1);
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_GameObject_SetTags", __CPP_GameObject_ChangeTags, 0);
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_GameObject_AddTags", __CPP_GameObject_ChangeTags, 1);
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_GameObject_RemoveTags", __CPP_GameObject_ChangeTags, 2);
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_GameObject_HasAnyTags", __CPP_GameObject_CheckTags, 0);
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_GameObject_HasAllTags", __CPP_GameObject_CheckTags, 1);

  return PLASMA_SUCCESS;
}

plGameObjectHandle plTypeScriptBinding::RetrieveGameObjectHandle(duk_context* pDuk, plInt32 iObjIdx /*= 0 */)
{
  if (duk_is_null_or_undefined(pDuk, iObjIdx))
    return plGameObjectHandle();

  if (duk_get_prop_index(pDuk, iObjIdx, plTypeScriptBindingIndexProperty::GameObjectHandle))
  {
    plGameObjectHandle hObject = *reinterpret_cast<plGameObjectHandle*>(duk_get_buffer(pDuk, -1, nullptr));
    duk_pop(pDuk);
    return hObject;
  }

  return plGameObjectHandle();
}

plGameObject* plTypeScriptBinding::ExpectGameObject(duk_context* pDuk, plInt32 iObjIdx /*= 0*/)
{
  plGameObjectHandle hObject = plTypeScriptBinding::RetrieveGameObjectHandle(pDuk, 0 /*this*/);

  plWorld* pWorld = plTypeScriptBinding::RetrieveWorld(pDuk);

  plGameObject* pGameObject = nullptr;
  PLASMA_VERIFY(pWorld->TryGetObject(hObject, pGameObject), "Invalid plGameObject");

  return pGameObject;
}

bool plTypeScriptBinding::RegisterGameObject(plGameObjectHandle handle, plUInt32& out_uiStashIdx)
{
  if (handle.IsInvalidated())
    return false;

  plUInt32& uiStashIdx = m_GameObjectToStashIdx[handle];

  if (uiStashIdx != 0)
  {
    out_uiStashIdx = uiStashIdx;
    return true;
  }

  uiStashIdx = AcquireStashObjIndex();

  plDuktapeHelper duk(m_Duk);                                     // [ ]
  duk.PushGlobalObject();                                         // [ global ]
  PLASMA_VERIFY(duk.PushLocalObject("__GameObject").Succeeded(), ""); // [ global __GameObject ]
  duk_get_prop_string(duk, -1, "GameObject");                     // [ global __GameObject GameObject ]

  duk_new(duk, 0); // [ global __GameObject object ]

  // set the plGameObjectHandle property
  {
    plGameObjectHandle* pHandleBuffer =
      reinterpret_cast<plGameObjectHandle*>(duk_push_fixed_buffer(duk, sizeof(plGameObjectHandle))); // [ global __GameObject object buffer ]
    *pHandleBuffer = handle;
    duk_put_prop_index(duk, -2, plTypeScriptBindingIndexProperty::GameObjectHandle); // [ global __GameObject object ]
  }

  StoreReferenceInStash(duk, uiStashIdx); // [ global __GameObject object ]
  duk.PopStack(3);                        // [ ]

  out_uiStashIdx = uiStashIdx;
  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, true, 0);
}

bool plTypeScriptBinding::DukPutGameObject(const plGameObjectHandle& hObject)
{
  plDuktapeHelper duk(m_Duk);

  plUInt32 uiStashIdx = 0;
  if (!RegisterGameObject(hObject, uiStashIdx))
  {
    duk.PushNull(); // [ null ]
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, false, +1);
  }

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, DukPushStashObject(duk, uiStashIdx), +1);
}

void plTypeScriptBinding::DukPutGameObject(const plGameObject* pObject)
{
  if (pObject == nullptr)
  {
    duk_push_null(m_Duk);
  }
  else
  {
    DukPutGameObject(pObject->GetHandle());
  }
}

static int __CPP_GameObject_IsValid(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObjectHandle hObject = plTypeScriptBinding::RetrieveGameObjectHandle(pDuk, 0 /*this*/);

  if (hObject.IsInvalidated())
  {
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(false), +1);
  }

  plWorld* pWorld = plTypeScriptBinding::RetrieveWorld(pDuk);

  plGameObject* pGameObject = nullptr;
  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(pWorld->TryGetObject(hObject, pGameObject)), +1);
}

static int __CPP_GameObject_SetX_Vec3(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  if (!pGameObject->IsDynamic())
  {
    plLog::SeriousWarning(
      "TypeScript component modifies transform of static game-object '{}'. Use 'Force Dynamic' mode on owner game-object.", pGameObject->GetName());
    pGameObject->MakeDynamic();
  }

  const plVec3 value = plTypeScriptBinding::GetVec3(pDuk, 1);

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::LocalPosition:
      pGameObject->SetLocalPosition(value);
      break;

    case GameObject_X::GlobalPosition:
      pGameObject->SetGlobalPosition(value);
      break;

    case GameObject_X::LocalScaling:
      pGameObject->SetLocalScaling(value);
      break;

    case GameObject_X::GlobalScaling:
      pGameObject->SetGlobalScaling(value);
      break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetX_Vec3(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  plVec3 value;

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::LocalPosition:
      value = pGameObject->GetLocalPosition();
      break;

    case GameObject_X::GlobalPosition:
      value = pGameObject->GetGlobalPosition();
      break;

    case GameObject_X::LocalScaling:
      value = pGameObject->GetLocalScaling();
      break;

    case GameObject_X::GlobalScaling:
      value = pGameObject->GetGlobalScaling();
      break;

    case GameObject_X::GlobalDirForwards:
      value = pGameObject->GetGlobalDirForwards();
      break;

    case GameObject_X::GlobalDirRight:
      value = pGameObject->GetGlobalDirRight();
      break;

    case GameObject_X::GlobalDirUp:
      value = pGameObject->GetGlobalDirUp();
      break;

    case GameObject_X::LinearVelocity:
      value = pGameObject->GetLinearVelocity();
      break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }

  plTypeScriptBinding::PushVec3(pDuk, value);

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_SetX_Float(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  if (!pGameObject->IsDynamic())
  {
    plLog::SeriousWarning(
      "TypeScript component modifies transform of static game-object '{}'. Use 'Force Dynamic' mode on owner game-object.", pGameObject->GetName());
    pGameObject->MakeDynamic();
  }

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::LocalUniformScaling:
      pGameObject->SetLocalUniformScaling(duk.GetFloatValue(1));
      break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetX_Float(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  float value = 0.0f;

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::LocalUniformScaling:
      value = pGameObject->GetLocalUniformScaling();
      break;

    case GameObject_X::ChildCount:
      value = static_cast<float>(pGameObject->GetChildCount());
      break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnFloat(value), +1);
}

static int __CPP_GameObject_SetX_Quat(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  if (!pGameObject->IsDynamic())
  {
    plLog::SeriousWarning(
      "TypeScript component modifies transform of static game-object '{}'. Use 'Force Dynamic' mode on owner game-object.", pGameObject->GetName());
    pGameObject->MakeDynamic();
  }

  const plQuat rot = plTypeScriptBinding::GetQuat(pDuk, 1);

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::LocalRotation:
      pGameObject->SetLocalRotation(rot);
      break;

    case GameObject_X::GlobalRotation:
      pGameObject->SetGlobalRotation(rot);
      break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetX_Quat(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  plQuat value;

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::LocalRotation:
      value = pGameObject->GetLocalRotation();
      break;

    case GameObject_X::GlobalRotation:
      value = pGameObject->GetGlobalRotation();
      break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }

  plTypeScriptBinding::PushQuat(pDuk, value);

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_SetX_Bool(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const bool value = duk.GetBoolValue(1, true);

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::ActiveFlag:
      pGameObject->SetActiveFlag(value);
      break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetX_Bool(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  bool value = false;

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::ActiveFlag:
      value = pGameObject->GetActiveFlag();
      break;

    case GameObject_X::Active:
      value = pGameObject->IsActive();
      break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(value), +1);
}

static int __CPP_GameObject_FindChildByName(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const char* szName = duk.GetStringValue(1);
  bool bRecursive = duk.GetBoolValue(2, true);

  plGameObject* pChild = pGameObject->FindChildByName(plTempHashedString(szName), bRecursive);

  plTypeScriptBinding* pBinding = plTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(pChild);

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_FindChildByPath(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const char* szPath = duk.GetStringValue(1);

  plGameObject* pChild = pGameObject->FindChildByPath(szPath);

  plTypeScriptBinding* pBinding = plTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(pChild);

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_TryGetComponentOfBaseTypeName(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const char* szTypeName = duk.GetStringValue(1);

  const plRTTI* pRtti = plRTTI::FindTypeByName(szTypeName);
  if (pRtti == nullptr)
  {
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnNull(), +1);
  }

  plComponent* pComponent = nullptr;
  if (!pGameObject->TryGetComponentOfBaseType(pRtti, pComponent))
  {
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnNull(), +1);
  }

  plTypeScriptBinding* pBinding = plTypeScriptBinding::RetrieveBinding(duk);
  pBinding->DukPutComponentObject(pComponent);

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_TryGetComponentOfBaseTypeNameHash(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  plUInt32 uiTypeNameHash = duk.GetUIntValue(1);

  const plRTTI* pRtti = plRTTI::FindTypeByNameHash32(uiTypeNameHash);
  if (pRtti == nullptr)
  {
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnNull(), +1);
  }

  plComponent* pComponent = nullptr;
  if (!pGameObject->TryGetComponentOfBaseType(pRtti, pComponent))
  {
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnNull(), +1);
  }

  plTypeScriptBinding* pBinding = plTypeScriptBinding::RetrieveBinding(duk);
  pBinding->DukPutComponentObject(pComponent);

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_TryGetScriptComponent(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);
  const char* szTypeName = duk.GetStringValue(1);

  plTypeScriptBinding* pBinding = plTypeScriptBinding::RetrieveBinding(duk);

  plTypeScriptBinding::TsComponentTypeInfo info;
  if (pBinding->FindScriptComponentInfo(szTypeName, info).Succeeded())
  {
    plHybridArray<plTypeScriptComponent*, 8> components;
    pGameObject->TryGetComponentsOfBaseType(components);

    for (auto* pTs : components)
    {
      if (pTs->GetTypeScriptComponentGuid() == info.Key())
      {
        pBinding->DukPutComponentObject(pTs);
        goto found;
      }
    }
  }

  duk.PushNull();

found:
  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_SearchForChildByNameSequence(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const plUInt32 uiTypeNameHash = duk.GetUIntValue(2);

  const plRTTI* pRtti = nullptr;

  if (uiTypeNameHash != 0)
  {
    pRtti = plRTTI::FindTypeByNameHash32(uiTypeNameHash);
  }

  plGameObject* pObject = pGameObject->SearchForChildByNameSequence(duk.GetStringValue(1), pRtti);

  plTypeScriptBinding* pBinding = plTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(pObject);

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_SendMessage(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  plTypeScriptBinding* pBinding = plTypeScriptBinding::RetrieveBinding(duk);

  if (duk.GetFunctionMagicValue() == 0) // SendMessage
  {
    plUniquePtr<plMessage> pMsg = pBinding->MessageFromParameter(pDuk, 1, plTime::MakeZero());

    if (duk.GetBoolValue(3))
      pGameObject->SendMessageRecursive(*pMsg);
    else
      pGameObject->SendMessage(*pMsg);

    if (duk.GetBoolValue(4)) // expect the message to have result values
    {
      // sync msg back to TS
      plTypeScriptBinding::SyncPlasmaObjectToTsObject(pDuk, pMsg->GetDynamicRTTI(), pMsg.Borrow(), 1);
    }
  }
  else // PostMessage
  {
    const plTime delay = plTime::Seconds(duk.GetNumberValue(4));

    plUniquePtr<plMessage> pMsg = pBinding->MessageFromParameter(pDuk, 1, delay);

    if (duk.GetBoolValue(3))
      pGameObject->PostMessageRecursive(*pMsg, delay);
    else
      pGameObject->PostMessage(*pMsg, delay);
  }

  return duk.ReturnVoid();
}

static int __CPP_GameObject_SendEventMessage(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);
  plTypeScriptComponent* pSender = plTypeScriptBinding::ExpectComponent<plTypeScriptComponent>(duk, 3);

  plTypeScriptBinding* pBinding = plTypeScriptBinding::RetrieveBinding(duk);

  if (duk.GetFunctionMagicValue() == 0) // SendEventMessage
  {
    plUniquePtr<plMessage> pMsg = pBinding->MessageFromParameter(pDuk, 1, plTime::MakeZero());

    pGameObject->SendEventMessage(plStaticCast<plEventMessage&>(*pMsg), pSender);

    if (duk.GetBoolValue(4)) // expect the message to have result values
    {
      // sync msg back to TS
      plTypeScriptBinding::SyncPlasmaObjectToTsObject(pDuk, pMsg->GetDynamicRTTI(), pMsg.Borrow(), 1);
    }
  }
  else // PostEventMessage
  {
    const plTime delay = plTime::Seconds(duk.GetNumberValue(4));

    plUniquePtr<plMessage> pMsg = pBinding->MessageFromParameter(pDuk, 1, delay);

    pGameObject->PostEventMessage(plStaticCast<plEventMessage&>(*pMsg), pSender, delay);
  }

  return duk.ReturnVoid();
}


static int __CPP_GameObject_SetString(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  switch (duk.GetFunctionMagicValue())
  {
    case 0:
      pGameObject->SetName(duk.GetStringValue(1));
      break;
    case 1:
      pGameObject->SetGlobalKey(duk.GetStringValue(1));
      break;
  }

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetString(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  plStringView res;

  switch (duk.GetFunctionMagicValue())
  {
    case 0:
      res = pGameObject->GetName();
      break;
    case 1:
      res = pGameObject->GetGlobalKey();
      break;
  }

  return duk.ReturnString(res);
}

static int __CPP_GameObject_SetTeamID(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  pGameObject->SetTeamID(static_cast<plUInt16>(duk.GetUIntValue(1)));

  return duk.ReturnVoid();
}

static int __CPP_GameObject_GetTeamID(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnUInt(pGameObject->GetTeamID()), +1);
}

static int __CPP_GameObject_ChangeTags(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const plUInt32 uiMagic = duk.GetFunctionMagicValue();

  if (uiMagic == 0) // SetTags
  {
    pGameObject->SetTags(plTagSet());
  }

  for (plUInt32 i = 1; i < duk.GetNumVarArgFunctionParameters(); ++i)
  {
    const char* szParam = duk.GetStringValue(i, nullptr);

    if (!plStringUtils::IsNullOrEmpty(szParam))
    {
      switch (uiMagic)
      {
        case 0: // SetTags
        case 1: // AddTags
          pGameObject->SetTag(plTagRegistry::GetGlobalRegistry().RegisterTag(szParam));
          break;

        case 2: // RemoveTags
          pGameObject->RemoveTag(plTagRegistry::GetGlobalRegistry().RegisterTag(szParam));
          break;

        default:
          PLASMA_ASSERT_NOT_IMPLEMENTED;
      }
    }
  }

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

static int __CPP_GameObject_CheckTags(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const plUInt32 uiMagic = duk.GetFunctionMagicValue();

  bool result = (uiMagic == 1);

  for (plUInt32 i = 1; i < duk.GetNumVarArgFunctionParameters(); ++i)
  {
    const char* szParam = duk.GetStringValue(i, nullptr);

    if (!plStringUtils::IsNullOrEmpty(szParam))
    {
      const bool bIsSet = pGameObject->GetTags().IsSetByName(szParam);

      if (uiMagic == 0) // HasAnyTag
      {
        if (bIsSet)
        {
          result = true;
          break;
        }
      }
      else if (uiMagic == 1) // HasAllTags
      {
        if (!bIsSet)
        {
          result = false;
          break;
        }
      }
      else
      {
        PLASMA_ASSERT_NOT_IMPLEMENTED;
      }
    }
  }

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnBool(result), +1);
}

static int __CPP_GameObject_GetParent(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  plGameObject* pParent = pGameObject->GetParent();

  plTypeScriptBinding* pBinding = plTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(pParent);

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_GameObject_SetX_GameObject(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  plGameObjectHandle hObject = plTypeScriptBinding::RetrieveGameObjectHandle(pDuk, 1);

  const plGameObject::TransformPreservation preserve =
    duk.GetBoolValue(2) ? plGameObject::TransformPreservation::PreserveGlobal : plGameObject::TransformPreservation::PreserveLocal;

  switch (duk.GetFunctionMagicValue())
  {
    case GameObject_X::SetParent:
      pGameObject->SetParent(hObject, preserve);
      break;

    case GameObject_X::AddChild:
      pGameObject->AddChild(hObject, preserve);
      break;

    case GameObject_X::DetachChild:
      pGameObject->DetachChild(hObject, preserve);
      break;
  }

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

static int __CPP_GameObject_GetChildren(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plTypeScriptBinding* pBinding = plTypeScriptBinding::RetrieveBinding(pDuk);
  plGameObject* pGameObject = plTypeScriptBinding::ExpectGameObject(duk, 0 /*this*/);

  const int arrayObj = duk_push_array(pDuk);

  int curIdx = 0;
  for (auto it = pGameObject->GetChildren(); it.IsValid(); ++it, ++curIdx)
  {
    pBinding->DukPutGameObject(it);
    duk_put_prop_index(pDuk, arrayObj, curIdx);
  }

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}
