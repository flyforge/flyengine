#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Duktape/duktape.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static plHashTable<duk_context*, plTypeScriptBinding*> s_DukToBinding;

plSet<const plRTTI*> plTypeScriptBinding::s_RequiredEnums;
plSet<const plRTTI*> plTypeScriptBinding::s_RequiredFlags;

static int __CPP_Time_Get(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  switch (duk.GetFunctionMagicValue())
  {
    case 0:
      return duk.ReturnFloat(plTime::Now().AsFloatInSeconds());

    case 1:
    {
      plWorld* pWorld = plTypeScriptBinding::RetrieveWorld(pDuk);
      return duk.ReturnFloat(pWorld->GetClock().GetAccumulatedTime().AsFloatInSeconds());
    }

    case 2:
    {
      plWorld* pWorld = plTypeScriptBinding::RetrieveWorld(pDuk);
      return duk.ReturnFloat(pWorld->GetClock().GetTimeDiff().AsFloatInSeconds());
    }
  }

  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return duk.ReturnFloat(0.0f);
}

plTypeScriptBinding::plTypeScriptBinding()
  : m_Duk("Typescript Binding")
{
  s_DukToBinding[m_Duk] = this;
}

plTypeScriptBinding::~plTypeScriptBinding()
{
  if (!m_CVars.IsEmpty())
  {
    m_CVars.Clear();

    plCVar::ListOfCVarsChanged("invalid");
  }

  s_DukToBinding.Remove(m_Duk);
}

plResult plTypeScriptBinding::Init_Time()
{
  m_Duk.RegisterGlobalFunction("__CPP_Time_GetRealTime", __CPP_Time_Get, 0, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Time_GetGameTime", __CPP_Time_Get, 0, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Time_GetGameTimeDiff", __CPP_Time_Get, 0, 2);

  return PLASMA_SUCCESS;
}

plTypeScriptBinding* plTypeScriptBinding::RetrieveBinding(duk_context* pDuk)
{
  plTypeScriptBinding* pBinding = nullptr;
  s_DukToBinding.TryGetValue(pDuk, pBinding);
  return pBinding;
}

plResult plTypeScriptBinding::Initialize(plWorld& ref_world)
{
  if (m_bInitialized)
    return PLASMA_SUCCESS;

  PLASMA_LOG_BLOCK("Initialize TypeScript Binding");
  PLASMA_PROFILE_SCOPE("Initialize TypeScript Binding");

  m_hScriptCompendium = plResourceManager::LoadResource<plScriptCompendiumResource>(":project/AssetCache/Common/Scripts.plScriptCompendium");

  m_Duk.EnableModuleSupport(&plTypeScriptBinding::DukSearchModule);
  m_Duk.StorePointerInStash("plTypeScriptBinding", this);

  m_Duk.RegisterGlobalFunction("__CPP_Binding_RegisterMessageHandler", &plTypeScriptBinding::__CPP_Binding_RegisterMessageHandler, 2);

  StoreWorld(&ref_world);

  SetupRttiFunctionBindings();
  SetupRttiPropertyBindings();

  PLASMA_SUCCEED_OR_RETURN(Init_RequireModules());
  PLASMA_SUCCEED_OR_RETURN(Init_Log());
  PLASMA_SUCCEED_OR_RETURN(Init_Utils());
  PLASMA_SUCCEED_OR_RETURN(Init_Time());
  PLASMA_SUCCEED_OR_RETURN(Init_GameObject());
  PLASMA_SUCCEED_OR_RETURN(Init_FunctionBinding());
  PLASMA_SUCCEED_OR_RETURN(Init_PropertyBinding());
  PLASMA_SUCCEED_OR_RETURN(Init_Component());
  PLASMA_SUCCEED_OR_RETURN(Init_World());
  PLASMA_SUCCEED_OR_RETURN(Init_Clock());
  PLASMA_SUCCEED_OR_RETURN(Init_Debug());
  PLASMA_SUCCEED_OR_RETURN(Init_Random());
  PLASMA_SUCCEED_OR_RETURN(Init_Physics());

  m_bInitialized = true;
  return PLASMA_SUCCESS;
}

