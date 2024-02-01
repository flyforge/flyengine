#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/Console/ConsoleFunction.h>
#include <Duktape/duktape.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

static int __CPP_Debug_DrawCross(duk_context* pDuk);
static int __CPP_Debug_DrawLines(duk_context* pDuk);
static int __CPP_Debug_DrawBox(duk_context* pDuk);
static int __CPP_Debug_DrawSphere(duk_context* pDuk);
static int __CPP_Debug_Draw2DText(duk_context* pDuk);
static int __CPP_Debug_Draw3DText(duk_context* pDuk);
static int __CPP_Debug_DrawInfoText(duk_context* pDuk);
static int __CPP_Debug_GetResolution(duk_context* pDuk);
static int __CPP_Debug_ReadCVar(duk_context* pDuk);
static int __CPP_Debug_WriteCVar(duk_context* pDuk);
static int __CPP_Debug_RegisterCVar(duk_context* pDuk);
static int __CPP_Debug_RegisterCFunc(duk_context* pDuk);

plResult plTypeScriptBinding::Init_Debug()
{
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawCross", __CPP_Debug_DrawCross, 4);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawLines", __CPP_Debug_DrawLines, 2, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_Draw2DLines", __CPP_Debug_DrawLines, 2, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawLineBox", __CPP_Debug_DrawBox, 4, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawSolidBox", __CPP_Debug_DrawBox, 4, 1);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawLineSphere", __CPP_Debug_DrawSphere, 4);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_Draw2DText", __CPP_Debug_Draw2DText, 5);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_Draw3DText", __CPP_Debug_Draw3DText, 4);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_DrawInfoText", __CPP_Debug_DrawInfoText, 3);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_GetResolution", __CPP_Debug_GetResolution, 0);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_ReadCVarBool", __CPP_Debug_ReadCVar, 1, plCVarType::Bool);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_ReadCVarInt", __CPP_Debug_ReadCVar, 1, plCVarType::Int);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_ReadCVarFloat", __CPP_Debug_ReadCVar, 1, plCVarType::Float);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_ReadCVarString", __CPP_Debug_ReadCVar, 1, plCVarType::String);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_WriteCVarBool", __CPP_Debug_WriteCVar, 2, plCVarType::Bool);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_WriteCVarInt", __CPP_Debug_WriteCVar, 2, plCVarType::Int);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_WriteCVarFloat", __CPP_Debug_WriteCVar, 2, plCVarType::Float);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_WriteCVarString", __CPP_Debug_WriteCVar, 2, plCVarType::String);
  m_Duk.RegisterGlobalFunction("__CPP_Debug_RegisterCVar", __CPP_Debug_RegisterCVar, 4);
  m_Duk.RegisterGlobalFunctionWithVarArgs("__CPP_Debug_RegisterCFunc", __CPP_Debug_RegisterCFunc);

  return PL_SUCCESS;
}

static int __CPP_Debug_DrawCross(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);
  plWorld* pWorld = plTypeScriptBinding::RetrieveWorld(duk);

  const plVec3 pos = plTypeScriptBinding::GetVec3(pDuk, 0);
  const float size = duk.GetFloatValue(1);
  const plColor color = plTypeScriptBinding::GetColor(pDuk, 2);
  const plTransform transform = plTypeScriptBinding::GetTransform(pDuk, 3);

  plDebugRenderer::DrawCross(pWorld, pos, size, color, transform);

  return duk.ReturnVoid();
}

static int __CPP_Debug_DrawLines(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);
  plWorld* pWorld = plTypeScriptBinding::RetrieveWorld(duk);

  const plColor color = plTypeScriptBinding::GetColor(pDuk, 1);

  plUInt32 uiNumLines = (plUInt32)duk_get_length(pDuk, 0);
  plHybridArray<plDebugRenderer::Line, 32> lines;
  lines.SetCount(uiNumLines);

  for (plUInt32 i = 0; i < uiNumLines; ++i)
  {
    auto& line = lines[i];

    duk_get_prop_index(pDuk, 0, i);

    line.m_start.x = duk.GetFloatProperty("startX", 0.0f, -1);
    line.m_start.y = duk.GetFloatProperty("startY", 0.0f, -1);
    line.m_start.z = duk.GetFloatProperty("startZ", 0.0f, -1);

    line.m_end.x = duk.GetFloatProperty("endX", 0.0f, -1);
    line.m_end.y = duk.GetFloatProperty("endY", 0.0f, -1);
    line.m_end.z = duk.GetFloatProperty("endZ", 0.0f, -1);

    duk_pop(pDuk);
  }

  if (duk.GetFunctionMagicValue() == 0)
  {
    plDebugRenderer::DrawLines(pWorld, lines, color);
  }
  else
  {
    plDebugRenderer::Draw2DLines(pWorld, lines, color);
  }

  return duk.ReturnVoid();
}

