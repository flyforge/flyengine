#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Utilities/CommandLineUtils.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(Configuration);

#define plCVarValueDefault plCVarValue::Default
#define plCVarValueStored plCVarValue::Stored
#define plCVarValueRestart plCVarValue::Restart

// Interestingly using 'plCVarValue::Default' directly inside a macro does not work. (?!)
#define CHECK_CVAR(var, Current, Default, Stored, Restart)      \
  PLASMA_TEST_BOOL(var != nullptr);                                 \
  if (var != nullptr)                                           \
  {                                                             \
    PLASMA_TEST_BOOL(var->GetValue() == Current);                   \
    PLASMA_TEST_BOOL(var->GetValue(plCVarValueDefault) == Default); \
    PLASMA_TEST_BOOL(var->GetValue(plCVarValueStored) == Stored);   \
    PLASMA_TEST_BOOL(var->GetValue(plCVarValueRestart) == Restart); \
  }

static plInt32 iChangedValue = 0;
static plInt32 iChangedRestart = 0;

#if PLASMA_ENABLED(PLASMA_SUPPORTS_DYNAMIC_PLUGINS) && PLASMA_ENABLED(PLASMA_COMPILE_ENGINE_AS_DLL)

static void ChangedCVar(const plCVarEvent& e)
{
  switch (e.m_EventType)
  {
    case plCVarEvent::ValueChanged:
      ++iChangedValue;
      break;
    case plCVarEvent::RestartValueChanged:
      ++iChangedRestart;
      break;
    default:
      break;
  }
}

#endif

PLASMA_CREATE_SIMPLE_TEST(Configuration, CVars)
{
  iChangedValue = 0;
  iChangedRestart = 0;

  // setup the filesystem
  // we need it to test the storing of cvars (during plugin reloading)

  plStringBuilder sOutputFolder1 = plTestFramework::GetInstance()->GetAbsOutputPath();

  PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(sOutputFolder1.GetData(), "test", "output", plFileSystem::AllowWrites) == PLASMA_SUCCESS);

  // Delete all cvar setting files
  {
    plStringBuilder sConfigFile;

    sConfigFile = ":output/CVars/CVars_" plasmaFoundationTest_Plugin1 ".cfg";

    plFileSystem::DeleteFile(sConfigFile.GetData());

    sConfigFile = ":output/CVars/CVars_" plasmaFoundationTest_Plugin2 ".cfg";

    plFileSystem::DeleteFile(sConfigFile.GetData());
  }

  plCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-test1_Int2");
  plCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("102");

  plCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-test1_Float2");
  plCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("102.2");

  plCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-test1_Bool2");
  plCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("false");

  plCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-test1_String2");
  plCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("test1c");

  plCVar::SetStorageFolder(":output/CVars");
  plCVar::LoadCVars(); // should do nothing (no settings files available)

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "No Plugin Loaded")
  {
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_Int") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_Float") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_Bool") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_String") == nullptr);

    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_Int") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_Float") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_Bool") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_String") == nullptr);
  }

#if PLASMA_ENABLED(PLASMA_SUPPORTS_DYNAMIC_PLUGINS) && PLASMA_ENABLED(PLASMA_COMPILE_ENGINE_AS_DLL)

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Plugin1 Loaded")
  {
    PLASMA_TEST_BOOL(plPlugin::LoadPlugin(plasmaFoundationTest_Plugin1) == PLASMA_SUCCESS);

    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_Int") != nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_Float") != nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_Bool") != nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_String") != nullptr);

    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_Int2") != nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_Float2") != nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_Bool2") != nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_String2") != nullptr);

    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_Int") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_Float") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_Bool") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_String") == nullptr);

    plPlugin::UnloadAllPlugins();
  }

#endif

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "No Plugin Loaded (2)")
  {
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_Int") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_Float") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_Bool") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_String") == nullptr);

    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_Int") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_Float") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_Bool") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_String") == nullptr);
  }

#if PLASMA_ENABLED(PLASMA_SUPPORTS_DYNAMIC_PLUGINS) && PLASMA_ENABLED(PLASMA_COMPILE_ENGINE_AS_DLL)

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Plugin2 Loaded")
  {
    // Plugin2 should automatically load Plugin1 with it

    PLASMA_TEST_BOOL(plPlugin::LoadPlugin(plasmaFoundationTest_Plugin2) == PLASMA_SUCCESS);

    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_Int") != nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_Float") != nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_Bool") != nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_String") != nullptr);

    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_Int") != nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_Float") != nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_Bool") != nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_String") != nullptr);

    plPlugin::UnloadAllPlugins();
  }

