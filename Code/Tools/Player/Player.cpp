#include <Player/Player.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <Core/Input/InputManager.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GameEngine/Animation/RotorComponent.h>
#include <GameEngine/Animation/SliderComponent.h>
#include <GameEngine/Gameplay/InputComponent.h>
#include <GameEngine/Gameplay/SpawnComponent.h>
#include <GameEngine/Gameplay/TimedDeathComponent.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Meshes/MeshComponent.h>

// this injects the main function
PL_APPLICATION_ENTRY_POINT(plPlayerApplication);

// these command line options may not all be directly used in plPlayer, but the plFallbackGameState reads those options to determine which scene to load
plCommandLineOptionString opt_Project("_Player", "-project", "Path to the project folder.\nUsually an absolute path, though relative paths will work for projects that are located inside the PL SDK directory.", "");
plCommandLineOptionString opt_Scene("_Player", "-scene", "Path to a scene file.\nUsually given relative to the corresponding project data directory where it resides, but can also be given as an absolute path.", "");

plPlayerApplication::plPlayerApplication()
  : plGameApplication("plPlayer", nullptr) // we don't have a fixed project path in this app, so we need to pass that in a bit later
{
}

plResult plPlayerApplication::BeforeCoreSystemsStartup()
{
  // show the command line options, if help is requested
  {
    // since this is a GUI application (not a console app), printf has no effect
    // therefore we have to show the command line options with a message box

    plStringBuilder cmdHelp;
    if (plCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, plCommandLineOption::LogAvailableModes::IfHelpRequested))
    {
      plLog::OsMessageBox(cmdHelp);
      SetReturnCode(-1);
      return PL_FAILURE;
    }
  }

  plStartup::AddApplicationTag("player");

  PL_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  DetermineProjectPath();

  return PL_SUCCESS;
}


void plPlayerApplication::AfterCoreSystemsStartup()
{
  ExecuteInitFunctions();

  plStartup::StartupHighLevelSystems();

  // we need a game state to do anything
  // if no custom game state is available, plFallbackGameState will be used
  // the game state is also responsible for either creating a world, or loading it
  // the plFallbackGameState inspects the command line to figure out which scene to load
  ActivateGameState(nullptr).AssertSuccess();
}

void plPlayerApplication::Run_InputUpdate()
{
  SUPER::Run_InputUpdate();

  if (GetActiveGameState() && GetActiveGameState()->WasQuitRequested())
  {
    RequestQuit();
  }
}

void plPlayerApplication::DetermineProjectPath()
{
  plStringBuilder sProjectPath = opt_Project.GetOptionValue(plCommandLineOption::LogMode::FirstTime);

#if PL_DISABLED(PL_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  // We can't specify command line arguments on many platforms so the project must be defined by plFileserve.
  // plFileserve must be started with the project special dir set. For example:
  // -specialdirs project ".../plEngine/Data/Samples/Testing Chambers

  if (sProjectPath.IsEmpty())
  {
    m_sAppProjectPath = ">project";
    return;
  }
#endif

  if (sProjectPath.IsEmpty())
  {
    const plStringBuilder sScenePath = opt_Scene.GetOptionValue(plCommandLineOption::LogMode::FirstTime);

    // project path is empty, need to extract it from the scene path

    if (!sScenePath.IsAbsolutePath())
    {
      // scene path is not absolute -> can't extract project path
      m_sAppProjectPath = plFileSystem::GetSdkRootDirectory();
      SetReturnCode(1);
      return;
    }

    if (plFileSystem::FindFolderWithSubPath(sProjectPath, sScenePath, "plProject", "plSdkRoot.txt").Failed())
    {
      // couldn't find the 'plProject' file in any parent folder of the scene
      m_sAppProjectPath = plFileSystem::GetSdkRootDirectory();
      SetReturnCode(1);
      return;
    }
  }
  else if (!plPathUtils::IsAbsolutePath(sProjectPath))
  {
    // project path is not absolute, so must be relative to the SDK directory
    sProjectPath.Prepend(plFileSystem::GetSdkRootDirectory(), "/");
  }

  sProjectPath.MakeCleanPath();
  sProjectPath.TrimWordEnd("/plProject");

  if (sProjectPath.IsEmpty())
  {
    m_sAppProjectPath = plFileSystem::GetSdkRootDirectory();
    SetReturnCode(1);
    return;
  }

  // store it now, even if it fails, for error reporting
  m_sAppProjectPath = sProjectPath;

  if (!plOSFile::ExistsDirectory(sProjectPath))
  {
    SetReturnCode(1);
    return;
  }
}