static int __CPP_Debug_DrawBox(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);
  plWorld* pWorld = plTypeScriptBinding::RetrieveWorld(duk);

  const plVec3 vMin = plTypeScriptBinding::GetVec3(pDuk, 0);
  const plVec3 vMax = plTypeScriptBinding::GetVec3(pDuk, 1);
  const plColor color = plTypeScriptBinding::GetColor(pDuk, 2);
  const plTransform transform = plTypeScriptBinding::GetTransform(pDuk, 3);

  switch (duk.GetFunctionMagicValue())
  {
    case 0:
      plDebugRenderer::DrawLineBox(pWorld, plBoundingBox::MakeFromMinMax(vMin, vMax), color, transform);
      break;

    case 1:
      plDebugRenderer::DrawSolidBox(pWorld, plBoundingBox::MakeFromMinMax(vMin, vMax), color, transform);
      break;
  }

  return duk.ReturnVoid();
}

static int __CPP_Debug_DrawSphere(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);
  plWorld* pWorld = plTypeScriptBinding::RetrieveWorld(duk);

  const plVec3 vCenter = plTypeScriptBinding::GetVec3(pDuk, 0);
  const float fRadius = duk.GetFloatValue(1);
  const plColor color = plTypeScriptBinding::GetColor(pDuk, 2);
  const plTransform transform = plTypeScriptBinding::GetTransform(pDuk, 3);

  switch (duk.GetFunctionMagicValue())
  {
    case 0:
      plDebugRenderer::DrawLineSphere(pWorld, plBoundingSphere::MakeFromCenterAndRadius(vCenter, fRadius), color, transform);
      break;
  }

  return duk.ReturnVoid();
}

static int __CPP_Debug_Draw2DText(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);
  plWorld* pWorld = plTypeScriptBinding::RetrieveWorld(duk);

  const char* szText = duk.GetStringValue(0);
  const plVec2 vPos = plTypeScriptBinding::GetVec2(pDuk, 1);
  const plColor color = plTypeScriptBinding::GetColor(pDuk, 2);
  const float fSize = duk.GetFloatValue(3, 16.0f);
  plDebugTextHAlign::Enum halign = static_cast<plDebugTextHAlign::Enum>(duk.GetIntValue(4));

  plDebugRenderer::Draw2DText(pWorld, szText, plVec2I32((int)vPos.x, (int)vPos.y), color, (plUInt32)fSize, halign);

  return duk.ReturnVoid();
}

static int __CPP_Debug_Draw3DText(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);
  plWorld* pWorld = plTypeScriptBinding::RetrieveWorld(duk);

  const char* szText = duk.GetStringValue(0);
  const plVec3 vPos = plTypeScriptBinding::GetVec3(pDuk, 1);
  const plColor color = plTypeScriptBinding::GetColor(pDuk, 2);
  const float fSize = duk.GetFloatValue(3, 16.0f);

  plDebugRenderer::Draw3DText(pWorld, szText, vPos, color, (plUInt32)fSize);

  return duk.ReturnVoid();
}

static int __CPP_Debug_DrawInfoText(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);
  plWorld* pWorld = plTypeScriptBinding::RetrieveWorld(duk);

  const plInt32 corner = duk.GetIntValue(0);
  const char* szText = duk.GetStringValue(1);
  const plColor color = plTypeScriptBinding::GetColor(pDuk, 2);

  plDebugRenderer::DrawInfoText(pWorld, static_cast<plDebugTextPlacement::Enum>(corner), "Script", szText, color);

  return duk.ReturnVoid();
}

static int __CPP_Debug_GetResolution(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plVec2 resolution;

  for (const plViewHandle& hView : plRenderWorld::GetMainViews())
  {
    plView* pView;
    if (plRenderWorld::TryGetView(hView, pView))
    {
      resolution.x = pView->GetViewport().width;
      resolution.y = pView->GetViewport().height;
    }
  }

  plTypeScriptBinding::PushVec2(pDuk, resolution);
  return duk.ReturnCustom();
}