plResult plTypeScriptBinding::LoadComponent(const plUuid& typeGuid, TsComponentTypeInfo& out_typeInfo)
{
  if (!m_bInitialized || !typeGuid.IsValid())
  {
    return PLASMA_FAILURE;
  }

  // check if this component type has been loaded before
  {
    auto itLoaded = m_LoadedComponents.Find(typeGuid);

    if (itLoaded.IsValid())
    {
      out_typeInfo = m_TsComponentTypes.Find(typeGuid);
      return itLoaded.Value() ? PLASMA_SUCCESS : PLASMA_FAILURE;
    }
  }

  PLASMA_PROFILE_SCOPE("Load Script Component");

  bool& bLoaded = m_LoadedComponents[typeGuid];
  bLoaded = false;

  plResourceLock<plScriptCompendiumResource> pCompendium(m_hScriptCompendium, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pCompendium.GetAcquireResult() != plResourceAcquireResult::Final)
  {
    return PLASMA_FAILURE;
  }

  auto itType = pCompendium->GetDescriptor().m_AssetGuidToInfo.Find(typeGuid);
  if (!itType.IsValid())
  {
    return PLASMA_FAILURE;
  }

  const plString& sComponentPath = itType.Value().m_sComponentFilePath;
  const plString& sComponentName = itType.Value().m_sComponentTypeName;

  const plStringBuilder sCompModule("__", sComponentName);

  m_Duk.PushGlobalObject();

  plStringBuilder req;
  req.Format("var {} = require(\"./{}\");", sCompModule, itType.Value().m_sComponentFilePath);
  if (m_Duk.ExecuteString(req).Failed())
  {
    plLog::Error("Could not load component");
    return PLASMA_FAILURE;
  }

  m_Duk.PopStack();

  m_TsComponentTypes[typeGuid].m_sComponentTypeName = sComponentName;
  RegisterMessageHandlersForComponentType(sComponentName, typeGuid);

  bLoaded = true;

  out_typeInfo = m_TsComponentTypes.FindOrAdd(typeGuid, nullptr);

  return PLASMA_SUCCESS;
}

plResult plTypeScriptBinding::FindScriptComponentInfo(const char* szComponentType, TsComponentTypeInfo& out_typeInfo)
{
  for (auto it : m_TsComponentTypes)
  {
    if (it.Value().m_sComponentTypeName == szComponentType)
    {
      out_typeInfo = it;
      return PLASMA_SUCCESS;
    }
  }

  return PLASMA_FAILURE;
}

void plTypeScriptBinding::Update()
{
  ExecuteConsoleFuncs();
}

