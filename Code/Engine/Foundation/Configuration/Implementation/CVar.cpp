#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>

// clang-format off
PLASMA_ENUMERABLE_CLASS_IMPLEMENTATION(plCVar);

// The CVars need to be saved and loaded whenever plugins are loaded and unloaded.
// Therefore we register as early as possible (Base Startup) at the plugin system,
// to be informed about plugin changes.
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(Foundation, CVars)

  // for saving and loading we need the filesystem, so make sure we are initialized after
  // and shutdown before the filesystem is
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "FileSystem"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plPlugin::Events().AddEventHandler(plCVar::PluginEventHandler);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    // save the CVars every time the core is shut down
    // at this point the filesystem might already be uninitialized by the user (data dirs)
    // in that case the variables cannot be saved, but it will fail silently
    // if it succeeds, the most recent state will be serialized though
    plCVar::SaveCVars();

    plPlugin::Events().RemoveEventHandler(plCVar::PluginEventHandler);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    // save the CVars every time the engine is shut down
    // at this point the filesystem should usually still be configured properly
    plCVar::SaveCVars();
  }

  // The user is responsible to call 'plCVar::SetStorageFolder' to define where the CVars are
  // actually stored. That call will automatically load all CVar states.

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on


plString plCVar::s_sStorageFolder;
plEvent<const plCVarEvent&> plCVar::s_AllCVarEvents;

void plCVar::AssignSubSystemPlugin(plStringView sPluginName)
{
  plCVar* pCVar = plCVar::GetFirstInstance();

  while (pCVar)
  {
    if (pCVar->m_sPluginName.IsEmpty())
      pCVar->m_sPluginName = sPluginName;

    pCVar = pCVar->GetNextInstance();
  }
}

void plCVar::PluginEventHandler(const plPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case plPluginEvent::BeforeLoading:
    {
      // before a new plugin is loaded, make sure all currently available CVars
      // are assigned to the proper plugin
      // all not-yet assigned cvars cannot be in any plugin, so assign them to the 'static' plugin
      AssignSubSystemPlugin("Static");
    }
    break;

    case plPluginEvent::AfterLoadingBeforeInit:
    {
      // after we loaded a new plugin, but before it is initialized,
      // find all new CVars and assign them to that new plugin
      AssignSubSystemPlugin(EventData.m_sPluginBinary);

      // now load the state of all CVars
      LoadCVars();
    }
    break;

    case plPluginEvent::BeforeUnloading:
    {
      SaveCVars();
    }
    break;

    default:
      break;
  }
}

plCVar::plCVar(plStringView sName, plBitflags<plCVarFlags> Flags, plStringView sDescription)
  : m_sName(sName)
  , m_sDescription(sDescription)
  , m_Flags(Flags)
{
  // 'RequiresRestart' only works together with 'Save'
  if (m_Flags.IsAnySet(plCVarFlags::RequiresRestart))
    m_Flags.Add(plCVarFlags::Save);

  PLASMA_ASSERT_DEV(!m_sDescription.IsEmpty(), "Please add a useful description for CVar '{}'.", sName);
}

plCVar* plCVar::FindCVarByName(plStringView sName)
{
  plCVar* pCVar = plCVar::GetFirstInstance();

  while (pCVar)
  {
    if (pCVar->GetName().IsEqual_NoCase(sName))
      return pCVar;

    pCVar = pCVar->GetNextInstance();
  }

  return nullptr;
}

void plCVar::SetStorageFolder(plStringView sFolder)
{
  s_sStorageFolder = sFolder;
}

plCommandLineOptionBool opt_NoFileCVars("cvar", "-no-file-cvars", "Disables loading CVar values from the user-specific, persisted configuration file.", false);

void plCVar::SaveCVarsToFile(plStringView sPath, bool bIgnoreSaveFlag)
{
  plHybridArray<plCVar*, 128> allCVars;

  for (plCVar* pCVar = plCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    if (bIgnoreSaveFlag || pCVar->GetFlags().IsAnySet(plCVarFlags::Save))
    {
      allCVars.PushBack(pCVar);
    }
  }

  SaveCVarsToFileInternal(sPath, allCVars);
}

