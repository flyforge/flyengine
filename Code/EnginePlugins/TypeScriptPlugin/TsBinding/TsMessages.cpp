#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <TypeScriptPlugin/Components/TypeScriptComponent.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

void plTypeScriptBinding::GenerateMessagesFile(const char* szFile)
{
  plStringBuilder sFileContent;

  sFileContent =
    R"(// AUTO-GENERATED FILE

import __Message = require("TypeScript/pl/Message")
export import Message = __Message.Message;
export import EventMessage = __Message.EventMessage;

import __Vec2 = require("TypeScript/pl/Vec2")
export import Vec2 = __Vec2.Vec2;

import __Vec3 = require("TypeScript/pl/Vec3")
export import Vec3 = __Vec3.Vec3;

import __Mat3 = require("TypeScript/pl/Mat3")
export import Mat3 = __Mat3.Mat3;

import __Mat4 = require("TypeScript/pl/Mat4")
export import Mat4 = __Mat4.Mat4;

import __Quat = require("TypeScript/pl/Quat")
export import Quat = __Quat.Quat;

import __Transform = require("TypeScript/pl/Transform")
export import Transform = __Transform.Transform;

import __Color = require("TypeScript/pl/Color")
export import Color = __Color.Color;

import __Time = require("TypeScript/pl/Time")
export import Time = __Time.Time;

import __Angle = require("TypeScript/pl/Angle")
export import Angle = __Angle.Angle;

import Enum = require("./AllEnums")
import Flags = require("./AllFlags")


)";

  GenerateAllMessagesCode(sFileContent);

  plDeferredFileWriter file;
  file.SetOutput(szFile, true);

  file.WriteBytes(sFileContent.GetData(), sFileContent.GetElementCount()).IgnoreResult();

  if (file.Close().Failed())
  {
    plLog::Error("Failed to write file '{}'", szFile);
    return;
  }
}

static void CreateMessageTypeList(plSet<const plRTTI*>& ref_found, plDynamicArray<const plRTTI*>& ref_sorted, const plRTTI* pRtti)
{
  if (ref_found.Contains(pRtti))
    return;

  if (!pRtti->IsDerivedFrom<plMessage>())
    return;

  if (pRtti == plGetStaticRTTI<plMessage>() || pRtti == plGetStaticRTTI<plEventMessage>())
    return;

  ref_found.Insert(pRtti);
  CreateMessageTypeList(ref_found, ref_sorted, pRtti->GetParentType());

  ref_sorted.PushBack(pRtti);
}

static void CreateMessageTypeList(plDynamicArray<const plRTTI*>& out_sorted)
{
  plSet<const plRTTI*> found;
  out_sorted.Reserve(100);

  plHybridArray<const plRTTI*, 64> alphabetical;
  plRTTI::ForEachDerivedType<plMessage>([&](const plRTTI* pRtti) { alphabetical.PushBack(pRtti); });
  alphabetical.Sort([](const plRTTI* p1, const plRTTI* p2) -> bool { return p1->GetTypeName().Compare(p2->GetTypeName()) < 0; });

  for (auto pRtti : alphabetical)
  {
    CreateMessageTypeList(found, out_sorted, pRtti);
  }
}

void plTypeScriptBinding::GenerateAllMessagesCode(plStringBuilder& out_Code)
{
  plDynamicArray<const plRTTI*> sorted;
  CreateMessageTypeList(sorted);

  for (auto pRtti : sorted)
  {
    GenerateMessageCode(out_Code, pRtti);
  }
}

void plTypeScriptBinding::GenerateMessageCode(plStringBuilder& out_Code, const plRTTI* pRtti)
{
  plStringBuilder sType, sParentType;
  GetTsName(pRtti, sType);

  GetTsName(pRtti->GetParentType(), sParentType);

  out_Code.AppendFormat("export class {0} extends {1}\n", sType, sParentType);
  out_Code.Append("{\n");
  out_Code.AppendFormat("  public static GetTypeNameHash(): number { return {}; }\n", plHashingUtils::StringHashTo32(pRtti->GetTypeNameHash()));
  out_Code.AppendFormat("  constructor() { super(); this.TypeNameHash = {}; }\n", plHashingUtils::StringHashTo32(pRtti->GetTypeNameHash()));
  GenerateMessagePropertiesCode(out_Code, pRtti);
  out_Code.Append("}\n\n");
}