#endif

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "No Plugin Loaded (2)")
  {
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_Int") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_Float") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_Bool") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test1_String") == nullptr);

    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_Int") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_Float") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_Bool") == nullptr);
    PLASMA_TEST_BOOL(plCVar::FindCVarByName("test2_String") == nullptr);
  }

#if PLASMA_ENABLED(PLASMA_SUPPORTS_DYNAMIC_PLUGINS) && PLASMA_ENABLED(PLASMA_COMPILE_ENGINE_AS_DLL)

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Default Value Test")
  {
    PLASMA_TEST_BOOL(plPlugin::LoadPlugin(plasmaFoundationTest_Plugin2) == PLASMA_SUCCESS);

    // CVars from Plugin 1
    {
      plCVarInt* pInt = (plCVarInt*)plCVar::FindCVarByName("test1_Int");
      CHECK_CVAR(pInt, 11, 11, 11, 11);

      if (pInt)
      {
        PLASMA_TEST_BOOL(pInt->GetType() == plCVarType::Int);
        PLASMA_TEST_BOOL(pInt->GetName() == "test1_Int");
        PLASMA_TEST_BOOL(pInt->GetDescription() == "Desc: test1_Int");

        pInt->m_CVarEvents.AddEventHandler(ChangedCVar);

        *pInt = 12;
        CHECK_CVAR(pInt, 12, 11, 11, 12);
        PLASMA_TEST_INT(iChangedValue, 1);
        PLASMA_TEST_INT(iChangedRestart, 0);

        // no change
        *pInt = 12;
        PLASMA_TEST_INT(iChangedValue, 1);
        PLASMA_TEST_INT(iChangedRestart, 0);
      }

      plCVarFloat* pFloat = (plCVarFloat*)plCVar::FindCVarByName("test1_Float");
      CHECK_CVAR(pFloat, 1.1f, 1.1f, 1.1f, 1.1f);

      if (pFloat)
      {
        PLASMA_TEST_BOOL(pFloat->GetType() == plCVarType::Float);
        PLASMA_TEST_BOOL(pFloat->GetName() == "test1_Float");
        PLASMA_TEST_BOOL(pFloat->GetDescription() == "Desc: test1_Float");

        pFloat->m_CVarEvents.AddEventHandler(ChangedCVar);

        *pFloat = 1.2f;
        CHECK_CVAR(pFloat, 1.1f, 1.1f, 1.1f, 1.2f);

        PLASMA_TEST_INT(iChangedValue, 1);
        PLASMA_TEST_INT(iChangedRestart, 1);

        // no change
        *pFloat = 1.2f;
        PLASMA_TEST_INT(iChangedValue, 1);
        PLASMA_TEST_INT(iChangedRestart, 1);

        pFloat->SetToRestartValue();
        CHECK_CVAR(pFloat, 1.2f, 1.1f, 1.1f, 1.2f);

        PLASMA_TEST_INT(iChangedValue, 2);
        PLASMA_TEST_INT(iChangedRestart, 1);
      }

      plCVarBool* pBool = (plCVarBool*)plCVar::FindCVarByName("test1_Bool");
      CHECK_CVAR(pBool, false, false, false, false);

      if (pBool)
      {
        PLASMA_TEST_BOOL(pBool->GetType() == plCVarType::Bool);
        PLASMA_TEST_BOOL(pBool->GetName() == "test1_Bool");
        PLASMA_TEST_BOOL(pBool->GetDescription() == "Desc: test1_Bool");

        *pBool = true;
        CHECK_CVAR(pBool, true, false, false, true);
      }

      plCVarString* pString = (plCVarString*)plCVar::FindCVarByName("test1_String");
      CHECK_CVAR(pString, "test1", "test1", "test1", "test1");

      if (pString)
      {
        PLASMA_TEST_BOOL(pString->GetType() == plCVarType::String);
        PLASMA_TEST_BOOL(pString->GetName() == "test1_String");
        PLASMA_TEST_BOOL(pString->GetDescription() == "Desc: test1_String");

        *pString = "test1_value2";
        CHECK_CVAR(pString, "test1_value2", "test1", "test1", "test1_value2");
      }
    }

    // CVars from Plugin 2
    {
      plCVarInt* pInt = (plCVarInt*)plCVar::FindCVarByName("test2_Int");
      CHECK_CVAR(pInt, 22, 22, 22, 22);

      if (pInt)
      {
        pInt->m_CVarEvents.AddEventHandler(ChangedCVar);

        *pInt = 23;
        CHECK_CVAR(pInt, 23, 22, 22, 23);
        PLASMA_TEST_INT(iChangedValue, 3);
        PLASMA_TEST_INT(iChangedRestart, 1);
      }

      plCVarFloat* pFloat = (plCVarFloat*)plCVar::FindCVarByName("test2_Float");
      CHECK_CVAR(pFloat, 2.2f, 2.2f, 2.2f, 2.2f);

      if (pFloat)
      {
        *pFloat = 2.3f;
        CHECK_CVAR(pFloat, 2.3f, 2.2f, 2.2f, 2.3f);
      }

      plCVarBool* pBool = (plCVarBool*)plCVar::FindCVarByName("test2_Bool");
      CHECK_CVAR(pBool, true, true, true, true);

      if (pBool)
      {
        *pBool = false;
        CHECK_CVAR(pBool, false, true, true, false);
      }

      plCVarString* pString = (plCVarString*)plCVar::FindCVarByName("test2_String");
      CHECK_CVAR(pString, "test2", "test2", "test2", "test2");

      if (pString)
      {
        *pString = "test2_value2";
        CHECK_CVAR(pString, "test2", "test2", "test2", "test2_value2");

        pString->SetToRestartValue();
        CHECK_CVAR(pString, "test2_value2", "test2", "test2", "test2_value2");
      }
    }

    plPlugin::UnloadAllPlugins();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Loaded Value Test")
  {
    PLASMA_TEST_BOOL(plPlugin::LoadPlugin(plasmaFoundationTest_Plugin2) == PLASMA_SUCCESS);

    // CVars from Plugin 1
    {
      plCVarInt* pInt = (plCVarInt*)plCVar::FindCVarByName("test1_Int");
      CHECK_CVAR(pInt, 12, 11, 12, 12);

      plCVarFloat* pFloat = (plCVarFloat*)plCVar::FindCVarByName("test1_Float");
      CHECK_CVAR(pFloat, 1.2f, 1.1f, 1.2f, 1.2f);

      plCVarBool* pBool = (plCVarBool*)plCVar::FindCVarByName("test1_Bool");
      CHECK_CVAR(pBool, false, false, false, false);

      plCVarString* pString = (plCVarString*)plCVar::FindCVarByName("test1_String");
      CHECK_CVAR(pString, "test1", "test1", "test1", "test1");
    }

    // CVars from Plugin 1, overridden by command line
    {
      plCVarInt* pInt = (plCVarInt*)plCVar::FindCVarByName("test1_Int2");
      CHECK_CVAR(pInt, 102, 21, 102, 102);

      plCVarFloat* pFloat = (plCVarFloat*)plCVar::FindCVarByName("test1_Float2");
      CHECK_CVAR(pFloat, 102.2f, 2.1f, 102.2f, 102.2f);

      plCVarBool* pBool = (plCVarBool*)plCVar::FindCVarByName("test1_Bool2");
      CHECK_CVAR(pBool, false, true, false, false);

      plCVarString* pString = (plCVarString*)plCVar::FindCVarByName("test1_String2");
      CHECK_CVAR(pString, "test1c", "test1b", "test1c", "test1c");
    }

    // CVars from Plugin 2
    {
      plCVarInt* pInt = (plCVarInt*)plCVar::FindCVarByName("test2_Int");
      CHECK_CVAR(pInt, 22, 22, 22, 22);

      plCVarFloat* pFloat = (plCVarFloat*)plCVar::FindCVarByName("test2_Float");
      CHECK_CVAR(pFloat, 2.2f, 2.2f, 2.2f, 2.2f);

      plCVarBool* pBool = (plCVarBool*)plCVar::FindCVarByName("test2_Bool");
      CHECK_CVAR(pBool, false, true, false, false);

      plCVarString* pString = (plCVarString*)plCVar::FindCVarByName("test2_String");
      CHECK_CVAR(pString, "test2_value2", "test2", "test2_value2", "test2_value2");
    }

    plPlugin::UnloadAllPlugins();
  }

#endif

  plFileSystem::ClearAllDataDirectories();
}