static int __CPP_Debug_ReadCVar(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  const char* szVarName = duk.GetStringValue(0);

  plCVar* pCVar = plCVar::FindCVarByName(szVarName);

  if (pCVar == nullptr || pCVar->GetType() != duk.GetFunctionMagicValue())
  {
    return duk.ReturnUndefined();
  }

  switch (duk.GetFunctionMagicValue())
  {
    case plCVarType::Bool:
    {
      plCVarBool* pVar = static_cast<plCVarBool*>(pCVar);
      return duk.ReturnBool(pVar->GetValue());
    }

    case plCVarType::Int:
    {
      plCVarInt* pVar = static_cast<plCVarInt*>(pCVar);
      return duk.ReturnInt(pVar->GetValue());
    }

    case plCVarType::Float:
    {
      plCVarFloat* pVar = static_cast<plCVarFloat*>(pCVar);
      return duk.ReturnNumber(pVar->GetValue());
    }

    case plCVarType::String:
    {
      plCVarString* pVar = static_cast<plCVarString*>(pCVar);
      return duk.ReturnString(pVar->GetValue());
    }

    default:
      PL_ASSERT_NOT_IMPLEMENTED;
  }

  return duk.ReturnUndefined();
}

static int __CPP_Debug_WriteCVar(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  const char* szVarName = duk.GetStringValue(0);

  plCVar* pCVar = plCVar::FindCVarByName(szVarName);

  if (pCVar == nullptr || pCVar->GetType() != duk.GetFunctionMagicValue())
  {
    duk.Error(plFmt("CVar '{}' does not exist.", szVarName));
    return duk.ReturnVoid();
  }

  switch (duk.GetFunctionMagicValue())
  {
    case plCVarType::Bool:
    {
      plCVarBool* pVar = static_cast<plCVarBool*>(pCVar);
      *pVar = duk.GetBoolValue(1, pVar->GetValue());
      break;
    }

    case plCVarType::Int:
    {
      plCVarInt* pVar = static_cast<plCVarInt*>(pCVar);
      *pVar = duk.GetIntValue(1, pVar->GetValue());
      break;
    }

    case plCVarType::Float:
    {
      plCVarFloat* pVar = static_cast<plCVarFloat*>(pCVar);
      *pVar = duk.GetFloatValue(1, pVar->GetValue());
      break;
    }

    case plCVarType::String:
    {
      plCVarString* pVar = static_cast<plCVarString*>(pCVar);
      *pVar = duk.GetStringValue(1, pVar->GetValue());
      break;
    }

    default:
      PL_ASSERT_NOT_IMPLEMENTED;
  }

  return duk.ReturnVoid();
}


static int __CPP_Debug_RegisterCVar(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plTypeScriptBinding* pBinding = plTypeScriptBinding::RetrieveBinding(pDuk);

  const char* szVarName = duk.GetStringValue(0);
  const plCVarType::Enum type = (plCVarType::Enum)duk.GetIntValue(1);
  const char* szDesc = duk.GetStringValue(3);

  auto& pCVar = pBinding->m_CVars[szVarName];

  if (pCVar != nullptr)
    return duk.ReturnVoid();

  switch (type)
  {
    case plCVarType::Int:
      pCVar = PL_DEFAULT_NEW(plCVarInt, szVarName, duk.GetIntValue(2), plCVarFlags::Default, szDesc);
      break;
    case plCVarType::Float:
      pCVar = PL_DEFAULT_NEW(plCVarFloat, szVarName, duk.GetFloatValue(2), plCVarFlags::Default, szDesc);
      break;
    case plCVarType::Bool:
      pCVar = PL_DEFAULT_NEW(plCVarBool, szVarName, duk.GetBoolValue(2), plCVarFlags::Default, szDesc);
      break;
    case plCVarType::String:
      pCVar = PL_DEFAULT_NEW(plCVarString, szVarName, duk.GetStringValue(2), plCVarFlags::Default, szDesc);
      break;

    default:
      duk.Error(plFmt("CVar '{}': invalid type {}", szVarName, (int)type));
      return duk.ReturnVoid();
  }

  plCVar::ListOfCVarsChanged("Scripting");

  return duk.ReturnVoid();
}

class TsConsoleFunc : public plConsoleFunctionBase
{

public:
  TsConsoleFunc(const char* szFunctionName, const char* szDescription)
    : plConsoleFunctionBase(szFunctionName, szDescription)
  {
  }

  plStaticArray<plVariant::Type::Enum, 8> m_Args;

  plUInt32 GetNumParameters() const override { return m_Args.GetCount(); }


  plVariant::Type::Enum GetParameterType(plUInt32 uiParam) const override { return m_Args[uiParam]; }

  plResult Call(plArrayPtr<plVariant> params) override
  {
    m_pBinding->StoreConsoleFuncCall(this, params);
    return PL_SUCCESS;
  }