void plCVar::SaveCVars()
{
  if (s_sStorageFolder.IsEmpty())
    return;

  // this command line disables loading and saving CVars to and from files
  if (opt_NoFileCVars.GetOptionValue(plCommandLineOption::LogMode::FirstTimeIfSpecified))
    return;

  // first gather all the cvars by plugin
  plMap<plString, plHybridArray<plCVar*, 128>> PluginCVars;

  {
    plCVar* pCVar = plCVar::GetFirstInstance();
    while (pCVar)
    {
      // only store cvars that should be saved
      if (pCVar->GetFlags().IsAnySet(plCVarFlags::Save))
      {
        if (!pCVar->m_sPluginName.IsEmpty())
          PluginCVars[pCVar->m_sPluginName].PushBack(pCVar);
        else
          PluginCVars["Static"].PushBack(pCVar);
      }

      pCVar = pCVar->GetNextInstance();
    }
  }

  plMap<plString, plHybridArray<plCVar*, 128>>::Iterator it = PluginCVars.GetIterator();

  plStringBuilder sTemp;

  // now save all cvars in their plugin specific file
  while (it.IsValid())
  {
    // create the plugin specific file
    sTemp.Format("{0}/CVars_{1}.cfg", s_sStorageFolder, it.Key());

    SaveCVarsToFileInternal(sTemp, it.Value());

    // continue with the next plugin
    ++it;
  }
}

void plCVar::SaveCVarsToFileInternal(plStringView path, const plDynamicArray<plCVar*>& vars)
{
  plStringBuilder sTemp;
  plFileWriter File;
  if (File.Open(path.GetData(sTemp)) == PLASMA_SUCCESS)
  {
    // write one line for each cvar, to save its current value
    for (plUInt32 var = 0; var < vars.GetCount(); ++var)
    {
      plCVar* pCVar = vars[var];

      switch (pCVar->GetType())
      {
        case plCVarType::Int:
        {
          plCVarInt* pInt = (plCVarInt*)pCVar;
          sTemp.Format("{0} = {1}\n", pCVar->GetName(), pInt->GetValue(plCVarValue::Restart));
        }
        break;
        case plCVarType::Bool:
        {
          plCVarBool* pBool = (plCVarBool*)pCVar;
          sTemp.Format("{0} = {1}\n", pCVar->GetName(), pBool->GetValue(plCVarValue::Restart) ? "true" : "false");
        }
        break;
        case plCVarType::Float:
        {
          plCVarFloat* pFloat = (plCVarFloat*)pCVar;
          sTemp.Format("{0} = {1}\n", pCVar->GetName(), pFloat->GetValue(plCVarValue::Restart));
        }
        break;
        case plCVarType::String:
        {
          plCVarString* pString = (plCVarString*)pCVar;
          sTemp.Format("{0} = \"{1}\"\n", pCVar->GetName(), pString->GetValue(plCVarValue::Restart));
        }
        break;
        default:
          PLASMA_REPORT_FAILURE("Unknown CVar Type: {0}", pCVar->GetType());
          break;
      }

      // add the one line for that cvar to the config file
      File.WriteBytes(sTemp.GetData(), sTemp.GetElementCount()).IgnoreResult();
    }
  }
}

void plCVar::LoadCVars(bool bOnlyNewOnes /*= true*/, bool bSetAsCurrentValue /*= true*/)
{
  LoadCVarsFromCommandLine(bOnlyNewOnes, bSetAsCurrentValue);
  LoadCVarsFromFile(bOnlyNewOnes, bSetAsCurrentValue);
}

static plResult ParseLine(const plString& sLine, plStringBuilder& out_sVarName, plStringBuilder& out_sVarValue)
{
  const char* szSign = sLine.FindSubString("=");

  if (szSign == nullptr)
    return PLASMA_FAILURE;

  {
    plStringView sSubString(sLine.GetData(), szSign);

    // remove all trailing spaces
    while (sSubString.EndsWith(" "))
      sSubString.Shrink(0, 1);

    out_sVarName = sSubString;
  }

  {
    plStringView sSubString(szSign + 1);

    // remove all spaces
    while (sSubString.StartsWith(" "))
      sSubString.Shrink(1, 0);

    // remove all trailing spaces
    while (sSubString.EndsWith(" "))
      sSubString.Shrink(0, 1);


    // remove " and start and end

    if (sSubString.StartsWith("\""))
      sSubString.Shrink(1, 0);

    if (sSubString.EndsWith("\""))
      sSubString.Shrink(0, 1);

    out_sVarValue = sSubString;
  }

  return PLASMA_SUCCESS;
}

