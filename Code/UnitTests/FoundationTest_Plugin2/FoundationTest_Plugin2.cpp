#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/ConstructionCounter.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>

static plInt32 g_iPluginState = -1;

void OnLoadPlugin();
void OnUnloadPlugin();

PLASMA_PLUGIN_DEPENDENCY(plasmaFoundationTest_Plugin1);

PLASMA_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

PLASMA_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

plCVarInt CVar_TestInt("test2_Int", 22, plCVarFlags::None, "Desc: test2_Int");
plCVarFloat CVar_TestFloat("test2_Float", 2.2f, plCVarFlags::Default, "Desc: test2_Float");
plCVarBool CVar_TestBool("test2_Bool", true, plCVarFlags::Save, "Desc: test2_Bool");
plCVarString CVar_TestString("test2_String", "test2", plCVarFlags::RequiresRestart, "Desc: test2_String");

plCVarBool CVar_TestInited("test2_Inited", false, plCVarFlags::None, "Desc: test2_Inited");

void OnLoadPlugin()
{
  PLASMA_TEST_BOOL_MSG(g_iPluginState == -1, "Plugin is in an invalid state.");
  g_iPluginState = 1;

  plCVarInt* pCVar = (plCVarInt*)plCVar::FindCVarByName("TestPlugin2InitCount");

  if (pCVar)
    *pCVar = *pCVar + 1;

  plCVarBool* pCVarDep = (plCVarBool*)plCVar::FindCVarByName("TestPlugin2FoundDependencies");

  if (pCVarDep)
  {
    *pCVarDep = true;

    // check that all CVars from plugin1 are available (ie. plugin1 is already loaded)
    *pCVarDep = *pCVarDep && (plCVar::FindCVarByName("test1_Int") != nullptr);
    *pCVarDep = *pCVarDep && (plCVar::FindCVarByName("test1_Float") != nullptr);
    *pCVarDep = *pCVarDep && (plCVar::FindCVarByName("test1_Bool") != nullptr);
    *pCVarDep = *pCVarDep && (plCVar::FindCVarByName("test1_String") != nullptr);
  }

  CVar_TestInited = true;
}

void OnUnloadPlugin()
{
  PLASMA_TEST_BOOL_MSG(g_iPluginState == 1, "Plugin is in an invalid state.");
  g_iPluginState = 2;

  plCVarInt* pCVar = (plCVarInt*)plCVar::FindCVarByName("TestPlugin2UninitCount");

  if (pCVar)
    *pCVar = *pCVar + 1;

  plCVarBool* pCVarDep = (plCVarBool*)plCVar::FindCVarByName("TestPlugin2FoundDependencies");

  if (pCVarDep)
  {
    *pCVarDep = true;

    // check that all CVars from plugin1 are STILL available (ie. plugin1 is not yet unloaded)
    *pCVarDep = *pCVarDep && (plCVar::FindCVarByName("test1_Int") != nullptr);
    *pCVarDep = *pCVarDep && (plCVar::FindCVarByName("test1_Float") != nullptr);
    *pCVarDep = *pCVarDep && (plCVar::FindCVarByName("test1_Bool") != nullptr);
    *pCVarDep = *pCVarDep && (plCVar::FindCVarByName("test1_String") != nullptr);
  }

  CVar_TestInited = false;
}

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(PluginGroup_Plugin2, TestSubSystem2)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "PluginGroup_Plugin1"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on
