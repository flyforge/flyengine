#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_World_DeleteObjectDelayed(duk_context* pDuk);
static int __CPP_World_CreateObject(duk_context* pDuk);
static int __CPP_World_CreateComponent(duk_context* pDuk);
static int __CPP_World_DeleteComponent(duk_context* pDuk);
static int __CPP_World_TryGetObjectWithGlobalKey(duk_context* pDuk);
static int __CPP_World_FindObjectsInSphere(duk_context* pDuk);
static int __CPP_World_FindObjectsInBox(duk_context* pDuk);

plHashTable<duk_context*, plWorld*> plTypeScriptBinding::s_DukToWorld;

plResult plTypeScriptBinding::Init_World()
{
  m_Duk.RegisterGlobalFunction("__CPP_World_DeleteObjectDelayed", __CPP_World_DeleteObjectDelayed, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_CreateObject", __CPP_World_CreateObject, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_CreateComponent", __CPP_World_CreateComponent, 2);
  m_Duk.RegisterGlobalFunction("__CPP_World_DeleteComponent", __CPP_World_DeleteComponent, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_TryGetObjectWithGlobalKey", __CPP_World_TryGetObjectWithGlobalKey, 1);
  m_Duk.RegisterGlobalFunction("__CPP_World_FindObjectsInSphere", __CPP_World_FindObjectsInSphere, 4);
  m_Duk.RegisterGlobalFunction("__CPP_World_FindObjectsInBox", __CPP_World_FindObjectsInBox, 4);

  return PL_SUCCESS;
}

void plTypeScriptBinding::StoreWorld(plWorld* pWorld)
{
  m_pWorld = pWorld;
  s_DukToWorld[m_Duk.GetContext()] = pWorld;
}

plWorld* plTypeScriptBinding::RetrieveWorld(duk_context* pDuk)
{
  plWorld* pWorld = nullptr;
  s_DukToWorld.TryGetValue(pDuk, pWorld);
  return pWorld;
}

static int __CPP_World_DeleteObjectDelayed(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plWorld* pWorld = plTypeScriptBinding::RetrieveWorld(duk);
  plGameObjectHandle hObject = plTypeScriptBinding::RetrieveGameObjectHandle(duk, 0 /*this*/);

  pWorld->DeleteObjectDelayed(hObject);

  return duk.ReturnVoid();
}

static int __CPP_World_CreateObject(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plWorld* pWorld = plTypeScriptBinding::RetrieveWorld(duk);

  plGameObjectDesc desc;

  desc.m_bActiveFlag = duk.GetBoolProperty("ActiveFlag", desc.m_bActiveFlag, 0);
  desc.m_bDynamic = duk.GetBoolProperty("Dynamic", desc.m_bDynamic, 0);
  desc.m_LocalPosition = plTypeScriptBinding::GetVec3Property(duk, "LocalPosition", 0, plVec3(0.0f));
  desc.m_LocalScaling = plTypeScriptBinding::GetVec3Property(duk, "LocalScaling", 0, plVec3(1.0f));
  desc.m_LocalRotation = plTypeScriptBinding::GetQuatProperty(duk, "LocalRotation", 0);
  desc.m_LocalUniformScaling = duk.GetFloatProperty("LocalUniformScaling", 1.0f, 0);
  desc.m_uiTeamID = static_cast<plUInt16>(duk.GetUIntProperty("TeamID", 0, 0));

  if (duk.PushLocalObject("Parent", 0).Succeeded())
  {
    desc.m_hParent = plTypeScriptBinding::RetrieveGameObjectHandle(duk, -1);
    duk.PopStack();
  }

  const char* szName = duk.GetStringProperty("Name", nullptr, 0);

  if (!plStringUtils::IsNullOrEmpty(szName))
  {
    desc.m_sName.Assign(szName);
  }

  plGameObjectHandle hObject = pWorld->CreateObject(desc);

  plTypeScriptBinding* pBinding = plTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(hObject);

  PL_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_World_CreateComponent(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plWorld* pWorld = plTypeScriptBinding::RetrieveWorld(duk);
  plGameObject* pOwner = plTypeScriptBinding::ExpectGameObject(duk, 0);

  const plUInt32 uiTypeNameHash = duk.GetUIntValue(1);

  const plRTTI* pRtti = plRTTI::FindTypeByNameHash32(uiTypeNameHash);
  if (pRtti == nullptr)
  {
    duk.Error(plFmt("Invalid component type name hash: {}", uiTypeNameHash));
    return duk.ReturnNull();
  }

  auto* pMan = pWorld->GetOrCreateManagerForComponentType(pRtti);

  plComponent* pComponent = nullptr;
  pMan->CreateComponent(pOwner, pComponent);

  plTypeScriptBinding* pBinding = plTypeScriptBinding::RetrieveBinding(duk);
  pBinding->DukPutComponentObject(pComponent);

  PL_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

static int __CPP_World_DeleteComponent(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plWorld* pWorld = plTypeScriptBinding::RetrieveWorld(duk);
  plComponentHandle hComponent = plTypeScriptBinding::RetrieveComponentHandle(duk, 0 /*this*/);

  plComponent* pComponent = nullptr;
  if (pWorld->TryGetComponent(hComponent, pComponent))
  {
    pComponent->GetOwningManager()->DeleteComponent(pComponent);
  }

  return duk.ReturnVoid();
}

static int __CPP_World_TryGetObjectWithGlobalKey(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plWorld* pWorld = plTypeScriptBinding::RetrieveWorld(duk);

  plGameObject* pObject = nullptr;
  bool objectExists = pWorld->TryGetObjectWithGlobalKey(plTempHashedString(duk.GetStringValue(0)), pObject);
  PL_IGNORE_UNUSED(objectExists);
  plTypeScriptBinding* pBinding = plTypeScriptBinding::RetrieveBinding(pDuk);
  pBinding->DukPutGameObject(pObject);

  PL_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
}

struct FindObjectsCallback
{
  duk_context* m_pDuk = nullptr;
  plTypeScriptBinding* m_pBinding = nullptr;

  plVisitorExecution::Enum Callback(plGameObject* pObject)
  {
    plDuktapeHelper duk(m_pDuk);

    if (!duk_get_global_string(m_pDuk, "callback")) // [ func ]
      return plVisitorExecution::Stop;

    PL_DUK_VERIFY_STACK(duk, +1);

    m_pBinding->DukPutGameObject(pObject); // [ func go ]

    PL_DUK_VERIFY_STACK(duk, +2);

    duk_call(m_pDuk, 1); // [ res ]

    PL_DUK_VERIFY_STACK(duk, +1);

    if (duk_get_boolean_default(m_pDuk, -1, false) == false)
    {
      duk_pop(m_pDuk); // [ ]
      return plVisitorExecution::Stop;
    }

    duk_pop(m_pDuk); // [ ]

    PL_DUK_VERIFY_STACK(duk, 0);
    return plVisitorExecution::Continue;
  }
};

static int __CPP_World_FindObjectsInSphere(duk_context* pDuk)
{
  duk_require_function(pDuk, -1); // last argument

  plDuktapeFunction duk(pDuk);
  plWorld* pWorld = plTypeScriptBinding::RetrieveWorld(duk);

  const char* szType = duk.GetStringValue(0);
  const plVec3 vSphereCenter = plTypeScriptBinding::GetVec3(pDuk, 1);
  const float fRadius = duk.GetFloatValue(2);

  duk_dup(pDuk, -1);
  duk_put_global_string(pDuk, "callback");

  FindObjectsCallback cb;
  cb.m_pDuk = pDuk;
  cb.m_pBinding = plTypeScriptBinding::RetrieveBinding(pDuk);

  auto category = plSpatialData::FindCategory(szType);

  if (category != plInvalidSpatialDataCategory)
  {
    plSpatialSystem::QueryParams queryParams;
    queryParams.m_uiCategoryBitmask = category.GetBitmask();

    pWorld->GetSpatialSystem()->FindObjectsInSphere(
      plBoundingSphere::MakeFromCenterAndRadius(vSphereCenter, fRadius), queryParams, plMakeDelegate(&FindObjectsCallback::Callback, &cb));
  }

  PL_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}

static int __CPP_World_FindObjectsInBox(duk_context* pDuk)
{
  duk_require_function(pDuk, -1); // last argument

  plDuktapeFunction duk(pDuk);
  plWorld* pWorld = plTypeScriptBinding::RetrieveWorld(duk);

  const char* szType = duk.GetStringValue(0);
  const plVec3 vBoxMin = plTypeScriptBinding::GetVec3(pDuk, 1);
  const plVec3 vBoxMax = plTypeScriptBinding::GetVec3(pDuk, 2);

  duk_dup(pDuk, -1);
  duk_put_global_string(pDuk, "callback");

  FindObjectsCallback cb;
  cb.m_pDuk = pDuk;
  cb.m_pBinding = plTypeScriptBinding::RetrieveBinding(pDuk);

  auto category = plSpatialData::FindCategory(szType);

  if (category != plInvalidSpatialDataCategory)
  {
    plSpatialSystem::QueryParams queryParams;
    queryParams.m_uiCategoryBitmask = category.GetBitmask();

    pWorld->GetSpatialSystem()->FindObjectsInBox(plBoundingBox::MakeFromMinMax(vBoxMin, vBoxMax), queryParams, plMakeDelegate(&FindObjectsCallback::Callback, &cb));
  }

  PL_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnVoid(), 0);
}