void plCVar::LoadCVarsFromFile(bool bOnlyNewOnes, bool bSetAsCurrentValue, plDynamicArray<plCVar*>* pOutCVars)
{
  if (s_sStorageFolder.IsEmpty())
    return;

  // this command line disables loading and saving CVars to and from files
  if (opt_NoFileCVars.GetOptionValue(plCommandLineOption::LogMode::FirstTimeIfSpecified))
    return;

  plMap<plString, plHybridArray<plCVar*, 128>> PluginCVars;

  // first gather all the cvars by plugin
  {
    for (plCVar* pCVar = plCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
    {
      // only load cvars that should be saved
      if (pCVar->GetFlags().IsAnySet(plCVarFlags::Save))
      {
        if (!bOnlyNewOnes || pCVar->m_bHasNeverBeenLoaded)
        {
          if (!pCVar->m_sPluginName.IsEmpty())
            PluginCVars[pCVar->m_sPluginName].PushBack(pCVar);
          else
            PluginCVars["Static"].PushBack(pCVar);
        }
      }

      // it doesn't matter whether the CVar could be loaded from file, either it works the first time, or it stays at its current value
      pCVar->m_bHasNeverBeenLoaded = false;
    }
  }

  {
    plMap<plString, plHybridArray<plCVar*, 128>>::Iterator it = PluginCVars.GetIterator();

    plStringBuilder sTemp;

    while (it.IsValid())
    {
      // create the plugin specific file
      sTemp.Format("{0}/CVars_{1}.cfg", s_sStorageFolder, it.Key());

      LoadCVarsFromFileInternal(sTemp.GetView(), it.Value(), bOnlyNewOnes, bSetAsCurrentValue, pOutCVars);

      // continue with the next plugin
      ++it;
    }
  }
}

void plCVar::LoadCVarsFromFile(plStringView sPath, bool bOnlyNewOnes, bool bSetAsCurrentValue, bool bIgnoreSaveFlag, plDynamicArray<plCVar*>* pOutCVars)
{
  plHybridArray<plCVar*, 128> allCVars;

  for (plCVar* pCVar = plCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    if (bIgnoreSaveFlag || pCVar->GetFlags().IsAnySet(plCVarFlags::Save))
    {
      if (!bOnlyNewOnes || pCVar->m_bHasNeverBeenLoaded)
      {
        allCVars.PushBack(pCVar);
      }
    }

    // it doesn't matter whether the CVar could be loaded from file, either it works the first time, or it stays at its current value
    pCVar->m_bHasNeverBeenLoaded = false;
  }

  LoadCVarsFromFileInternal(sPath, allCVars, bOnlyNewOnes, bSetAsCurrentValue, pOutCVars);
}

void plCVar::LoadCVarsFromFileInternal(plStringView path, const plDynamicArray<plCVar*>& vars, bool bOnlyNewOnes, bool bSetAsCurrentValue, plDynamicArray<plCVar*>* pOutCVars)
{
  plFileReader File;
  plStringBuilder sTemp;

  if (File.Open(path.GetData(sTemp)) == PLASMA_SUCCESS)
  {
    plStringBuilder sContent;
    sContent.ReadAll(File);

    plDynamicArray<plString> Lines;
    sContent.ReplaceAll("\r", ""); // remove carriage return

    // splits the string at occurrence of '\n' and adds each line to the 'Lines' container
    sContent.Split(true, Lines, "\n");

    plStringBuilder sVarName;
    plStringBuilder sVarValue;

    for (const plString& sLine : Lines)
    {
      if (ParseLine(sLine, sVarName, sVarValue) == PLASMA_FAILURE)
        continue;

      // now find a variable with the same name
      for (plUInt32 var = 0; var < vars.GetCount(); ++var)
      {
        plCVar* pCVar = vars[var];

        if (!sVarName.IsEqual(pCVar->GetName()))
          continue;

        // found the cvar, now convert the text into the proper value *sigh*
        switch (pCVar->GetType())
        {
          case plCVarType::Int:
          {
            plInt32 Value = 0;
            if (plConversionUtils::StringToInt(sVarValue, Value).Succeeded())
            {
              plCVarInt* pTyped = (plCVarInt*)pCVar;
              pTyped->m_Values[plCVarValue::Stored] = Value;
              *pTyped = Value;
            }
          }
          break;
          case plCVarType::Bool:
          {
            bool Value = sVarValue.IsEqual_NoCase("true");

            plCVarBool* pTyped = (plCVarBool*)pCVar;
            pTyped->m_Values[plCVarValue::Stored] = Value;
            *pTyped = Value;
          }
          break;
          case plCVarType::Float:
          {
            double Value = 0.0;
            if (plConversionUtils::StringToFloat(sVarValue, Value).Succeeded())
            {
              plCVarFloat* pTyped = (plCVarFloat*)pCVar;
              pTyped->m_Values[plCVarValue::Stored] = static_cast<float>(Value);
              *pTyped = static_cast<float>(Value);
            }
          }
          break;
          case plCVarType::String:
          {
            const char* Value = sVarValue.GetData();

            plCVarString* pTyped = (plCVarString*)pCVar;
            pTyped->m_Values[plCVarValue::Stored] = Value;
            *pTyped = Value;
          }
          break;
          default:
            PLASMA_REPORT_FAILURE("Unknown CVar Type: {0}", pCVar->GetType());
            break;
        }

        if (pOutCVars)
        {
          pOutCVars->PushBack(pCVar);
        }

        if (bSetAsCurrentValue)
          pCVar->SetToRestartValue();
      }
    }
  }
}

plCommandLineOptionDoc opt_CVar("cvar", "-CVarName", "<value>", "Forces a CVar to the given value.\n\
Overrides persisted settings.\n\
Examples:\n\
-MyIntVar 42\n\
-MyStringVar \"Hello\"\n\
",
  nullptr);

void plCVar::LoadCVarsFromCommandLine(bool bOnlyNewOnes /*= true*/, bool bSetAsCurrentValue /*= true*/, plDynamicArray<plCVar*>* pOutCVars /*= nullptr*/)
{
  plStringBuilder sTemp;

  for (plCVar* pCVar = plCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    if (bOnlyNewOnes && !pCVar->m_bHasNeverBeenLoaded)
      continue;

    sTemp.Set("-", pCVar->GetName());

    if (plCommandLineUtils::GetGlobalInstance()->GetOptionIndex(sTemp) != -1)
    {
      if (pOutCVars)
      {
        pOutCVars->PushBack(pCVar);
      }

      // has been specified on the command line -> mark it as 'has been loaded'
      pCVar->m_bHasNeverBeenLoaded = false;

      switch (pCVar->GetType())
      {
        case plCVarType::Int:
        {
          plCVarInt* pTyped = (plCVarInt*)pCVar;
          plInt32 Value = pTyped->m_Values[plCVarValue::Stored];
          Value = plCommandLineUtils::GetGlobalInstance()->GetIntOption(sTemp, Value);

          pTyped->m_Values[plCVarValue::Stored] = Value;
          *pTyped = Value;
        }
        break;
        case plCVarType::Bool:
        {
          plCVarBool* pTyped = (plCVarBool*)pCVar;
          bool Value = pTyped->m_Values[plCVarValue::Stored];
          Value = plCommandLineUtils::GetGlobalInstance()->GetBoolOption(sTemp, Value);

          pTyped->m_Values[plCVarValue::Stored] = Value;
          *pTyped = Value;
        }
        break;
        case plCVarType::Float:
        {
          plCVarFloat* pTyped = (plCVarFloat*)pCVar;
          double Value = pTyped->m_Values[plCVarValue::Stored];
          Value = plCommandLineUtils::GetGlobalInstance()->GetFloatOption(sTemp, Value);

          pTyped->m_Values[plCVarValue::Stored] = static_cast<float>(Value);
          *pTyped = static_cast<float>(Value);
        }
        break;
        case plCVarType::String:
        {
          plCVarString* pTyped = (plCVarString*)pCVar;
          plString Value = plCommandLineUtils::GetGlobalInstance()->GetStringOption(sTemp, 0, pTyped->m_Values[plCVarValue::Stored]);

          pTyped->m_Values[plCVarValue::Stored] = Value;
          *pTyped = Value;
        }
        break;
        default:
          PLASMA_REPORT_FAILURE("Unknown CVar Type: {0}", pCVar->GetType());
          break;
      }

      if (bSetAsCurrentValue)
        pCVar->SetToRestartValue();
    }
  }
}

void plCVar::ListOfCVarsChanged(plStringView sSetPluginNameTo)
{
  AssignSubSystemPlugin(sSetPluginNameTo);

  LoadCVars();

  plCVarEvent e(nullptr);
  e.m_EventType = plCVarEvent::Type::ListOfVarsChanged;

  s_AllCVarEvents.Broadcast(e);
}


PLASMA_STATICLINK_FILE(Foundation, Foundation_Configuration_Implementation_CVar);
