#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/ConstructionCounter.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Reflection/Reflection.h>

static plInt32 g_iPluginState = -1;

void OnLoadPlugin();
void OnUnloadPlugin();

plCVarInt CVar_TestInt("test1_Int", 11, plCVarFlags::Save, "Desc: test1_Int");
plCVarFloat CVar_TestFloat("test1_Float", 1.1f, plCVarFlags::RequiresRestart, "Desc: test1_Float");
plCVarBool CVar_TestBool("test1_Bool", false, plCVarFlags::None, "Desc: test1_Bool");
plCVarString CVar_TestString("test1_String", "test1", plCVarFlags::Default, "Desc: test1_String");

plCVarInt CVar_TestInt2("test1_Int2", 21, plCVarFlags::Default, "Desc: test1_Int2");
plCVarFloat CVar_TestFloat2("test1_Float2", 2.1f, plCVarFlags::Default, "Desc: test1_Float2");
plCVarBool CVar_TestBool2("test1_Bool2", true, plCVarFlags::Default, "Desc: test1_Bool2");
plCVarString CVar_TestString2("test1_String2", "test1b", plCVarFlags::Default, "Desc: test1_String2");

PLASMA_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

PLASMA_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

void OnLoadPlugin()
{
  PLASMA_TEST_BOOL_MSG(g_iPluginState == -1, "Plugin is in an invalid state.");
  g_iPluginState = 1;

  plCVarInt* pCVar = (plCVarInt*)plCVar::FindCVarByName("TestPlugin1InitCount");

  if (pCVar)
    *pCVar = *pCVar + 1;

  plCVarBool* pCVarPlugin2Inited = (plCVarBool*)plCVar::FindCVarByName("test2_Inited");
  if (pCVarPlugin2Inited)
  {
    PLASMA_TEST_BOOL(*pCVarPlugin2Inited == false); // Although Plugin2 is present, it should not yet have been initialized
  }
}

void OnUnloadPlugin()
{
  PLASMA_TEST_BOOL_MSG(g_iPluginState == 1, "Plugin is in an invalid state.");
  g_iPluginState = 2;

  plCVarInt* pCVar = (plCVarInt*)plCVar::FindCVarByName("TestPlugin1UninitCount");

  if (pCVar)
    *pCVar = *pCVar + 1;
}

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(PluginGroup_Plugin1, TestSubSystem1)

  //BEGIN_SUBSYSTEM_DEPENDENCIES
  //  "PluginGroup_Plugin1"
  //END_SUBSYSTEM_DEPENDENCIES

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

struct plTestStruct2
{
  float m_fFloat2;

  plTestStruct2() { m_fFloat2 = 42.0f; }
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plTestStruct2);

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plTestStruct2, plNoBase, 1, plRTTIDefaultAllocator<plTestStruct2>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Float2", m_fFloat2),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on