void plTypeScriptBinding::CleanupStash(plUInt32 uiNumIterations)
{
  if (!m_LastCleanupObj.IsValid())
    m_LastCleanupObj = m_GameObjectToStashIdx.GetIterator();

  plDuktapeHelper duk(m_Duk); // [ ]
  duk.PushGlobalStash();      // [ stash ]

  for (plUInt32 i = 0; i < uiNumIterations && m_LastCleanupObj.IsValid(); ++i)
  {
    plGameObject* pGO;
    if (!m_pWorld->TryGetObject(m_LastCleanupObj.Key(), pGO))
    {
      duk.PushNull();                                        // [ stash null ]
      duk_put_prop_index(duk, -2, m_LastCleanupObj.Value()); // [ stash ]

      ReleaseStashObjIndex(m_LastCleanupObj.Value());
      m_LastCleanupObj = m_GameObjectToStashIdx.Remove(m_LastCleanupObj);
    }
    else
    {
      ++m_LastCleanupObj;
    }
  }

  if (!m_LastCleanupComp.IsValid())
    m_LastCleanupComp = m_ComponentToStashIdx.GetIterator();

  for (plUInt32 i = 0; i < uiNumIterations && m_LastCleanupComp.IsValid(); ++i)
  {
    plComponent* pGO;
    if (!m_pWorld->TryGetComponent(m_LastCleanupComp.Key(), pGO))
    {
      duk.PushNull();                                         // [ stash null ]
      duk_put_prop_index(duk, -2, m_LastCleanupComp.Value()); // [ stash ]

      ReleaseStashObjIndex(m_LastCleanupComp.Value());
      m_LastCleanupComp = m_ComponentToStashIdx.Remove(m_LastCleanupComp);
    }
    else
    {
      ++m_LastCleanupComp;
    }
  }

  duk.PopStack(); // [ ]

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

plUInt32 plTypeScriptBinding::AcquireStashObjIndex()
{
  plUInt32 idx;

  if (!m_FreeStashObjIdx.IsEmpty())
  {
    idx = m_FreeStashObjIdx.PeekBack();
    m_FreeStashObjIdx.PopBack();
  }
  else
  {
    idx = m_uiNextStashObjIdx;
    ++m_uiNextStashObjIdx;
  }

  return idx;
}

void plTypeScriptBinding::ReleaseStashObjIndex(plUInt32 uiIdx)
{
  m_FreeStashObjIdx.PushBack(uiIdx);
}

void plTypeScriptBinding::StoreReferenceInStash(duk_context* pDuk, plUInt32 uiStashIdx)
{
  plDuktapeHelper duk(pDuk);               // [ object ]
  duk.PushGlobalStash();                   // [ object stash ]
  duk_dup(duk, -2);                        // [ object stash object ]
  duk_put_prop_index(duk, -2, uiStashIdx); // [ object stash ]
  duk.PopStack();                          // [ object ]

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

bool plTypeScriptBinding::DukPushStashObject(duk_context* pDuk, plUInt32 uiStashIdx)
{
  plDuktapeHelper duk(pDuk);

  duk.PushGlobalStash();          // [ stash ]
  duk_push_uint(duk, uiStashIdx); // [ stash idx ]

  if (!duk_get_prop(duk, -2)) // [ stash obj/undef ]
  {
    duk_pop_2(duk);     // [ ]
    duk_push_null(duk); // [ null ]
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, false, +1);
  }
  else // [ stash obj ]
  {
    duk_replace(duk, -2); // [ obj ]
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, true, +1);
  }
}

void plTypeScriptBinding::SyncTsObjectEzTsObject(duk_context* pDuk, const plRTTI* pRtti, void* pObject, plInt32 iObjIdx)
{
  plHybridArray<const plAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (const plAbstractProperty* pProp : properties)
  {
    if (pProp->GetCategory() != plPropertyCategory::Member)
      continue;

    auto pMember = static_cast<const plAbstractMemberProperty*>(pProp);

    const plVariant value = plTypeScriptBinding::GetVariantProperty(pDuk, pProp->GetPropertyName(), iObjIdx, pMember->GetSpecificType());

    if (value.IsValid())
    {
      plReflectionUtils::SetMemberPropertyValue(pMember, pObject, value);
    }
  }
}

void plTypeScriptBinding::SyncEzObjectToTsObject(duk_context* pDuk, const plRTTI* pRtti, const void* pObject, plInt32 iObjIdx)
{
  plDuktapeHelper duk(pDuk);

  plHybridArray<const plAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (const plAbstractProperty* pProp : properties)
  {
    if (pProp->GetCategory() != plPropertyCategory::Member)
      continue;

    auto pMember = static_cast<const plAbstractMemberProperty*>(pProp);

    const plRTTI* pType = pMember->GetSpecificType();

    if (pType->GetTypeFlags().IsAnySet(plTypeFlags::IsEnum | plTypeFlags::Bitflags))
    {
      const plVariant val = plReflectionUtils::GetMemberPropertyValue(pMember, pObject);

      SetVariantProperty(duk, pMember->GetPropertyName(), -1, val);
    }
    else
    {
      if (pType->GetVariantType() == plVariant::Type::Invalid)
        continue;

      const plVariant val = plReflectionUtils::GetMemberPropertyValue(pMember, pObject);

      SetVariantProperty(duk, pMember->GetPropertyName(), -1, val);
    }
  }

  PLASMA_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, 0);
}

