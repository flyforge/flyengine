#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Configuration/CVar.h>

plCVarInt CVar_TestPlugin1InitializedCount("TestPlugin1InitCount", 0, plCVarFlags::None, "How often Plugin1 has been initialized.");
plCVarInt CVar_TestPlugin1UninitializedCount("TestPlugin1UninitCount", 0, plCVarFlags::None, "How often Plugin1 has been uninitialized.");
plCVarInt CVar_TestPlugin1Reloaded("TestPlugin1Reloaded", 0, plCVarFlags::None, "How often Plugin1 has been reloaded (counts init AND de-init).");

plCVarInt CVar_TestPlugin2InitializedCount("TestPlugin2InitCount", 0, plCVarFlags::None, "How often Plugin2 has been initialized.");
plCVarInt CVar_TestPlugin2UninitializedCount("TestPlugin2UninitCount", 0, plCVarFlags::None, "How often Plugin2 has been uninitialized.");
plCVarInt CVar_TestPlugin2Reloaded("TestPlugin2Reloaded", 0, plCVarFlags::None, "How often Plugin2 has been reloaded (counts init AND de-init).");
plCVarBool CVar_TestPlugin2FoundDependencies("TestPlugin2FoundDependencies", false, plCVarFlags::None, "Whether Plugin2 found all its dependencies (other plugins).");

PLASMA_CREATE_SIMPLE_TEST(Configuration, Plugin)
{
  CVar_TestPlugin1InitializedCount = 0;
  CVar_TestPlugin1UninitializedCount = 0;
  CVar_TestPlugin1Reloaded = 0;
  CVar_TestPlugin2InitializedCount = 0;
  CVar_TestPlugin2UninitializedCount = 0;
  CVar_TestPlugin2Reloaded = 0;
  CVar_TestPlugin2FoundDependencies = false;

#if PLASMA_ENABLED(PLASMA_SUPPORTS_DYNAMIC_PLUGINS) && PLASMA_ENABLED(PLASMA_COMPILE_ENGINE_AS_DLL)

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "LoadPlugin")
  {
    PLASMA_TEST_BOOL(plPlugin::LoadPlugin(plasmaFoundationTest_Plugin2) == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(plPlugin::LoadPlugin(plasmaFoundationTest_Plugin2, plPluginLoadFlags::PluginIsOptional) == PLASMA_SUCCESS); // loading already loaded plugin is always a success

    PLASMA_TEST_INT(CVar_TestPlugin1InitializedCount, 1);
    PLASMA_TEST_INT(CVar_TestPlugin2InitializedCount, 1);

    PLASMA_TEST_INT(CVar_TestPlugin1UninitializedCount, 0);
    PLASMA_TEST_INT(CVar_TestPlugin2UninitializedCount, 0);

    PLASMA_TEST_INT(CVar_TestPlugin1Reloaded, 0);
    PLASMA_TEST_INT(CVar_TestPlugin2Reloaded, 0);

    PLASMA_TEST_BOOL(CVar_TestPlugin2FoundDependencies);

    // this will fail the FoundationTests, as it logs an error
    // PLASMA_TEST_BOOL(plPlugin::LoadPlugin("Test") == PLASMA_FAILURE); // plugin does not exist
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "UnloadPlugin")
  {
    CVar_TestPlugin2FoundDependencies = false;
    plPlugin::UnloadAllPlugins();

    PLASMA_TEST_INT(CVar_TestPlugin1InitializedCount, 1);
    PLASMA_TEST_INT(CVar_TestPlugin2InitializedCount, 1);

    PLASMA_TEST_INT(CVar_TestPlugin1UninitializedCount, 1);
    PLASMA_TEST_INT(CVar_TestPlugin2UninitializedCount, 1);

    PLASMA_TEST_INT(CVar_TestPlugin1Reloaded, 0);
    PLASMA_TEST_INT(CVar_TestPlugin2Reloaded, 0);

    PLASMA_TEST_BOOL(CVar_TestPlugin2FoundDependencies);
    PLASMA_TEST_BOOL(plPlugin::LoadPlugin("Test", plPluginLoadFlags::PluginIsOptional) == PLASMA_FAILURE); // plugin does not exist
  }

#endif
}