void plTypeScriptBinding::GenerateMessagePropertiesCode(plStringBuilder& out_Code, const plRTTI* pRtti)
{
  plStringBuilder sProp;
  plStringBuilder sDefault;

  for (const plAbstractProperty* pProp : pRtti->GetProperties())
  {
    if (pProp->GetCategory() != plPropertyCategory::Member)
      continue;

    auto pMember = static_cast<const plAbstractMemberProperty*>(pProp);

    const char* szTypeName = TsType(pMember->GetSpecificType());
    if (szTypeName == nullptr)
      continue;

    const plVariant defaultValue = plReflectionUtils::GetDefaultValue(pMember);
    GenerateConstructorString(sDefault, defaultValue);

    if (!sDefault.IsEmpty())
    {
      sProp.SetFormat("  {0}: {1} = {2};\n", pMember->GetPropertyName(), szTypeName, sDefault);
    }
    else
    {
      sProp.SetFormat("  {0}: {1};\n", pMember->GetPropertyName(), szTypeName);
    }

    out_Code.Append(sProp.GetView());
  }
}

void plTypeScriptBinding::InjectMessageImportExport(plStringBuilder& content, const char* szMessageFile)
{
  plDynamicArray<const plRTTI*> sorted;
  CreateMessageTypeList(sorted);

  plStringBuilder sImportExport, sTypeName;

  sImportExport.SetFormat(R"(import __AllMessages = require("{}")
)",
    szMessageFile);

  for (const plRTTI* pRtti : sorted)
  {
    GetTsName(pRtti, sTypeName);
    sImportExport.AppendFormat("export import {0} = __AllMessages.{0};\n", sTypeName);
  }

  AppendToTextFile(content, sImportExport);
}

static plUniquePtr<plMessage> CreateMessage(plUInt32 uiTypeHash, const plRTTI*& ref_pRtti)
{
  static plHashTable<plUInt32, const plRTTI*, plHashHelper<plUInt32>, plStaticsAllocatorWrapper> MessageTypes;

  if (!MessageTypes.TryGetValue(uiTypeHash, ref_pRtti))
  {
    ref_pRtti = plRTTI::FindTypeIf([=](const plRTTI* pRtti) { return plHashingUtils::StringHashTo32(pRtti->GetTypeNameHash()) == uiTypeHash; });
    MessageTypes[uiTypeHash] = ref_pRtti;
  }

  if (ref_pRtti == nullptr || !ref_pRtti->GetAllocator()->CanAllocate())
    return nullptr;

  return ref_pRtti->GetAllocator()->Allocate<plMessage>();
}

plUniquePtr<plMessage> plTypeScriptBinding::MessageFromParameter(duk_context* pDuk, plInt32 iObjIdx, plTime delay)
{
  plDuktapeHelper duk(pDuk);

  plUInt32 uiTypeNameHash = duk.GetUIntValue(iObjIdx);

  const plRTTI* pRtti = nullptr;
  plUniquePtr<plMessage> pMsg = CreateMessage(uiTypeNameHash, pRtti);

  if (pMsg != nullptr)
  {
    SyncTsObjectPlTsObject(pDuk, pRtti, pMsg.Borrow(), iObjIdx + 1);
  }
  else
  {
    plUInt32 uiMsgStashIdx = 0xFFFFFFFF;

    m_StashedMsgDelivery.SetCount(c_uiMaxMsgStash);
    const plTime tNow = m_pWorld->GetClock().GetAccumulatedTime();

    for (plUInt32 i = 0; i < c_uiMaxMsgStash; ++i)
    {
      m_uiNextStashMsgIdx++;

      if (m_uiNextStashMsgIdx >= c_uiLastStashMsgIdx)
        m_uiNextStashMsgIdx = c_uiFirstStashMsgIdx;

      if (m_StashedMsgDelivery[m_uiNextStashMsgIdx - c_uiFirstStashMsgIdx] < tNow)
        goto found;
    }

    plLog::Error("Too many posted messages with large delay (> {}). DukTape stash is full.", c_uiMaxMsgStash);
    PL_DUK_RETURN_AND_VERIFY_STACK(duk, nullptr, 0);

  found:

    m_StashedMsgDelivery[m_uiNextStashMsgIdx - c_uiFirstStashMsgIdx] = tNow + delay + plTime::MakeFromMilliseconds(50);

    {
      duk_dup(duk, iObjIdx + 1);                       // [ object ]
      StoreReferenceInStash(duk, m_uiNextStashMsgIdx); // [ object ]
      duk.PopStack();                                  // [ ]
    }

    pMsg = plGetStaticRTTI<plMsgTypeScriptMsgProxy>()->GetAllocator()->Allocate<plMsgTypeScriptMsgProxy>();
    plMsgTypeScriptMsgProxy* pTypedMsg = static_cast<plMsgTypeScriptMsgProxy*>(pMsg.Borrow());
    pTypedMsg->m_uiTypeNameHash = uiTypeNameHash;
    pTypedMsg->m_uiStashIndex = m_uiNextStashMsgIdx;
  }

  PL_DUK_VERIFY_STACK(duk, 0);
  return pMsg;
}