void plTypeScriptBinding::GenerateConstructorString(plStringBuilder& out_String, const plVariant& value)
{
  out_String.Clear();

  const plVariant::Type::Enum type = value.GetType();

  switch (type)
  {
    case plVariant::Type::Invalid:
      break;

    case plVariant::Type::Bool:
    case plVariant::Type::Int8:
    case plVariant::Type::UInt8:
    case plVariant::Type::Int16:
    case plVariant::Type::UInt16:
    case plVariant::Type::Int32:
    case plVariant::Type::UInt32:
    case plVariant::Type::Int64:
    case plVariant::Type::UInt64:
    case plVariant::Type::Float:
    case plVariant::Type::Double:
    case plVariant::Type::String:
    case plVariant::Type::StringView:
    case plVariant::Type::HashedString:
    {
      out_String = value.ConvertTo<plString>();
      break;
    }

    case plVariant::Type::Color:
    case plVariant::Type::ColorGamma:
    {
      const plColor c = value.ConvertTo<plColor>();
      out_String.Format("new Color({}, {}, {}, {})", c.r, c.g, c.b, c.a);
      break;
    }

    case plVariant::Type::Vector2:
    {
      const plVec2 v = value.Get<plVec2>();
      out_String.Format("new Vec2({}, {})", v.x, v.y);
      break;
    }

    case plVariant::Type::Vector3:
    {
      const plVec3 v = value.Get<plVec3>();
      out_String.Format("new Vec3({}, {}, {})", v.x, v.y, v.z);
      break;
    }

    case plVariant::Type::Vector4:
    {
      const plVec4 v = value.Get<plVec4>();
      out_String.Format("new Vec4({}, {}, {}, {})", v.x, v.y, v.z, v.w);
      break;
    }

    case plVariant::Type::Vector2I:
    {
      const plVec2I32 v = value.Get<plVec2I32>();
      out_String.Format("new Vec2({}, {})", v.x, v.y);
      break;
    }

    case plVariant::Type::Vector3I:
    {
      const plVec3I32 v = value.Get<plVec3I32>();
      out_String.Format("new Vec3({}, {}, {})", v.x, v.y, v.z);
      break;
    }

    case plVariant::Type::Vector4I:
    {
      const plVec4I32 v = value.Get<plVec4I32>();
      out_String.Format("new Vec4({}, {}, {}, {})", v.x, v.y, v.z, v.w);
      break;
    }

    case plVariant::Type::Vector2U:
    {
      const plVec2U32 v = value.Get<plVec2U32>();
      out_String.Format("new Vec2({}, {})", v.x, v.y);
      break;
    }

    case plVariant::Type::Vector3U:
    {
      const plVec3U32 v = value.Get<plVec3U32>();
      out_String.Format("new Vec3({}, {}, {})", v.x, v.y, v.z);
      break;
    }

    case plVariant::Type::Vector4U:
    {
      const plVec4U32 v = value.Get<plVec4U32>();
      out_String.Format("new Vec4({}, {}, {}, {})", v.x, v.y, v.z, v.w);
      break;
    }

    case plVariant::Type::Quaternion:
    {
      const plQuat q = value.Get<plQuat>();
      out_String.Format("new Quat({}, {}, {}, {})", q.v.x, q.v.y, q.v.z, q.w);
      break;
    }

    case plVariant::Type::Matrix3:
    {
      out_String = "new Mat3()";
      break;
    }

    case plVariant::Type::Matrix4:
    {
      out_String = "new Mat4()";
      break;
    }

    case plVariant::Type::Transform:
    {
      out_String = "new Transform()";
      break;
    }

    case plVariant::Type::Time:
      out_String.Format("{0}", value.Get<plTime>().GetSeconds());
      break;

    case plVariant::Type::Angle:
      out_String.Format("{0}", value.Get<plAngle>().GetRadian());
      break;

    case plVariant::Type::Uuid:
    case plVariant::Type::DataBuffer:
    case plVariant::Type::VariantArray:
    case plVariant::Type::VariantDictionary:
    case plVariant::Type::TypedPointer:
    case plVariant::Type::TypedObject:
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      break;
  }
}