  plResult DoCall(const plArrayPtr<plVariant>& params)
  {
    auto& cm = m_pBinding->m_ConsoleFuncs[GetName()];

    const plUInt32 uiNumArgs = params.GetCount();

    if (uiNumArgs != m_Args.GetCount())
      return PL_FAILURE;

    plDuktapeContext& duk = m_pBinding->GetDukTapeContext();

    // accessing the plWorld is the reason why the function call is stored and delayed
    // otherwise this would hang indefinitely
    PL_LOCK(m_pBinding->GetWorld()->GetWriteMarker());

    for (auto& reg : cm.m_Registered)
    {
      plComponent* pComponent;
      if (!reg.m_hOwner.IsInvalidated() && !m_pBinding->GetWorld()->TryGetComponent(reg.m_hOwner, pComponent))
      {
        reg.m_hOwner.Invalidate();
        continue;
      }

      duk.PushGlobalStash();                             // [ stash ]
      duk_get_prop_index(duk, -1, reg.m_uiFuncStashIdx); // [ stash func ]

      for (plUInt32 arg = 0; arg < uiNumArgs; ++arg)
      {
        plResult r = PL_FAILURE;

        switch (m_Args[arg])
        {
          case plVariant::Type::Bool:
            duk.PushBool(params[arg].ConvertTo<bool>(&r));
            break;
          case plVariant::Type::Double:
            duk.PushNumber(params[arg].ConvertTo<double>(&r));
            break;
          case plVariant::Type::String:
            duk.PushString(params[arg].ConvertTo<plString>(&r).GetData());
            break;

            PL_DEFAULT_CASE_NOT_IMPLEMENTED
        }

        if (r.Failed())
        {
          duk.Error(plFmt("Could not convert cfunc argument {} to expected type {}", arg, (int)m_Args[arg]));
          return PL_FAILURE;
        }
      }

      duk_call(duk, uiNumArgs); // [ stash result ]
      duk.PopStack(2);          // [ ]
    }

    return PL_SUCCESS;
  }

  plTypeScriptBinding* m_pBinding = nullptr;
};

void plTypeScriptBinding::StoreConsoleFuncCall(plConsoleFunctionBase* pFunc, const plArrayPtr<plVariant>& params)
{
  auto& call = m_CFuncCalls.ExpandAndGetRef();
  call.m_pFunc = pFunc;
  call.m_Arguments = params;
}

void plTypeScriptBinding::ExecuteConsoleFuncs()
{
  for (auto& call : m_CFuncCalls)
  {
    TsConsoleFunc* pFunc = static_cast<TsConsoleFunc*>(call.m_pFunc);
    pFunc->DoCall(call.m_Arguments.GetArrayPtr()).IgnoreResult();
  }

  m_CFuncCalls.Clear();
}

static int __CPP_Debug_RegisterCFunc(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plTypeScriptBinding* pBinding = plTypeScriptBinding::RetrieveBinding(pDuk);

  plComponentHandle hComponent = plTypeScriptBinding::ExpectComponent<plComponent>(pDuk, 0)->GetHandle();
  const char* szName = duk.GetStringValue(1);
  const char* szDesc = duk.GetStringValue(2);

  duk_require_function(pDuk, 3);

  auto& fb1 = pBinding->m_ConsoleFuncs[szName];

  if (fb1.m_pFunc == nullptr)
  {
    auto f = PL_DEFAULT_NEW(TsConsoleFunc, szName, szDesc);
    f->m_pBinding = pBinding;

    for (plUInt32 arg = 4; arg < duk.GetNumVarArgFunctionParameters(); ++arg)
    {
      const plInt32 iArgType = duk.GetIntValue(arg, plVariant::Type::Invalid);
      f->m_Args.PushBack(static_cast<plVariant::Type::Enum>(iArgType));
    }

    fb1.m_pFunc = f;
  }
  else
  {
    for (plUInt32 arg = 4; arg < duk.GetNumVarArgFunctionParameters(); ++arg)
    {
      const plInt32 iArgType = duk.GetIntValue(arg, plVariant::Type::Invalid);
      TsConsoleFunc* func = static_cast<TsConsoleFunc*>(fb1.m_pFunc.Borrow());

      if (func->m_Args[arg - 4] != static_cast<plVariant::Type::Enum>(iArgType))
      {
        duk.Error(plFmt("Re-registering cfunc '{}' with differing argument {} ({} != {}).", szName, arg - 4, iArgType, func->m_Args[arg - 4]));
        return duk.ReturnVoid();
      }
    }
  }

  auto& fb2 = fb1.m_Registered.ExpandAndGetRef();
  fb2.m_hOwner = hComponent;
  fb2.m_uiFuncStashIdx = pBinding->AcquireStashObjIndex();

  // store a reference to the console function in the stash
  {
    duk.PushGlobalStash();                             // [ stash ]
    duk_dup(duk, 3);                                   // [ stash func ]
    duk_put_prop_index(duk, -2, fb2.m_uiFuncStashIdx); // [ stash ]
    duk.PopStack();                                    // [ ]
  }

  return duk.ReturnVoid();
}