void plTypeScriptBinding::DukPutMessage(duk_context* pDuk, const plMessage& msg)
{
  plDuktapeHelper duk(pDuk);

  const plRTTI* pRtti = msg.GetDynamicRTTI();
  plStringBuilder sMsgName = pRtti->GetTypeName();
  sMsgName.TrimWordStart("pl");

  duk.PushGlobalObject();                              // [ global ]
  duk.PushLocalObject("__AllMessages").IgnoreResult(); // [ global __AllMessages ]
  duk_get_prop_string(duk, -1, sMsgName.GetData());    // [ global __AllMessages msgname ]
  duk_new(duk, 0);                                     // [ global __AllMessages msg ]
  duk_remove(duk, -2);                                 // [ global msg ]
  duk_remove(duk, -2);                                 // [ msg ]

  SyncPlObjectToTsObject(pDuk, pRtti, &msg, -1);

  PL_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, +1);
}

void plTypeScriptBinding::RegisterMessageHandlersForComponentType(const char* szComponent, const plUuid& componentType)
{
  plDuktapeHelper duk(m_Duk);

  m_CurrentTsMsgHandlerRegistrator = componentType;

  const plStringBuilder sCompModule("__", szComponent);

  duk.PushGlobalObject();                           // [ global ]
  if (duk.PushLocalObject(sCompModule).Succeeded()) // [ global __CompModule ]
  {
    if (duk.PushLocalObject(szComponent).Succeeded()) // [ global __CompModule obj ]
    {
      if (duk.PrepareObjectFunctionCall("RegisterMessageHandlers").Succeeded()) // [ global __CompModule obj func ]
      {
        duk.CallPreparedFunction().IgnoreResult(); // [ global __CompModule obj result ]
        duk.PopStack();                            // [ global __CompModule obj ]
      }

      duk.PopStack(); // [ global __CompModule ]
    }

    duk.PopStack(); // [ global ]
  }

  duk.PopStack(); // [ ]

  m_CurrentTsMsgHandlerRegistrator = plUuid::MakeInvalid();

  PL_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

int plTypeScriptBinding::__CPP_Binding_RegisterMessageHandler(duk_context* pDuk)
{
  plTypeScriptBinding* tsb = plTypeScriptBinding::RetrieveBinding(pDuk);

  PL_ASSERT_DEV(tsb->m_CurrentTsMsgHandlerRegistrator.IsValid(), "'pl.TypescriptComponent.RegisterMessageHandler' may only be called from 'static RegisterMessageHandlers()'");

  plDuktapeFunction duk(pDuk);

  plUInt32 uiMsgTypeHash = duk.GetUIntValue(0);
  const char* szMsgHandler = duk.GetStringValue(1);

  const plRTTI* pMsgType = plRTTI::FindTypeByNameHash32(uiMsgTypeHash);

  // this happens for pure TypeScript messages
  // if (pMsgType == nullptr)
  //{
  //  plLog::Error("Message with type name hash '{}' does not exist.", uiMsgTypeHash);
  //  return duk.ReturnVoid();
  //}

  auto& tsc = tsb->m_TsComponentTypes[tsb->m_CurrentTsMsgHandlerRegistrator];
  auto& mh = tsc.m_MessageHandlers.ExpandAndGetRef();

  mh.m_sHandlerFunc = szMsgHandler;
  mh.m_pMessageType = pMsgType;
  mh.m_uiMessageTypeNameHash = uiMsgTypeHash;

  PL_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

bool plTypeScriptBinding::HasMessageHandler(const TsComponentTypeInfo& typeInfo, const plRTTI* pMsgRtti) const
{
  if (!typeInfo.IsValid())
    return false;

  for (auto& mh : typeInfo.Value().m_MessageHandlers)
  {
    if (mh.m_pMessageType == pMsgRtti)
    {
      return true;
    }
  }

  return false;
}

bool plTypeScriptBinding::DeliverMessage(const TsComponentTypeInfo& typeInfo, plTypeScriptComponent* pComponent, plMessage& ref_msg, bool bSynchronizeAfterwards)
{
  if (!typeInfo.IsValid())
    return false;

  auto& tsc = typeInfo.Value();

  if (tsc.m_MessageHandlers.IsEmpty())
    return false;

  const plRTTI* pMsgRtti = ref_msg.GetDynamicRTTI();

  ++m_iMsgDeliveryRecursion;
  PL_SCOPE_EXIT(--m_iMsgDeliveryRecursion);

  plStringBuilder sStashMsgName;

  for (auto& mh : tsc.m_MessageHandlers)
  {
    if (mh.m_pMessageType == pMsgRtti)
    {
      plDuktapeHelper duk(m_Duk);

      DukPutComponentObject(pComponent); // [ comp ]

      if (duk.PrepareMethodCall(mh.m_sHandlerFunc).Succeeded()) // [ comp func comp ]
      {
        plTypeScriptBinding::DukPutMessage(duk, ref_msg); // [ comp func comp msg ]

        if (bSynchronizeAfterwards)
        {
          sStashMsgName.SetFormat("plMsg-{}", m_iMsgDeliveryRecursion);

          duk.PushGlobalStash();                       // [ ... stash ]
          duk_dup(duk, -2);                            // [ ... stash msg ]
          duk_put_prop_string(duk, -2, sStashMsgName); // [ ... stash ]
          duk_pop(duk);                                // [ ... ]
        }

        duk.PushCustom();                        // [ comp func comp msg ]
        duk.CallPreparedMethod().IgnoreResult(); // [ comp result ]
        duk.PopStack(2);                         // [ ]

        if (bSynchronizeAfterwards)
        {
          duk.PushGlobalStash();                                                    // [ ... stash ]
          duk_get_prop_string(duk, -1, sStashMsgName);                              // [ ... stash msg ]
          plTypeScriptBinding::SyncTsObjectPlTsObject(duk, pMsgRtti, &ref_msg, -1); // [ ... stash msg ]
          duk_pop_2(duk);                                                           // [ ... ]
        }

        PL_DUK_RETURN_AND_VERIFY_STACK(duk, true, 0);
      }
      else
      {
        // TODO: better error handling

        plLog::Error("{}.{}(msg: {}) does not exist", typeInfo.Key(), mh.m_sHandlerFunc, pMsgRtti->GetTypeName());

        // mh.m_pMessageType = nullptr;

        // remove 'this'   [ comp ]
        duk.PopStack(); // [ ]

        PL_DUK_RETURN_AND_VERIFY_STACK(duk, false, 0);
      }

      PL_DUK_RETURN_AND_VERIFY_STACK(duk, true, 0);
    }
  }

  return false;
}

bool plTypeScriptBinding::DeliverTsMessage(const TsComponentTypeInfo& typeInfo, plTypeScriptComponent* pComponent, const plMsgTypeScriptMsgProxy& msg)
{
  if (!typeInfo.IsValid())
    return false;

  auto& tsc = typeInfo.Value();

  for (auto& mh : tsc.m_MessageHandlers)
  {
    if (mh.m_uiMessageTypeNameHash == msg.m_uiTypeNameHash)
    {
      plDuktapeHelper duk(m_Duk);

      DukPutComponentObject(pComponent); // [ comp ]

      if (duk.PrepareMethodCall(mh.m_sHandlerFunc).Succeeded()) // [ comp func comp ]
      {
        DukPushStashObject(duk, msg.m_uiStashIndex); // [ comp func comp msg ]
        duk.PushCustom();                            // [ comp func comp msg ]
        duk.CallPreparedMethod().IgnoreResult();     // [ comp result ]
        duk.PopStack(2);                             // [ ]

        PL_DUK_RETURN_AND_VERIFY_STACK(duk, true, 0);
      }
      else
      {
        // TODO: better error handling

        plLog::Error("{}.{}(msg: {}) does not exist", typeInfo.Key(), mh.m_sHandlerFunc, msg.m_uiTypeNameHash);

        // mh.m_uiMessageTypeNameHash = 0;

        // remove 'this'   [ comp ]
        duk.PopStack(); // [ ]

        PL_DUK_RETURN_AND_VERIFY_STACK(duk, false, 0);
      }

      PL_DUK_RETURN_AND_VERIFY_STACK(duk, true, 0);
    }
  }

  return false;
}
