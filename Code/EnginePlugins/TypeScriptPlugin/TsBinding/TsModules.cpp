#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Duktape/duktape.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

plResult plTypeScriptBinding::Init_RequireModules()
{
  PLASMA_LOG_BLOCK("Init_RequireModules");

  if (m_Duk.ExecuteString("var __GameObject = require(\"./TypeScript/pl/GameObject\");").Failed())
  {
    plLog::Error("Failed to import 'GameObject.ts'");
    return PLASMA_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Component = require(\"./TypeScript/pl/Component\");").Failed())
  {
    plLog::Error("Failed to import 'Component.ts'");
    return PLASMA_FAILURE;
  }

  if (m_Duk.ExecuteString("var __AllComponents = require(\"./TypeScript/pl/AllComponents\");").Failed())
  {
    plLog::Error("Failed to import 'AllComponents.ts'");
    return PLASMA_FAILURE;
  }

  if (m_Duk.ExecuteString("var __AllMessages = require(\"./TypeScript/pl/AllMessages\");").Failed())
  {
    plLog::Error("Failed to import 'AllMessages.ts'");
    return PLASMA_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Log = require(\"./TypeScript/pl/Log\");").Failed())
  {
    plLog::Error("Failed to import 'Log.ts'");
    return PLASMA_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Vec2 = require(\"./TypeScript/pl/Vec2\");").Failed())
  {
    plLog::Error("Failed to import 'Vec2.ts'");
    return PLASMA_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Vec3 = require(\"./TypeScript/pl/Vec3\");").Failed())
  {
    plLog::Error("Failed to import 'Vec3.ts'");
    return PLASMA_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Mat3 = require(\"./TypeScript/pl/Mat3\");").Failed())
  {
    plLog::Error("Failed to import 'Mat3.ts'");
    return PLASMA_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Mat4 = require(\"./TypeScript/pl/Mat4\");").Failed())
  {
    plLog::Error("Failed to import 'Mat4.ts'");
    return PLASMA_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Quat = require(\"./TypeScript/pl/Quat\");").Failed())
  {
    plLog::Error("Failed to import 'Quat.ts'");
    return PLASMA_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Color = require(\"./TypeScript/pl/Color\");").Failed())
  {
    plLog::Error("Failed to import 'Color.ts'");
    return PLASMA_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Transform = require(\"./TypeScript/pl/Transform\");").Failed())
  {
    plLog::Error("Failed to import 'Transform.ts'");
    return PLASMA_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Time = require(\"./TypeScript/pl/Time\");").Failed())
  {
    plLog::Error("Failed to import 'Time.ts'");
    return PLASMA_FAILURE;
  }

  if (m_Duk.ExecuteString("var __Physics = require(\"./TypeScript/pl/Physics\");").Failed())
  {
    plLog::Error("Failed to import 'Physics.ts'");
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

int plTypeScriptBinding::DukSearchModule(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  plStringBuilder sRequestedFile = duk.GetStringValue(0);
  if (!sRequestedFile.HasAnyExtension())
  {
    sRequestedFile.ChangeFileExtension("ts");
  }

  PLASMA_LOG_BLOCK("DukSearchModule", sRequestedFile);

  plTypeScriptBinding* pBinding = static_cast<plTypeScriptBinding*>(duk.RetrievePointerFromStash("plTypeScriptBinding"));

  plResourceLock<plScriptCompendiumResource> pCompendium(pBinding->m_hScriptCompendium, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pCompendium.GetAcquireResult() != plResourceAcquireResult::Final)
  {
    duk.PushUndefined();
    duk.Error(plFmt("'required' module \"{}\" could not be loaded: JsLib resource is missing.", sRequestedFile));
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
  }

  auto it = pCompendium->GetDescriptor().m_PathToSource.Find(sRequestedFile);

  if (!it.IsValid())
  {
    duk.PushUndefined();
    duk.Error(plFmt("'required' module \"{}\" could not be loaded: JsLib resource does not contain source for it.", sRequestedFile));
    PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnCustom(), +1);
  }

  PLASMA_DUK_RETURN_AND_VERIFY_STACK(duk, duk.ReturnString(it.Value()), +1);
}
